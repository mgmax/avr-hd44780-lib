/*
 *    Filename: lcd.c
 *     Version: 0.2.3b - last change: removed special cursor handling in lcd_putchar
 * Description: HD44780 Display Library for ATMEL AVR
 *     License: Public Domain
 *
 *      Author: Copyright (C) Max Gaukler <development@maxgaukler.de>
 *        Date: 2010
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

#include "lcd.h"
/// global variable that stores the current position. \see lcd_set_position() \ingroup lcd_functions
uint8_t lcd_position=0;
/// global variable that stores the active controller (or 255 for all controllers). \see lcd_set_controller() \ingroup lcd_functions
uint8_t lcd_controller=0;
/// global variable that stores the last cursor mode \see lcd_set_cursor() \ingroup lcd_functions
uint8_t lcd_cursor_mode=0;

/** \defgroup lowlevelio Low-level IO functions
* Edit these functions (and possibly lcd_write()) to implement your own way of interfacing the LCD.
* @{
*/
/** Set data outputs
*
* In 4bit mode, this sets D4-D7 to bits 0-3 of the input value. In 8bit mode (not yet implemented!), this sets D0-D7 to the input value.
*/

#ifdef LCD_MODE_8BIT
	#error SHIFT_8BIT NOT YET IMPLEMENTED
#endif
#ifdef LCD_FREE_DATA_LINES
#ifndef LCD_DATA_DDR
	#error: LCD_DATA_DDR not defined
#endif
#ifndef LCD_DATA_PORT
	#error: LCD_DATA_PORT not defined
#endif
/// Rechts- oder Linksschieben je nach Vorzeichen
inline uint8_t lcd_shift(uint8_t data, int8_t pos) {
	if (pos < 0) {
		return data >> (-pos);
	} else {
		return data << pos;
	}
}

inline void LCD_DATA_OUT(uint8_t data) {
	uint8_t loeschMaske = ~((1<<LCD_D4_PIN) | (1<<LCD_D5_PIN) | (1<<LCD_D6_PIN) | (1<<LCD_D7_PIN));
#warning DEBUG
// uint8_t schreibMaske = (((data & (1<<0)) << LCD_D4_PIN) | (((data & (1<<1)) << (LCD_D5_PIN - 1))) | (((data & (1<<2)) << (LCD_D6_PIN - 2))) | (((data & (1<<3)) << (LCD_D7_PIN - 3))));
	uint8_t schreibMaske = lcd_shift(data & (1<<0),LCD_D4_PIN) | lcd_shift(data & (1<<1),LCD_D5_PIN - 1) | lcd_shift(data & (1<<2),LCD_D6_PIN - 2) | lcd_shift(data & (1<<3),LCD_D7_PIN - 3);
	LCD_DATA_PORT = (LCD_DATA_PORT & loeschMaske) | schreibMaske;
}
#else // normal operation in 4bit mode
inline void LCD_DATA_OUT(uint8_t data) {
	LCD_OUT=(LCD_OUT&(~(0x0F<<LCD_D4_PIN))) | (((data)&0x0F)<<LCD_D4_PIN);
	LCD_OUT_UPDATE();
}
#endif

/** Set the RS output pin

RS
*/

inline void LCD_RS(uint8_t rs) {
	if (rs) {
		LCD_RS_OUT|=(1<<LCD_RS_PIN);
	} else {
		LCD_RS_OUT&=~(1<<LCD_RS_PIN);
	}
	LCD_OUT_UPDATE();
}

/** Set the EN output pin

 "EN" (Enable) is the clock pin of the HD44780
 */
inline void LCD_EN(uint8_t en) {
// 	lcd_controller=1;
#ifdef LCD_TWO_CONTROLLERS
	if (((lcd_controller==0) || (lcd_controller==255)) && en) {
		LCD_EN_OUT|=(1<<LCD_EN_PIN);
	} else {
		LCD_EN_OUT&=~(1<<LCD_EN_PIN);
	}
#else
	if (en) {
		LCD_EN_OUT|=(1<<LCD_EN_PIN);
	} else {
		LCD_EN_OUT&=~(1<<LCD_EN_PIN);
	}
#endif

#ifdef LCD_TWO_CONTROLLERS
	if (((lcd_controller==1) || (lcd_controller==255)) && en) {
		LCD_EN2_OUT|=(1<<LCD_EN2_PIN);
	} else {
		LCD_EN2_OUT&=~(1<<LCD_EN2_PIN);
	}
#endif
	LCD_OUT_UPDATE();
}

/** Update output port from buffer

This does nothing for direct output. For shift registers it sends the (modified) output buffer to the shift register.
You can control inlining of this function by defining LCD_OUT_UPDATE_NEVER_INLINE (less size but less performance) or LCD_OUT_UPDATE_ALWAYS_INLINE (more size and more performance). By default it is inlined for all cases except LCD_MODE_SHIFT_4BIT and LCD_MODE_SHIFT_8BIT.
 */

// inline LCD_OUT_UPDATE for all trivial cases (direct output)
#if !defined(DOXYGEN) && ( (!defined LCD_MODE_SHIFT_4BIT && !defined LCD_MODE_SHIFT_8BIT && !defined LCD_OUT_UPDATE_NEVER_INLINE) || defined LCD_OUT_UPDATE_ALWAYS_INLINE )
inline /* inline the following function: */
#endif
void LCD_OUT_UPDATE(void) {
#if defined(LCD_MODE_SHIFT_4BIT)
	// 4bit output with shift register
	uint8_t i;
	for (i=0;i<8;i++) {
		if (LCD_OUT&(1<<(7-i))) {
			LCD_SHIFT_PORT|=(1<<LCD_SHIFT_DATA_PIN);
		} else {
			LCD_SHIFT_PORT&=~(1<<LCD_SHIFT_DATA_PIN);
		}
		_delay_us(1);
		LCD_SHIFT_PORT&=~(1<<LCD_SHIFT_CLOCK_PIN);
		_delay_us(1);
		LCD_SHIFT_PORT|=1<<LCD_SHIFT_CLOCK_PIN;
		_delay_us(1);
	}
	LCD_SHIFT_PORT&=~(1<<LCD_SHIFT_LATCH_PIN);
	_delay_us(1);
	LCD_SHIFT_PORT|=1<<LCD_SHIFT_LATCH_PIN;
	_delay_us(1);
#elif defined(LCD_MODE_SHIFT_8BIT)
	// 8bit output with shift registers
#error SHIFT_8BIT NOT YET IMPLEMENTED
#else
	// do nothing for diect output
#endif
}

/*@}*/


/** \defgroup lcd_functions Library Functions

These functions are used for accessing the display.
@{
*/



/// Send a raw command and wait for the execution time (3ms for home and clear, 100µs for all other commands)
void lcd_command(uint8_t data) {
	lcd_write(data,0);
	// execution time 37us
	_delay_us(100);
	if ((data==LCD_HOME) || (data==LCD_CLEAR)) {
		// execution time 1.52ms
		_delay_ms(3);
	}
}

/// Send data and wait 50µs
void lcd_data(uint8_t data) {
	lcd_write(data,1);
	// execution time 37+4 us
	_delay_us(100);
}

/** Write data or commands to the LCD

Edit this function to add your own way of 8 interfacing the LCD in 8-bit-mode (e.g. by shift register).
In 4-bit-mode this calls lcd_nibble() to send the half-bytes. */
void lcd_write(uint8_t data, uint8_t rs) {
	// RS
	LCD_RS(rs);

	// address setup time 60ns
	_delay_us(0.1);
	lcd_nibble(data>>4);

	lcd_nibble(data&0x0F);
}

/**  Write a half-byte in 4-bit-mode to the LCD */
void lcd_nibble(uint8_t data) {
	// Send a nibble in 4bit mode
	LCD_DATA_OUT(data);
	_delay_us(10); // data setup time 80ns, enable pulse width 230ns
	LCD_EN(1);
	_delay_us(10);
	LCD_EN(0);
	_delay_us(10);
	// data hold time 10ns
}



/** Initialise the LCD

Call this function before using the LCD and 100ms after the supply voltage has become stable. */
void lcd_init(void) {
	lcd_set_controller(LCD_ALL_CONTROLLERS);
	LCD_EN(0);
	lcd_position=0;
	LCD_RS(0);
#if defined(LCD_MODE_SHIFT_4BIT) || defined(LCD_MODE_SHIFT_8BIT)
	LCD_SHIFT_DDR|=(1<<LCD_SHIFT_DATA_PIN)|(1<<LCD_SHIFT_LATCH_PIN)|(1<<LCD_SHIFT_CLOCK_PIN);
	LCD_SHIFT_PORT|=(1<<LCD_SHIFT_LATCH_PIN) | (1<<LCD_SHIFT_CLOCK_PIN);
	// all unused shift pins are automatically set to 0 because this is the buffer's start value
#else
#ifdef LCD_FREE_DATA_LINES
	LCD_DATA_DDR |= (1<<LCD_D4_PIN)|(1<<LCD_D5_PIN)|(1<<LCD_D6_PIN)|(1<<LCD_D7_PIN); // all data-pins initialised... (only one PORT allowed!!)
	LCD_RS_DDR |= (1<<LCD_RS_PIN);
	LCD_EN_DDR |= (1<<LCD_EN_PIN);
#else
	LCD_DDR |= (1<<LCD_D4_PIN)|(1<<(LCD_D4_PIN+1))|(1<<(LCD_D4_PIN+2))|(1<<(LCD_D4_PIN+3));
	LCD_RS_DDR |= (1<<LCD_RS_PIN);
	LCD_EN_DDR |= (1<<LCD_EN_PIN);
#endif
#ifdef LCD_RW_PIN
	LCD_DDR|=(1<<LCD_RW_PIN);
	LCD_PORT&=~(1<<LCD_RW_PIN);
#endif /* LCD_RW_PIN */

#ifdef LCD_TWO_CONTROLLERS
	LCD_EN2_DDR|=(1<<LCD_EN2_PIN);
#endif /* LCD_TWO_CONTROLLERS */

#endif
	// See HD44780U datasheet: "Initializing by instruction, 4bit mode"
	// Set interface to 8bit three times
	uint8_t i;
	for (i=0;i<3;i++) {
		lcd_nibble((LCD_FUNCTION|LCD_FUNCTION_8BIT)>>4);
		_delay_ms(10);
	}
	_delay_us(10);

	// Switch to 4bit mode (display is still in 8bit mode during this command)
	lcd_nibble((LCD_FUNCTION|LCD_FUNCTION_4BIT)>>4);

// 	// First command
// 	lcd_nibble((LCD_FUNCTION|LCD_FUNCTION_4BIT)>>4);
// 	lcd_nibble((LCD_FUNCTION|LCD_FUNCTION_4BIT|LCD_FUNCTION_2LINES|LCD_FUNCTION_5x10)&0x0f);

	// Now the display is mostly initialised and the real configuration can be set
	uint8_t init_command=LCD_FUNCTION|LCD_FUNCTION_4BIT;
#if defined(LCD_5x10_PIXEL_CHARS)
	init_command |= LCD_FUNCTION_5x10;
#else
	init_command |= LCD_FUNCTION_5x8;
#endif
	uint8_t lines_per_controller=LCD_LINES;
#if defined(LCD_HALF_LINES)
	// two "half" lines are one real line
	lines_per_controller/=2;
#endif
#if defined LCD_TWO_CONTROLLERS
	// each controller has half of all lines
	lines_per_controller/=2;
#endif
	/// \todo automatisches Setzen von 2LINES austesten
	if (lines_per_controller==2) {
		init_command|=LCD_FUNCTION_2LINES;
	}
	lcd_command(LCD_ENTRYMODE|LCD_ENTRYMODE_INCREMENT);
	lcd_clear(); // clear and go to home position
	lcd_home(); // reset any shift-options
	lcd_set_cursor(0); // LCD_ON_CURSOR_OFF
	lcd_set_controller(0);
}

/** Go to the start position (takes 3ms to execute)
* This also resets some shift-options that can be set by special LCD commands.
*/

void lcd_home(void) {
#ifdef LCD_TWO_CONTROLLERS
	lcd_set_controller(1);
	lcd_command(LCD_HOME); // home also resets some shift options
	lcd_set_controller(0);
#endif
	lcd_position=0;
	lcd_command(LCD_HOME);
#ifdef LCD_TWO_CONTROLLERS
	// call lcd_set_cursor() to switch off cursor on inactive controller for two-controller displays
	lcd_set_cursor(lcd_cursor_mode);
#endif
}

/** (internal function) Select the controller

@param num The controller number (counted from zero) or LCD_ALL_CONTROLLERS. */
#ifdef LCD_TWO_CONTROLLERS
inline void lcd_set_controller(uint8_t num) {
	lcd_controller=num;
// 	lcd_controller=LCD_ALL_CONTROLLERS;
}
#else
// attribute "unused" to suppress unused-parameter warnings
inline void lcd_set_controller(uint8_t __attribute__((unused)) num) {}
#endif

/// Clear the LCD (takes 3ms to execute) and go back to the start position
/// For two-controller displays: switch back to the first controller after this
void lcd_clear(void) {
	lcd_set_controller(LCD_ALL_CONTROLLERS);
	lcd_command(LCD_CLEAR);
	lcd_set_position(0);
}

/// Go to the start of the specified line
void lcd_set_line(uint8_t x) {
	lcd_set_position(x*LCD_COLS);
}

/** Go to the specified position

The position is stored in the global variable lcd_position. The command LCD_SET_DDRAM_ADDR then sets the position in the LCD controller. This also affects the cursor.
@param position The character number (Example: for a 4x20 character display the third line starts at 40)
*/
void lcd_set_position(uint8_t position) {
	if (position>LCD_LINES*LCD_COLS) {
		position=position%(LCD_LINES*LCD_COLS);
	}
	lcd_position=position;
	uint8_t line=lcd_position/LCD_COLS;
#ifdef LCD_HALF_LINES
	line=line/2;
#endif
#ifdef LCD_TWO_CONTROLLERS
	uint8_t lcd_old_controller=lcd_controller;
	// two-controller displays must have at least two lines!
	if (line<LCD_LINES/2) {
		lcd_set_controller(0);
	} else {
		lcd_set_controller(1);
	}
	if (lcd_controller != lcd_old_controller) {
		// call lcd_set_cursor() to switch off cursor on inactive controller for two-controller displays
		lcd_set_cursor(lcd_cursor_mode);
	}
#endif
#ifdef LCD_SPECIAL_LINE_LAYOUT
	uint8_t line_addr;
	if (line==0) {
		line_addr=LCD_LINE1_ADDR;
	} else if (line==1) {
		line_addr=LCD_LINE2_ADDR;
	} else if (line==2) {
		line_addr=LCD_LINE3_ADDR;
	} else if (line==3) {
		line_addr=LCD_LINE4_ADDR;
	}
#else
	uint8_t line_addr=(line%2)*0x40;
#endif
	lcd_command(LCD_SET_DDRAM_ADDR|(line_addr+lcd_position%LCD_COLS));
}

/// Cursor Mode: No cursor
#define LCD_ON_CURSOR_OFF 0
/// Cursor Mode: Cursor on (underline the current character)
#define LCD_ON_CURSOR_ON 1
/// Cursor Mode: Blink the current character
#define LCD_ON_CURSOR_BLINK 2
/// Cursor Mode: Cursor and Blink (underline and blink the current character)
#define LCD_ON_CURSOR_AND_BLINK 3
/// Cursor Mode: Display completely off
#define LCD_OFF 4




/** Set the cursor mode, switches the LCD on or off

@param mode LCD_ON_CURSOR_OFF, LCD_ON_CURSOR_ON, LCD_ON_CURSOR_BLINK or LCD_CURSOR_AND_BLINK
For two-controller LCDs: The cursor is only enabled for the current controller and disabled for the other controller. As soon as the cursor reaches the second display, the cursor is disabled on the first display and enabled on the second: When a controller change is done by lcd_set_position(), it calls lcd_set_cursor() again with the current mode, which is stored in the global variable lcd_cursor_mode. lcd_set_position is called automatically through lcd_linewrap() by lcd_putchar(), lcd_print() and lcd_putstr().
\todo test the new version of this function, especially two-controller cursor handling
*/
void lcd_set_cursor(uint8_t mode) {
	lcd_cursor_mode=mode;
#ifdef LCD_TWO_CONTROLLERS
	uint8_t lcd_old_controller=lcd_controller;
#endif
	// Modes "on, no cursor" and "off" are for both controllers
	if (mode==LCD_ON_CURSOR_OFF) {
		lcd_set_controller(LCD_ALL_CONTROLLERS);
		lcd_command(LCD_CONTROL|LCD_CONTROL_DISPLAY_ON|LCD_CONTROL_CURSOR_OFF);
	} else if (mode==LCD_OFF) {
		lcd_set_controller(LCD_ALL_CONTROLLERS);
		lcd_command(LCD_CONTROL|LCD_CONTROL_DISPLAY_OFF);
	} else {
		// Mode "cursor enabled" is only for the active controller
		if (mode==LCD_ON_CURSOR_ON) {
			lcd_command(LCD_CONTROL|LCD_CONTROL_DISPLAY_ON|LCD_CONTROL_CURSOR_ON);
		} else if (mode==LCD_ON_CURSOR_BLINK) {
			lcd_command(LCD_CONTROL|LCD_CONTROL_DISPLAY_ON|LCD_CONTROL_CURSOR_OFF|LCD_CONTROL_BLINK);
		} else if (mode==LCD_ON_CURSOR_AND_BLINK) {
			lcd_command(LCD_CONTROL|LCD_CONTROL_DISPLAY_ON|LCD_CONTROL_CURSOR_ON|LCD_CONTROL_BLINK);
		}
#ifdef LCD_TWO_CONTROLLERS
		lcd_set_controller((lcd_controller+1)%2); // switch to inactive controller
		// disable the cursor for the inactive controller
		lcd_command(LCD_CONTROL|LCD_CONTROL_DISPLAY_ON|LCD_CONTROL_CURSOR_OFF);
#endif
	}
#ifdef LCD_TWO_CONTROLLERS
	lcd_set_controller(lcd_old_controller);
#endif
}

/** (internal function) If the cursor has passed the end of a line, set the position to the beginning of the next line

This internal function is used for automatic line wrapping. It is necessary because the memory addresses of a line are not always consecutive: A 2x8 character display has line 1 at address 0-7 and line 2 at address 40-47. */
void lcd_linewrap(void) {
	// Overflow: go back to the start
	if (lcd_position>=LCD_LINES*LCD_COLS) {
		lcd_position=0;
	}
	// start of new line reached, set position
	if (lcd_position%LCD_COLS==0) {
		lcd_set_position(lcd_position);
	}
}

/** Write a single char to the LCD at the current position
 *
 * This will print a single char and move to the next position (with automatic linewrapping).
 *
 * @param chr The character (even 0x00 is allowed and will be printed).
 */
void lcd_putchar(char chr) {
	lcd_data(chr);
	lcd_position++;
	lcd_linewrap();
}

/** Write a string starting at the current position
 *
 * This will print a string with automatic linewrapping. If the string is longer than the display, it will be displayed in multiple parts without a delay inbetween.
 *
 * @param str The string to be displayed (zero-terminated array of char)
 */
void lcd_putstr(char *str) {
	while (*str!=0) {
		lcd_putchar(*str);
		str++;
	}
}

/** Write a string from ROM starting at the current position
 *
 * This will print a string with automatic linewrapping. If the string is longer than the display, it will be displayed in multiple parts without a delay inbetween.
 *
 * @param str The string to be displayed (zero-terminated array of char in ROM)
 */
void lcd_putstr_P(PGM_P str) {
	while (pgm_read_byte(str)!=0) {
		lcd_putchar(pgm_read_byte(str));
		str++;
	}
}

/** Write a string after clearing the display
 *
 * This clears the display, and then writes a string starting at the home position.
 *
 * @param str The string to be displayed (zero-terminated array of char)
*/
void lcd_print(char *str) {
	lcd_clear();
	lcd_putstr(str);
}

/** Add a custom character

This writes a custom character into character generator RAM. For displays with 5x8 pixel characters (the default), eight characters consisting of eight lines each can be stored. The eigth line is the cursor position which should be kept empty. Displays with 5x10 pixel characters can only store four characters consisting of eleven lines where the eleventh line is the cursor position.

@param number The number can be between 0 and 7 (or 0-3 for 5x10 char displays). The saved character can be accessed by this number: You can include your characters in a string like "hello\001".
Due to technical limitations in C the character number zero can currently not really be used - 0x00 is the string terminating character so it cannot be included into strings. It can only be printed using lcd_putchar(0).

@param data The character is supplied as array of uint8_t. Each line is one byte, bit 4 is the leftmost pixel and bit 0 is the rightmost pixel. The top line is the first byte.

Example for 5x8 pixel character: This displays a black outlined box (the eight line is the cursor position).
@code
uint8_t character_box[]= {
0b00011111,
0b00010001,
0b00010001,
0b00010001,
0b00010001,
0b00010001,
0b00011111,
0b00000000 };
lcd_customchar(1,character_box);
lcd_print("This is a box: \001");
@endcode
 */
void lcd_customchar(uint8_t number, uint8_t *data) {
#ifdef LCD_TWO_CONTROLLERS
	uint8_t lcd_old_controller=lcd_controller;
	lcd_controller=LCD_ALL_CONTROLLERS;
#endif
#if defined LCD_5x10_PIXEL_CHARS
	lcd_command(LCD_SET_CGRAM_ADDR|number<<4);
	uint8_t bytes=11;
#else
	lcd_command(LCD_SET_CGRAM_ADDR|number<<3);
	uint8_t bytes=8;
#endif
	uint8_t i;
	for (i=0;i<bytes;i++) {
		lcd_data(*data);
		data++;
	}
#if defined LCD_5x10_PIXEL_CHARS
	for (i=0;i<5;i++) {
		lcd_data(0);
	}
	uint8_t bytes=11;
#endif
	lcd_command(LCD_SET_DDRAM_ADDR|0);
#ifdef LCD_TWO_CONTROLLERS
	lcd_controller=lcd_old_controller;
#endif
	lcd_home();
}
/** Add a custom character from ROM
This is like lcd_customchar(), but the character is stored in flash ROM and does not consume RAM space.
\todo test this

Example:
@code
// this needs to be global (out of any function, out of main):
const PROGMEM uint8_t character_box[]= {
0b00011111,
0b00010001,
0b00010001,
0b00010001,
0b00010001,
0b00010001,
0b00011111,
0b00000000 };

// in the function:
lcd_customchar_P(1,character_box);
lcd_print("This is a box: \001");
@endcode
*/

void lcd_customchar_P(uint8_t number, const uint8_t * PROGMEM data) {
#ifdef LCD_TWO_CONTROLLERS
	uint8_t lcd_old_controller=lcd_controller;
	lcd_controller=LCD_ALL_CONTROLLERS;
#endif
#if defined LCD_5x10_PIXEL_CHARS
	lcd_command(LCD_SET_CGRAM_ADDR|number<<4);
	uint8_t bytes=11;
#else
	lcd_command(LCD_SET_CGRAM_ADDR|number<<3);
	uint8_t bytes=8;
#endif
	uint8_t i;
	for (i=0;i<bytes;i++) {
		lcd_data(pgm_read_byte(data));
		data++;
	}
#if defined LCD_5x10_PIXEL_CHARS
	for (i=0;i<5;i++) {
		lcd_data(0);
	}
	uint8_t bytes=11;
#endif
	lcd_command(LCD_SET_DDRAM_ADDR|0);
#ifdef LCD_TWO_CONTROLLERS
	lcd_controller=lcd_old_controller;
#endif
	lcd_home();
}

/*@}*/


