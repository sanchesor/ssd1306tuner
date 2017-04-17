#include "stm32f10x.h"
#include "libssd1306.h"
#include "movingrect.h"
#include <math.h>


volatile uint8_t tl[64];
volatile int tl_index = 0;


void TIM2_IRQHandler()
{
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) == SET)
	{
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
		

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
	tim.TIM_Prescaler = 7200-1;	// 72-1 -> period in us
	tim.TIM_Period = 60000-1;
	TIM_TimeBaseInit(TIM2, &tim);
	
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	TIM_Cmd(TIM2, ENABLE);
	
	nvic.NVIC_IRQChannel = TIM2_IRQn;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 0;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&nvic);
	
}


void draw_timeline()
{
	int offset = 5;
	
	for(int i=0;i<64;i++)
	{
		drawLineH(offset, offset+tl[i], i);
	}
	
}

double speed=1.0;
void calc_timeline()
{
	
	uint16_t t = TIM_GetCounter(TIM2);
	
	/*
	tl_index = t/10;
	
	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6) == 0)
		tl[tl_index] = tl[tl_index]+10;
	else
		tl[tl_index] = 0;	
	*/
	
	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6) == 0)
		speed += 0.01*speed;
	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_7) == 0)
		speed -= 0.01*speed;
		
	for(int i=0;i<63;i++)
	{
		tl[i] = 30*sin(0.1*i + t*0.001*speed);
	}
	
}

int main(void)
{
	hardware_init();
	ssd1306init();
	
	while(1)
	{					
		
		
		ClearBuffer();
		
		calc_timeline();
		draw_timeline();

		TransferBuffer();
		

		
		
	}
}
