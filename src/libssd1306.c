#include "libssd1306.h"
#include "string.h"

#define BUFFER_SIZE (128*64)/8

uint8_t buffer[BUFFER_SIZE];
			
void ClearBuffer()
{
	memset(buffer, 0, BUFFER_SIZE);
}

void drawPixel(uint16_t x, uint16_t y)
{
	if(x < 0 || x > 127 || y < 0 || y > 63)
		return;
	
	buffer[x+(y/8)*128] |= (1 << (y&7));
}

void drawLineH(uint16_t x1, uint16_t x2, uint16_t y)
{
	int xb=x1,xe=x2;
	
	if(x2 < x1)
	{
		xb = x2;
		xe = x1;
	}
	
	for(uint16_t i=xb;i<=xe;i++)
	{
		drawPixel(i, y);
	}
}

void drawLineV(uint16_t y1, uint16_t y2, uint16_t x)
{
	int yb=y1,ye=y2;
	
	if(y2 < y1)
	{
		yb = y2;
		ye = y1;
	}
		
	for(uint16_t i=yb;i<=ye;i++)
	{
		drawPixel(x, i);
	}
}

void drawArrowV(uint16_t y1, uint16_t y2, uint16_t x)
{
	drawLineV(y1,y2,x);
	if(y2 > y1)
	{
		drawPixel(x-1,y2-1);
		drawPixel(x+1,y2-1);
		drawPixel(x-2,y2-2);
		drawPixel(x+2,y2-2);	
	}
	else
	{
		drawPixel(x-1,y2+1);
		drawPixel(x+1,y2+1);
		drawPixel(x-2,y2+2);
		drawPixel(x+2,y2+2);			
	}
}

void drawThickArrowV(uint16_t y1, uint16_t y2, uint16_t x)
{
	drawLineV(y1,y2,x);	
	
	if(y2 > y1)
	{
		drawPixel(x-1,y2-1);
		drawPixel(x+1,y2-1);
		drawPixel(x-2,y2-2);
		drawPixel(x+2,y2-2);	
		
		drawPixel(x-1,y2-2);
		drawPixel(x+1,y2-2);
		drawPixel(x-2,y2-3);
		drawPixel(x+2,y2-3);	

		drawLineV(y1,y2-1,x-1);
		drawLineV(y1,y2-1,x+1);	
	}
	else
	{
		drawPixel(x-1,y2+1);
		drawPixel(x+1,y2+1);
		drawPixel(x-2,y2+2);
		drawPixel(x+2,y2+2);			
		
		drawPixel(x-1,y2+2);
		drawPixel(x+1,y2+2);
		drawPixel(x-2,y2+3);
		drawPixel(x+2,y2+3);			
		
		drawLineV(y1,y2+1,x-1);
		drawLineV(y1,y2+1,x+1);	
		
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

void ssd1306init()
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
