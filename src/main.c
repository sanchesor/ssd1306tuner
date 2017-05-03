#include "stm32f10x.h"
#include "libssd1306.h"

#define T1 4010
#define T2 3007
#define T3 2257
#define T4 1690
#define T5 1340
#define T6 1002

#define COUNTER_MAX 10000 - 1
//#define COUNTER_MAX 65535	// 0xFFFF = max uint16_t

int TT[] = {T1, T2, T3, T4, T5, T6};

#define INDEX_MAX 64
#define INDEX_CUR 65
#define INDEX_LAST_POS 66
#define INDEX_DIR 67
#define INDEX_DIR_P1 68
#define INDEX_DIR_P2 69
#define INDEX_DIR_P3 70
#define INDEX_DIR_AVG 71
#define INDEX_MIN 72
#define INDEX_MIN_P 73
#define INDEX_MIN_COUNT 74

#define INDEX_LINES_SIZE 75

volatile int lines[6][INDEX_LINES_SIZE];	


volatile int num_lines;

volatile int offset[] = {0,0,0,0,0,0};

volatile int base_offset[] = {
	COUNTER_MAX%T1, 
	COUNTER_MAX%T2, 
	COUNTER_MAX%T3, 
	COUNTER_MAX%T4, 
	COUNTER_MAX%T5, 
	COUNTER_MAX%T6
};



void hardware_init()
{
	GPIO_InitTypeDef gpio;
	I2C_InitTypeDef i2c;
	TIM_TimeBaseInitTypeDef tim;
	NVIC_InitTypeDef nvic;
	ADC_InitTypeDef adc;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOA|RCC_APB2Periph_ADC1|RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1|RCC_APB1Periph_TIM2, ENABLE);
	
	 
	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; // SCL, SDA
	gpio.GPIO_Mode = GPIO_Mode_AF_OD;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio);

	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;	// faster, slower
	gpio.GPIO_Mode = GPIO_Mode_IPU;
	gpio.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &gpio);

	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_0;	// A0
	gpio.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_Init(GPIOA, &gpio);
	
	I2C_StructInit(&i2c);
	i2c.I2C_Mode = I2C_Mode_I2C;
	i2c.I2C_ClockSpeed = 1000000;
	I2C_Init(I2C1, &i2c);
	I2C_Cmd(I2C1, ENABLE);
		
	TIM_TimeBaseStructInit(&tim);
	tim.TIM_CounterMode = TIM_CounterMode_Up;
	tim.TIM_Prescaler = 217;	// 72Mhz/217 -> 3us
	tim.TIM_Period = COUNTER_MAX;
	TIM_TimeBaseInit(TIM2, &tim);
	
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM2, ENABLE);
	
	nvic.NVIC_IRQChannel = TIM2_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 0;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);

	RCC_ADCCLKConfig(RCC_PCLK2_Div6);
	
	ADC_StructInit(&adc);
	adc.ADC_ContinuousConvMode = ENABLE;
	adc.ADC_NbrOfChannel = 1;
	adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_Init(ADC1, &adc);	
	
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_71Cycles5);
	ADC_Cmd(ADC1, ENABLE);

	ADC_ResetCalibration(ADC1);
	while (ADC_GetResetCalibrationStatus(ADC1));

	ADC_StartCalibration(ADC1);
	while (ADC_GetCalibrationStatus(ADC1));

	ADC_SoftwareStartConvCmd(ADC1, ENABLE);	
	
}

void draw_lines()
{
	int offset = 1;
	int maxh = 20;
	
	for(int l=0;l<6;l++)
	{
		for(int i=0;i<64;i++)
		{
			drawLineH(offset + l*maxh, offset + l*maxh + lines[l][i], i);			
		}	
	
		int dir = lines[l][INDEX_DIR_AVG];
		if(dir>0)
			drawThickArrowV(63, 63-dir, offset + l*maxh + 10);
		else if(dir<0)
			drawThickArrowV(1, 1-dir, offset + l*maxh + 10);			
	}
}

void clear_lines()
{
	for(int l=0;l<num_lines;l++)
	{
		for(int i=0;i<64;i++)
			lines[l][i] = 0;
	}
}

void shift_dir(int l)
{
	int avg = (lines[l][INDEX_DIR]
		+ lines[l][INDEX_DIR_P1]
		+ lines[l][INDEX_DIR_P2]
		+ lines[l][INDEX_DIR_P3])/4;
	
	lines[l][INDEX_DIR_AVG] = avg;
	
	lines[l][INDEX_DIR_P3] = lines[l][INDEX_DIR_P2];
	lines[l][INDEX_DIR_P2] = lines[l][INDEX_DIR_P1];
	lines[l][INDEX_DIR_P1] = lines[l][INDEX_DIR];	
}

void calc_dir(int l)
{
	int threshold = lines[l][INDEX_MAX] * 3/4;	// 75%
	if(threshold > 10)
	{		
		int lower1=-1,lower2=-1,higher1=-1,higher2=-1;
		for(int i=0;i<64 && higher2==-1;i++)
		{
			if(lines[l][i] < threshold && lower1==-1)
			{
				lower1 = lines[l][i];
			}
			else if(lines[l][i] < threshold && lines[l][i] > lower1 && lower2==-1) 	
			{
				lower2 = lines[l][i];
			}
			else if(lines[l][i] >= threshold && higher1 == -1)
			{
				higher1 = lines[l][i];
			}
			else if(lines[l][i] >= threshold && lines[l][i] >= higher1 && higher2 == -1)
			{
				higher2 = lines[l][i];
				
				int dir = i - lines[l][INDEX_LAST_POS];
				if(dir > 32)
					dir -= 64;
				else if (dir < -32)
					dir += 64;
					
				lines[l][INDEX_DIR] = dir;
				lines[l][INDEX_LAST_POS] = i;				
			}
			else
			{
				lower1=-1;
				lower2=-1;
				higher1=-1;
				higher2=-1;
				lines[l][INDEX_DIR] = 0;
			}
			
		}
	}
	else
	{
		lines[l][INDEX_DIR] = 0;
	}
	
	shift_dir(l);
}


void calc_lines_all_waves()
{
	uint16_t cur_v = ADC_GetConversionValue(ADC1)/4;

		
	uint16_t t = TIM_GetCounter(TIM2);
	
	// 6 lines = 6 strings
	num_lines = 6;
	for(int l=0;l<num_lines;l++)
	{
		uint32_t tp = (t + offset[l])%TT[l];		
		int li = 64*tp/TT[l];
		
		// first 0
		if(li == 0 && lines[l][INDEX_CUR] == 63)
		{									
			calc_dir(l);
			lines[l][INDEX_MAX] = 0;
			
			lines[l][INDEX_MIN_P] = lines[l][INDEX_MIN];
			lines[l][INDEX_MIN] = 5000;
		}
		
		lines[l][INDEX_CUR] = li;
		
		int l_cur_v = cur_v - lines[l][INDEX_MIN_P];
		if(l_cur_v < 0)
			l_cur_v = 0;
		else if(l_cur_v > 19)
			l_cur_v = 19;
				
		
		// filtering
		if(li>1)
		{
			int diff1 = lines[l][li-1] - lines[l][li-2];
			int diff2 = l_cur_v - lines[l][li-1];
			int diff = diff1-diff2;
			if(diff > 3 || diff < -3)
			{
				lines[l][li-1] = (l_cur_v + lines[l][li-2])/2;
			}
		}
							
		if(l_cur_v > lines[l][INDEX_MAX])
			lines[l][INDEX_MAX] = l_cur_v;
		
		if(cur_v < lines[l][INDEX_MIN])
			lines[l][INDEX_MIN] = cur_v;
		
				
		lines[l][li] = l_cur_v;	
		
		/* count test
		lines[l][li]++;		
		if(li < 63)
		{			
			lines[l][li+1] = 0;
		}
		else
		{
			lines[l][0] = 0;
		}
		*/
	}		
}


void TIM2_IRQHandler()
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		
		for(int i=0;i<6;i++)
		{
			offset[i] = (offset[i] + base_offset[i])%TT[i];
		}

		ClearBuffer();		
		draw_lines();
		TransferBuffer();						
	}
}

int main(void)
{
	hardware_init();
	ssd1306init();
					
	while(1)
	{		
		calc_lines_all_waves();
	}
}

