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
 *
 */

#ifndef SSD1306XLED_H
#define SSD1306XLED_H

// ----------------------------------------------------------------------------

		int ssd1306_init(void);

		void ssd1306_setpos(uint8_t x, uint8_t y);
		void ssd1306_fillscreen(uint8_t fill);
		void ssd1306_char_font6x8(char ch);
		void ssd1306_string_font6x8(char *s);
		void ssd1306_char_f8x16(uint8_t x, uint8_t y, const char ch[]);
	
// ----------------------------------------------------------------------------

#endif