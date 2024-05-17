/*
 * SSD1306xLED - Drivers for SSD1306 controlled dot matrix OLED/PLED 128x64 displays
 *
 * @created: 2014-08-12
 * @author: Neven Boyanov
 *
 * Source code available at: https://bitbucket.org/tinusaur/ssd1306xled
 *
 * Modified by Tejashwi Kalp Taru, with the help of TinyI2C (https://github.com/technoblogy/tiny-i2c/)
 * Modified code available at: https://github.com/tejashwikalptaru/ssd1306xled
 */

// ----------------------------------------------------------------------------

#include "task.h"
#include "asf.h"
#include "i2c1.h"

#include "ssd1306xled.h"
#include "font6x8.h"
#include "font8x16.h"
#include "lcd.h"

#define SSD1306_COMMAND 0x00
#define SSD1306_DATA 0x40

// ----------------------------------------------------------------------------

// Some code based on "IIC_wtihout_ACK" by http://www.14blog.com/archives/1358

const uint8_t ssd1306_init_sequence [26] = {	// Initialization Sequence
    
	0xAE,			// Set Display ON/OFF - AE=OFF, AF=ON
	0xD5, 0xF0,		// Set display clock divide ratio/oscillator frequency, set divide ratio
	0xA8, 0x3F,		// Set multiplex ratio (1 to 64) ... (height - 1)
	0xD3, 0x00,		// Set display offset. 00 = no offset
	0x40 | 0x00,	// Set start line address, at 0.
	0x8D, 0x14,		// Charge Pump Setting, 14h = Enable Charge Pump
	0x20, 0x00,		// Set Memory Addressing Mode - 00=Horizontal, 01=Vertical, 10=Page, 11=Invalid
	0xA0 | 0x01,	// Set Segment Re-map
	0xC8,			// Set COM Output Scan Direction
	0xDA, 0x12,		// Set COM Pins Hardware Configuration - 128x32:0x02, 128x64:0x12
	0x81, 0x3F,		// Set contrast control register
	0xD9, 0x22,		// Set pre-charge period (0x22 or 0xF1)
	0xDB, 0x20,		// Set Vcomh Deselect Level - 0x00: 0.65 x VCC, 0x20: 0.77 x VCC (RESET), 0x30: 0.83 x VCC
	0xA4,			// Entire Display ON (resume) - output RAM to display
	0xA6,			// Set Normal/Inverse Display mode. A6=Normal; A7=Inverse
	0x2E,			// Deactivate Scroll command
	0xAF,			// Set Display ON/OFF - AE=OFF, AF=ON
};

static uint8_t obuf[1024];


int ssd1306_init(void)
{
    uint8_t i;
    
    obuf[0] = SSD1306_COMMAND;
    
    for(i=0;i<26;i++)
	{
        obuf[i+1] = ssd1306_init_sequence[i];
	}
	
	if(!i2c1_writeNBytes(SSD1306_SA, obuf, i+1))
        return 0;
        
	ssd1306_fillscreen(0);
    
    return 1;
}


void ssd1306_fillscreen(uint8_t fill) {
	uint16_t p;
    uint8_t y;
    
    
    for(y=0;y<8;y++)
    {
        ssd1306_setpos(0, y);
    
        p=1;
         obuf[0]=  SSD1306_DATA;
    
            for (uint16_t i = 0; i < 128; i++) 
            {
                obuf[p++]=(fill);
            }
    
        i2c1_writeNBytes(SSD1306_SA, obuf, p); 
    }
        
}

void ssd1306_setpos(uint8_t x, uint8_t y)
{
   
    obuf[0] = SSD1306_COMMAND;
    obuf[1] = (0xb0 | (y & 0x07));
	obuf[2] = (0x10 | ((x & 0xf0) >> 4));
	obuf[3] = (x & 0x0f); // | 0x01
    
    i2c1_writeNBytes(SSD1306_SA, obuf, 4); 
        
}

void ssd1306_char_font6x8(char ch) {
	uint8_t i;
	uint8_t c = ch - 32;

    obuf[0]=  SSD1306_DATA;
    
	for (i= 0; i < 6; i++)
	{
		obuf[i+1]=ssd1306xled_font6x8[c * 6 + i];
	}
	
    i2c1_writeNBytes(SSD1306_SA, obuf, i+1); 
}

void ssd1306_string_font6x8(char *s) {
	while (*s) {
		ssd1306_char_font6x8(*s++);
	}
}

void ssd1306_char_f8x16(uint8_t x, uint8_t y, const char ch[])
{
	uint8_t c, j=0, i = 0;
	while (ch[j] != '\0')
	{
		c = ch[j] - 32;
		if (x > 120)
		{
			x = 0;
			y++;
		}
		ssd1306_setpos(x, y);
        
        obuf[0]=  SSD1306_DATA;
    
		for (i = 0; i < 8; i++)
		{
			obuf[i+1]=(ssd1306xled_font8x16[c * 16 + i]);
		}
        
        i2c1_writeNBytes(SSD1306_SA, obuf, i+1); 
        
		ssd1306_setpos(x, y + 1);
        
        obuf[0]=  SSD1306_DATA;
		
		for (i = 0; i < 8; i++)
		{
			obuf[i+1]=(ssd1306xled_font8x16[c * 16 + i + 8]);
		}
		
        i2c1_writeNBytes(SSD1306_SA, obuf, i+1); 
        
		x += 8;
		j++;
	}
}


// ----------------------------------------------------------------------------

char LCDText[16*4+1];

// Glue to previous demo board
void LCD_Initialize(void)
{
	ssd1306_init();
}



void LCD_Update(void)
{
	ssd1306_setpos(0,0);
	ssd1306_string_font6x8(LCDText);
}

void LCD_Erase(void)
{
	ssd1306_fillscreen(0);
}

void LCDDisplay(char * text, uint8_t value, bool delay)
{
	/* Clear screen */
	ssd1306_fillscreen(0);

	snprintf(LCDText, sizeof(LCDText), text);

	if (value)
	{
		uint8_t char_count = 0;
		do {
			++char_count;
		} while (*(++text));
	
		snprintf(LCDText+char_count, sizeof(LCDText), "%d", value);
	}

	ssd1306_setpos(0,0);
	ssd1306_string_font6x8(LCDText);

	if(delay)
	{
		delay_ms(2000);
	}	
}

void LCDTRXCount(uint8_t txCount, uint8_t rxCount)
{
	snprintf(LCDText, sizeof(LCDText), "TX Messages: %d \nRX Messages: %d", txCount, rxCount);
	
	ssd1306_setpos(0,0);
	ssd1306_string_font6x8(LCDText);
	
	delay_ms(200);
}