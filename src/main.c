#include "stm32f10x.h"
#include "libssd1306.h"
#include "movingrect.h"

#define T1 4027
#define T2 3002
#define T3 2246
#define T4 1685
#define T5 1337
#define T6 1000

#define COUNTER_MAX 10000 - 1

uint16_t TT[] = {T1, T2, T3, T4, T5, T6};

volatile uint8_t lines[6][64];

volatile uint16_t offset[] = {0,0,0,0,0,0};

volatile uint16_t base_offset[] = {
	COUNTER_MAX%T1, 
	COUNTER_MAX%T2, 
	COUNTER_MAX%T3, 
	COUNTER_MAX%T4, 
	COUNTER_MAX%T5, 
	COUNTER_MAX%T6
};

void TIM2_IRQHandler()
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		
		for(int i=0;i<6;i++)
		{
			offset[i] = (offset[i] + base_offset[i])%TT[i];
		}

	}
}


void hardware_init()
{
	GPIO_InitTypeDef gpio;
	I2C_InitTypeDef i2c;
	TIM_TimeBaseInitTypeDef tim;
	NVIC_InitTypeDef nvic;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOA, ENABLE);
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


	TIM_TimeBaseStructInit(&tim);
	tim.TIM_CounterMode = TIM_CounterMode_Up;
	tim.TIM_Prescaler = 7200-1;
	tim.TIM_Period = 200;
	TIM_TimeBaseInit(TIM3, &tim);
	
	TIM_ITConfig(TIM3, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
	
	nvic.NVIC_IRQChannel = TIM3_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 0;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);
	
}

/*
void draw_timeline()
{
	int offset = 5;
	
	for(int i=0;i<64;i++)
	{
		drawLineH(offset, offset+tl[i], i);
	}	
}
*/

void draw_lines()
{
	int offset = 5;
	int maxh = 20;
	
	for(int l=0;l<6;l++)
	{
		for(int i=0;i<64;i++)
		{
			drawLineH(offset + l*maxh, offset + l*maxh + lines[l][i], i);
		}	
	}
}

void calc_lines()
{
	//uint16_t v = ADC_GetConversionValue(ADC1);
	uint16_t cur_v = 4;
	
	// normalize cur_v !
	
	uint16_t t = TIM_GetCounter(TIM2);
	
	for(int l=0;l<6;l++)
	{
		uint16_t tp = (t + offset[l])%TT[l];		
		int li = 64*tp/TT[l];
		
		lines[l][li] = cur_v;
	}	
}

void TIM3_IRQHandler()
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) == SET)
	{
		TIM_ClearITPendingBit(TIM3, TIM_IT_Update);

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


		

	}
}
