#include "stm32f10x.h"
#include "string.h"


#define BUFFER_SIZE (128*64)/8

static uint8_t buffer[BUFFER_SIZE];
			
void ClearBuffer()
{
	memset(buffer, 0, BUFFER_SIZE);
}

void drawPixel(uint16_t x, uint16_t y)
{
	buffer[x+(y/8)*128] |= (1 << (y&7));
}

void drawLineH(uint16_t x1, uint16_t x2, uint16_t y)
{
	for(uint16_t i=x1;i<=x2;i++)
	{
		drawPixel(i, y);
	}
}

void drawLineV(uint16_t y1, uint16_t y2, uint16_t x)
{
	for(uint16_t i=y1;i<=y2;i++)
	{
		drawPixel(x, i);
	}
}

void drawRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	drawLineH(x1, x2, y1);
	drawLineV(y1, y2, x2);
	drawLineH(x1, x2, y2);
	drawLineV(y1, y2, x1);
}

void fillRect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
	for(int x=x1;x<=x2;x++)
	{
		drawLineV(y1, y2, x);
	}
}

void SendStart()
{
	I2C_GenerateSTART(I2C1, ENABLE);
	while(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) != SUCCESS);
}

void SendStop()
{
	I2C_GenerateSTOP(I2C1, ENABLE);
}

void SendAddress()
{
	// slave address 0x3c or 0x3d or 0x78
	I2C_Send7bitAddress(I2C1, 0x78, I2C_Direction_Transmitter);
	while(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) != SUCCESS);
}

void SendStartAndAddress()
{
	SendStart();
	SendAddress();
}

void SendData(uint8_t data)
{
	I2C_SendData(I2C1, data);
	while(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) != SUCCESS);
}

void init()
{
	SendStartAndAddress();

	SendData(0xAE);                    // Display Off

	SendData(0x00 | 0x0);            // low col = 0
	SendData(0x10 | 0x0);           // hi col = 0
	SendData(0x40 | 0x0);            // line #0

	SendData(0x81);                   // Set Contrast 0x81
	SendData(0xCF);
										// flips display
	SendData(0xA1);                    // Segremap - 0xA1
	SendData(0xC8);                    // COMSCAN DEC 0xC8 C0
	SendData(0xA6);                    // Normal Display 0xA6 (Invert A7)

	SendData(0xA4);                // DISPLAY ALL ON RESUME - 0xA4
	SendData(0xA8);                    // Set Multiplex 0xA8
	SendData(0x3F);                    // 1/64 Duty Cycle

	SendData(0xD3);                    // Set Display Offset 0xD3
	SendData(0x0);                     // no offset

	SendData(0xD5);                    // Set Display Clk Div 0xD5
	SendData(0x80);                    // Recommneded resistor ratio 0x80

	SendData(0xD9);                  // Set Precharge 0xd9
	SendData(0xF1);

	SendData(0xDA);                    // Set COM Pins0xDA
	SendData(0x12);

	SendData(0xDB);                 // Set VCOM Detect - 0xDB
	SendData(0x40);

	SendData(0x20);                    // Set Memory Addressing Mode
	SendData(0x00);                    // 0x00 - Horizontal

	SendData(0x40 | 0x0);              // Set start line at line 0 - 0x40

	SendData(0x8D);                    // Charge Pump -0x8D
	SendData(0x14);


	SendData(0xA4);              //--turn on all pixels - A5. Regular mode A4
	SendData(0xAF);                //--turn on oled panel - AF

	SendStop();
}

#define SSD1306_LCDWIDTH      128
#define SSD1306_LCDHEIGHT      64
#define SSD1306_SETCONTRAST   0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR   0x22
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_CHARGEPUMP 0x8D
#define SSD1306_EXTERNALVCC 0x1
#define SSD1306_SWITCHCAPVCC 0x2


///////////////////////////////////////
//
void  ssd1306_command(uint8_t c)
{
    uint8_t control = 0x00; // some use 0X00 other examples use 0X80. I tried both
    SendStartAndAddress();
    SendData(control); // This is Command
    SendData(c);
    SendStop();
}
////////////////////////////////////////////
//
void  ssd1306_data(uint8_t c)
{
	SendStartAndAddress();
    SendData(0X40); // This byte is DATA
    SendData(c);
    SendStop();
}
///////////////////////////////////////////////////
// Used when doing Horizontal or Vertical Addressing
void setColAddress()
{
  ssd1306_command(SSD1306_COLUMNADDR); // 0x21 COMMAND
  ssd1306_command(0); // Column start address
  ssd1306_command(SSD1306_LCDWIDTH-1); // Column end address
}
/////////////////////////////////////////////////////
// Used when doing Horizontal or Vertical Addressing
void setPageAddress()
{
  ssd1306_command(SSD1306_PAGEADDR); // 0x22 COMMAND
  ssd1306_command(0); // Start Page address
  ssd1306_command((SSD1306_LCDHEIGHT/8)-1);// End Page address
}
///////////////////////////////////////////////////////////
// Transfers the local buffer to the CGRAM in the SSD1306
void TransferBuffer()
{
  uint16_t j=0;

  // set the Column and Page addresses to 0,0
  setColAddress();
  setPageAddress();

  SendStartAndAddress();
  SendData(0X40); // data not command
  for(j=0;j<BUFFER_SIZE;j++)
  {
	SendData(buffer[j]);
  }

  SendStop();
}

void simple_delay(int delay)
{
	for(int i=0;i<=4000;i++)
	{
		for(int j=delay;j>0;j--)
		{

		}
	}
}

typedef struct MovingRect
{
	double x1;
	double y1;
	double x2;
	double y2;
	double dx;
	double dy;
	uint8_t isFilled;
} MovingRect;

void MoveRect(MovingRect* r)
{
	int max_x = 127;
	int max_y = 63;


	r->x1 += r->dx;
	r->x2 += r->dx;
	r->y1 += r->dy;
	r->y2 += r->dy;

	if(r->dx > 0 && r->x1 > max_x)
		r->x1 -= max_x;

	if(r->dx > 0 && r->x2 > max_x)
		r->x2 -= max_x;
	
	if(r->dx < 0 && r->x1 < 0)
		r->x1 += max_x;

	if(r->dx < 0 && r->x2 < 0)
		r->x2 += max_x;
	
	if(r->dy > 0 && r->y1 > max_y)
		r->y1 -= max_y;

	if(r->dy > 0 && r->y2 > max_y)
		r->y2 -= max_y;

	if(r->dy < 0 && r->y1 < 0)
		r->y1 += max_y;

	if(r->dy < 0 && r->y2 < 0)
		r->y2 += max_y;

	
	int x1,x2,y1,y2;
	int xx1,xx2,yy1,yy2;
	int is2rect = 0;
	
	if(r->x1 < r->x2)
	{
		x1 = r->x1;
		x2 = r->x2;
		xx1 = x1;
		xx2 = x2;
	}
	else
	{
		x1 = r->x1;
		x2 = max_x;
		xx1 = 0;
		xx2 = r->x2;
		is2rect = 1;
	}
	
	if(r->y1 < r->y2)
	{
		y1 = r->y1;
		y2 = r->y2;
		yy1 = y1;
		yy2 = y2;
	}
	else
	{
		y1 = r->y1;
		y2 = max_y;
		yy1 = 0;
		yy2 = r->y2;
		is2rect = 1;
	}	
	
	if(r->isFilled == 1)
	{		
		fillRect(x1, y1, x2, y2);
		if(is2rect == 1)
			fillRect(xx1, yy1, xx2, yy2);
	}
	else
	{
		drawRect(x1, y1, x2, y2);
		if(is2rect == 1)
			drawRect(xx1, yy1, xx2, yy2);
	}
}

void hardware_init()
{
	GPIO_InitTypeDef gpio;
	I2C_InitTypeDef i2c;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);

	GPIO_StructInit(&gpio);
	gpio.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; // SCL, SDA
	gpio.GPIO_Mode = GPIO_Mode_AF_OD;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &gpio);

	I2C_StructInit(&i2c);
	i2c.I2C_Mode = I2C_Mode_I2C;
	i2c.I2C_ClockSpeed = 1000000;
	I2C_Init(I2C1, &i2c);
	I2C_Cmd(I2C1, ENABLE);
	
}

int main(void)
{
	hardware_init();
	init();

	int w = 12;
	int sep = 2;
	int h = 10;
	MovingRect r1 = {.x1 = 0, .y1 = 0, .x2 = 1*w-sep, .y2 = h, .dx = 0, .dy = 1, .isFilled = 1};
	MovingRect r2 = {.x1 = 1*w, .y1 = 0, .x2 = 2*w-sep, .y2 = h, .dx = 0, .dy = 2, .isFilled = 1};
	MovingRect r3 = {.x1 = 2*w, .y1 = 0, .x2 = 3*w-sep, .y2 = h, .dx = 0, .dy = 4, .isFilled = 1};
	MovingRect r4 = {.x1 = 3*w, .y1 = 0, .x2 = 4*w-sep, .y2 = h, .dx = 0, .dy = 8, .isFilled = 1};
	MovingRect r5 = {.x1 = 4*w, .y1 = 0, .x2 = 5*w-sep, .y2 = h, .dx = 0, .dy = 16, .isFilled = 1};
	MovingRect r6 = {.x1 = 5*w, .y1 = 0, .x2 = 6*w-sep, .y2 = h, .dx = 0, .dy = 31, .isFilled = 1};


//	MovingRect r7 = {.x1 = 10, .y1 = 10, .x2 = 50, .y2 = 50, .dx = 0.3, .dy = 0.2, .isFilled = 1};

	while(1)
	{
		ClearBuffer();
		
		
		MoveRect(&r1);
		MoveRect(&r2);
		MoveRect(&r3);
		MoveRect(&r4);
		MoveRect(&r5);
		MoveRect(&r6);
		drawLineV(0,63,6*w+1);
		
//		MoveRect(&r7);

		TransferBuffer();
	}
}
