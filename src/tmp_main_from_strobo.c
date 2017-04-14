/*
 * strobo.c
 *
 * Created: 2016-06-15 14:14:26
 * Author : pasawick
 */ 

#define F_CPU 16000000L

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
//#include "libusart.h"
//#include <stdio.h>


// 4 us
#define T1 3035	// 3000 * 4us = 12ms = 1/82Hz
#define T2 2275
#define T3 1705
#define T4 1275
#define T5 1010
#define T6 758


#define COUNTER_MAX 0xffff - T1 - 100	// whatever

uint16_t TT[] = {T1, T2, T3, T4, T5, T6};
	
uint16_t TTs[][6] = 
{
	{0, 0, 0, 0, 0, 0},												// 0/8
	{0.125*T1, 0.125*T2, 0.125*T3, 0.125*T4, 0.125*T5, 0.125*T6},	// 1/8
	{0.25*T1, 0.25*T2, 0.25*T3, 0.25*T4, 0.25*T5, 0.25*T6},			// 2/8
	{0.375*T1, 0.375*T2, 0.375*T3, 0.375*T4, 0.375*T5, 0.375*T6},	// 3/8
	{0.5*T1, 0.5*T2, 0.5*T3, 0.5*T4, 0.5*T5, 0.5*T6},				// 4/8
	{0.625*T1, 0.625*T2, 0.625*T3, 0.625*T4, 0.625*T5, 0.625*T6},	// 5/8
	{0.75*T1, 0.75*T2, 0.75*T3, 0.75*T4, 0.75*T5, 0.75*T6},			// 6/8
	{0.875*T1, 0.875*T2, 0.875*T3, 0.875*T4, 0.875*T5, 0.875*T6},	// 7/8
	{T1, T2, T3, T4, T5, T6}										// 8/8
};

volatile uint16_t offset[] = {0,0,0,0,0,0};

volatile uint16_t base_offset[] = {
	COUNTER_MAX%T1, 
	COUNTER_MAX%T2, 
	COUNTER_MAX%T3, 
	COUNTER_MAX%T4, 
	COUNTER_MAX%T5, 
	COUNTER_MAX%T6
};

char matrix[] = {0,0,0,0,0,0,0,0};

uint16_t adc_value = 0;
int max_value = 0;
int max_pack[200];

int last_push = 0;
int string_mode = -1;

void timer_init()
{
	TCCR1B |= (1 << WGM12);		// Mode 4, CTC on OCR1A
	TIMSK1 |= (1 << OCIE1A);	//Set interrupt on compare match
	TCCR1B |= (1 << CS11) | (1 << CS10); //  64
	OCR1A = COUNTER_MAX;	 // max
	
	sei();
}

char reset = 0;
ISR (TIMER1_COMPA_vect)	
{
	for(int i=0;i<6;i++)
	{
		offset[i] = (offset[i] + base_offset[i])%TT[i];
	}
	reset = 1;
}


void adc_init()
{
	ADCSRA |= (1<<ADEN)|(1<<ADPS1)|(1<<ADPS2);	// ADC ON, prescaler 64
	ADMUX |= (1<<REFS1)|(1<<REFS0);		// internal 1.1v voltage reference
}

int mi = 0;
void read_adc()
{
	ADMUX = (ADMUX & 0b11111000) | PC3;
	ADCSRA |= (1<<ADSC);	// start
	while(ADCSRA & (1<<ADSC));	// wait
			
	adc_value = ADCW;
	
	/*
	if(mi < 200)
		max_pack[mi++] = adc_value;
	else
	{
		max_value = 0;
		for(int i=0;i<200;i++)
		{
			max_value = max_pack[i] > max_value ? max_pack[i] : max_value;
		}
		mi = 0;
	}
	*/
}



void spi_init()
{
	// mosi & sck & SS as output
	DDRB |= (1<<5)|(1<<3)|(1<<2);
	
	// ss up
	PORTB |= (1<<2); 
	
	// spi enable, master, fsck/16
	SPCR |= (1<<SPE)|(1<<MSTR)|(1<<SPR0);
}

void spi_write(char data)
{
	SPDR = data;
	while(!(SPSR & (1<<SPIF)));
}

void matrix_write(char cByte, char dByte)
{
	PORTB &= ~(1<<2);	// ss down
	spi_write(cByte);
	spi_write(dByte);
	PORTB |= (1<<2);	// ss up
	
}

void matrix_init()
{
	spi_init();
	
	matrix_write(0x0a, 0x07);	// intensity 3
	matrix_write(0x0b, 0x07);	// no scan limit
	matrix_write(0x09, 0x00);	// no decode
	matrix_write(0x0c, 0x01);	// no shutdown
	matrix_write(0x0f, 0x00);	// no test
	
	// clear
	matrix_write(1, 0);
	matrix_write(2, 0);
	matrix_write(3, 0);
	matrix_write(4, 0);
	matrix_write(5, 0);
	matrix_write(6, 0);
	matrix_write(7, 0);
	matrix_write(8, 0);
	
}


void calculate_matrix_8_1()
{
	//int limit = max_value*0.2;
	uint16_t limit = 0;
	for(int i=0;i<6;i++)	// linia = struna
	{
		if(string_mode == i || string_mode == -1)
		{
			uint16_t t = (TCNT1 + offset[i])%TT[i];
			
			for(int p=0;p<8;p++)
			{
				//if( (t >= TT[i]*0.125*p && t < TT[i]*0.125*(p+1)) && adc_value > limit)
				if(t >= TTs[p][i] && t < TTs[p+1][i] && adc_value > limit)
					matrix[i] |= (1<<p);
				else
					matrix[i] &= ~(1<<p);
			}		
		}
		else
		{
			matrix[i] = 0;
		}
	}
}


void calculate_matrix()
{
	calculate_matrix_8_1();
	
	if(string_mode == -1)
		matrix[7] = 0b00111111;
	else
		matrix[7] = (1<<string_mode);
}

void update_matrix()
{
	matrix_write(1, matrix[0]);
	matrix_write(2, matrix[1]);
	matrix_write(3, matrix[2]);
	matrix_write(4, matrix[3]);
	matrix_write(5, matrix[4]);
	matrix_write(6, matrix[5]);
	// line 7 mute
	matrix_write(8, matrix[7]);
}


void push_mode_init()
{
	DDRB &= ~(1<<1);
	PORTB |= (1<<1);	
}

void push_mode_update()
{
	if((PINB & (1<<1)) == 0)	// pushed
	{
		last_push = 1;
	}
	else
	{
		if(last_push == 1)
		{
			string_mode += 1;
			if(string_mode == 6)
				string_mode = -1;
		}
		last_push = 0;
	}	
}


int loop = 0;
int main(void)
{	
	push_mode_init();
    matrix_init();
	adc_init();
	timer_init();
		
	//usart_init(__UBRR);	
	//reset = 1;
	
    while (1) 
    {		
		push_mode_update();		
		read_adc();		
		calculate_matrix();
		update_matrix();

		//usart_put_uint16_t(adc_value);		
		/*
		loop++;
		if(reset == 1)
		{
			usart_put_uint16_t(loop);
			reset = 0;
			loop = 0;
		}
		*/
    }
}

