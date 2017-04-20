#include "stm32f10x.h"
#include "libssd1306.h"
#include "movingrect.h"

#define T1 4010
#define T2 3002
#define T3 2257
#define T4 1690
#define T5 1340
#define T6 1005

//#define COUNTER_MAX 10000 - 1
#define COUNTER_MAX 65535	// 0xFFFF = max uint16_t

int TT[] = {T1, T2, T3, T4, T5, T6};

volatile uint8_t lines[6][64];
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
	}
}

void calc_lines_all_waves()
{
	uint16_t cur_v = ADC_GetConversionValue(ADC1)/4;

	if(cur_v > 19)
		cur_v = 19;
	
	uint16_t t = TIM_GetCounter(TIM2);
	
	// 6 lines = 6 strings
	num_lines = 6;
	for(int l=0;l<num_lines;l++)
	{
		uint32_t tp = (t + offset[l])%TT[l];		
		int li = 64*tp/TT[l];
		
		lines[l][li] = cur_v;
	}	
	
}

void calc_lines_one_wave_2wider(int string)
{
	int cur_v = ADC_GetConversionValue(ADC1)/4;

	if(cur_v > 19)
		cur_v = 19;
	
	int t = TIM_GetCounter(TIM2);
	
	int factor = 1;	
	// 4 lines = 4 ranges
	num_lines = 4;
	for(int l=0;l<num_lines;l++)
	{
		int tp = (t + offset[string])%(TT[string]*factor);		
		int li = 64*tp/(TT[string]*factor);
		
		lines[l][li] = cur_v;
		
		factor *= 2;
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
		calc_lines_one_wave_2wider(0);
	}
}

