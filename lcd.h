/*
 *    Filename: lcd.h
 *     Version: 0.2.2
 * Description: HD44780 Display Library for ATMEL AVR
 *     License: Public Domain
 *
 *      Author: Copyright (C) Max Gaukler <development@maxgaukler.de>
 *        Date: 2010
 *
 * I, the copyright holder of this work, hereby release it into the public domain. This applies worldwide.
 * In case this is not legally possible:
 * I grant anyone the right to use this work for any purpose, without any conditions, unless such conditions are required by law.
 *
 */
// Until a good way is found, you have to include lcd.c, not lcd.h
#include <avr/pgmspace.h>
#include <avr/io.h>
#include <util/delay.h>
 
#ifndef _LCD_H_INCLUDED
#define _LCD_H_INCLUDED

// IO Helper macros
#ifndef cbi
#define cbi(sfr, bit)	(sfr &= ~_BV(bit))
#endif

#ifndef sbi
#define sbi(sfr, bit)	(sfr |= _BV(bit))
#endif


/** \mainpage HD44780 Display Library for Atmel AVR Microcontrollers

\section including-into-prog Including the library into your program

Just include the file lcd.c (not .h!) into your main program.
@code #include "./lcd/lcd.c" @endcode

\section features Features

As long as the interrupt does not change the port that the display is connected to, the library should be interrupt-safe.

\section interfacing Interfacing the Display to the Microcontroller
\subsection direct_4bit Direct Interfacing in 4-bit mode
 
 In 4-bit mode, 6 IO pins are needed. Data lines D4-D7, E and RS are connected to the microcontroller. RW can be directly connected to ground. If you already connected RW to the microcontroller, define LCD_RW_PIN and it will automatically be set to low. D0-D3 can be left unconnected. 4-bit-mode needs only about 1 microsecond longer per command than 8-bit-mode, but 8-bit-mode needs four more IO pins.

This mode is selected by defining LCD_MODE_4BIT. LCD_PORT, LCD_DDR, LCD_EN_PIN, LCD_RS_PIN and LCD_D4_PIN need to be defined. D4,D5,D6,D7 are connected to LCD_D4_PIN, LCD_D4_PIN+1, and so on.
If you want to connect RS and EN to different ports than the data bits, define LCD_EN_PORT, LCD_EN_DDR, (for two-controller LCDs also LCD_EN2_PORT and LCD_EN2_DDR),  LCD_RS_PORT and LCD_RS_DDR.

Example:
@code
#define LCD_MODE_4BIT
#define LCD_PORT PORTA
#define LCD_DDR DDRA
#define LCD_EN_PIN PA4
#define LCD_RS_PIN PA5
// D4 -> PA0, D5 -> PA1, D6 -> PA2, D7 -> PA3
#define LCD_D4_PIN PA0
// optional:
#define LCD_RW_PIN PA6
@endcode

\subsection shiftregister_4bit Shift Register in 4-bit-mode

Only three IO pins are needed for shift register mode. These are connected to a shift register (74(V)HC(T)595 or similar, 8 bit, serial input, parallel output with latch).

This mode is selected by defining LCD_MODE_SHIFT_4BIT. LCD_SHIFT_PORT, LCD_SHIFT_DDR, LCD_SHIFT_DATA_PIN, LCD_SHIFT_CLOCK_PIN, LCD_SHIFT_LATCH_PIN define the connections to the shift register. For 595-style shift registers, clock is SCK (shift clock), latch is RCK (register clock) and data is D. Set LCD_EN_PIN, LCD_RS_PIN and LCD_D4_PIN to the pin numbers at the shift register. D4, D5, D6 and D7 of the LCD are connected to output LCD_D4_PIN, LCD_D4_pin+1, and so on.

\section display_conf Display Size Configuration

The display size is set with LCD_COLS and LCD_LINES. For most displays, this is the real value of columns and lines.  Usually each line can be a maximum of 40 characters wide, a display may be a maximum of 4 lines high. Displays with more than two lines or non-standard displays require special configuration, which is described in the next sections.

\subsection one_ctrl_more_lines One-Controller Displays with "half" lines

Some displays behave as if they were twice as wide and half as high. They just display every long internal line as two lines. For such displays, use the real value for columns and lines, and define LCD_HALF_LINES. In this mode a display may be up to 8 lines high, but only up to 20 characters wide.

The only difference between using this mode and setting double width and half height is that lcd_set_line works correctly.

\subsection one_ctrl_four_lines Displays with special memory layout

Some displays have a memory layout that doesn't follow the default of line 1 = 0x00, line 2 = 0x40. A different memory layout can be used by defining LCD_SPECIAL_LINE_LAYOUT and LCD_LINE1_ADDR to LCD_LINE4_ADDR. The typical layout for 4x16 character displays with only one controller is 0x00, 0x40, 0x10, 0x50. If you configured such a display as 2x32 characters (or 4x16 with LCD_HALF_LINES), the second and third line would be swapped.

Example:
@code
#define LCD_SPECIAL_LINE_LAYOUT
#define LCD_LINE1_ADDR 0x00
#define LCD_LINE2_ADDR 0x40
#define LCD_LINE3_ADDR 0x10
#define LCD_LINE4_ADDR 0x50
@endcode

\subsection two_ctrl_display Two-Controller Displays with four lines

If a display has two controllers, it has two separate EN pins. You need to define the EN2 pin number with LCD_EN2_PIN. Such a display has to be configured for at least two lines. If you really have a one-line display that is more than 80 characters wide, configure it as two lines high and twice as wide.

@code
#define LCD_TWO_CONTROLLERS
@endcode

 */
 
 
 /** \defgroup raw_commands Raw Commands sent to the controller
 
 These constants are used when communicating directly with the controller with lcd_command(). The bit names in the description refer to the Hitachi HD44780U datasheet. Usually calling these commands from your code should not be necessary. If you use them, be careful that the LCD position should always match the internal variable lcd_position. If the position was moved, call lcd_set_position() to go to your new position, lcd_home(), go back to the old position with  @code lcd_set_position(lcd_position);@endcode, or set @code lcd_position=your_new_known_position;@endcode (faster, does not send command to the LCD)
  */
 
/*@{*/
/// Clear: clear display and go to position 0
#define LCD_CLEAR 0x01

/// Home: go to position 0
#define LCD_HOME 0x02

/// <b>Entry Mode</b>: set cursor and display movement
#define LCD_ENTRYMODE 0x04
/// Option: I/D=1: auto-increment (left-to-right) of character position
#define LCD_ENTRYMODE_INCREMENT 0x02
/// Option: I/D=0: auto-decrement (right-to-left)
/// right-to-left is not really supported by this library yet, its internal position will not match the LCD position in this setting, it may cause trouble
#define LCD_ENTRYMODE_DECREMENT 0x00

/// Option: S: shift: the cursor does not move, but the display does
/// This is currently unsupported, automatic linebreak may do wrong things
/// The shifting does not work across line borders
#define LCD_ENTRYMODE_SHIFT 0x01

/// <b>Display On/Off Control</b> - used by lcd_set_cursor()
#define LCD_CONTROL 0x08
/// Option: D=1: display on
#define LCD_CONTROL_DISPLAY_ON 0x04
/// Option: D=0: display off (clear)
#define LCD_CONTROL_DISPLAY_OFF 0x00
/// Option: C=1: display cursor
#define LCD_CONTROL_CURSOR_ON 0x02
/// Option: C=0: no cursor
#define LCD_CONTROL_CURSOR_OFF 0x00
/// Option: B=1: blink current character
#define LCD_CONTROL_BLINK 0x01

/// <b>Cursor (or Display) shift</b>: move cursor or display content - please call lcd_set_position() after moving the cursor with this command, not really supported
#define LCD_SHIFT 0x10
/// Option: S/C=1: move cursor position
#define LCD_SHIFT_DISPLAY 0x08
/// Option: S/C=0: move display content, cursor stays at the same position (not really supported)
#define LCD_SHIFT_CURSOR 0x00
/// Option: R/L=1: right
#define LCD_SHIFT_RIGHT 0x04
/// Option: R/L=0: left
#define LCD_SHIFT_LEFT 0x00

/// <b>Function Set</b>: Initialize the Controllers - used by lcd_init()
#define LCD_FUNCTION 0x20
/// Option: DL=1: interface data length 8bit
#define LCD_FUNCTION_8BIT 0x10
/// Option: DL=0: interface data length 4bit
#define LCD_FUNCTION_4BIT 0x00
/// Option: N=1: two display lines (per controller)
#define LCD_FUNCTION_2LINES 0x08
/// Option: N=0: one display line (per controller)
#define LCD_FUNCTION_1LINE 0x00
/// Option: F=1: character font
#define LCD_FUNCTION_5x10 0x04
/// Option: F=0: character font
#define LCD_FUNCTION_5x8 0x00

/// <b>Set Character Generator RAM Address</b>
#define LCD_SET_CGRAM_ADDR 0x40

/// <b>Set Display Data RAM Address</b> (LCD Position) - used by lcd_set_position(), lcd_set_line()
#define LCD_SET_DDRAM_ADDR 0x80
 /*@}*/



#define LCD_ALL_CONTROLLERS 255


#if defined(LCD_MODE_4BIT) && ( !defined(LCD_PORT) || !defined(LCD_DDR) || !defined(LCD_EN_PIN) || !defined(LCD_RS_PIN) || !defined(LCD_D4_PIN) || !defined(LCD_LINES) || !defined(LCD_COLS))
#error for LCD_MODE_4BIT the constants LCD_PORT, LCD_DDR, LCD_EN_PIN, LCD_RS_PIN, LCD_D4_PIN, LCD_LINES, LCD_COLS need to be defined
#endif


#if defined(LCD_MODE_SHIFT_4BIT) && ( !defined(LCD_SHIFT_PORT) || !defined(LCD_SHIFT_DDR) || !defined(LCD_SHIFT_DATA_PIN) || !defined(LCD_SHIFT_CLOCK_PIN) || !defined(LCD_SHIFT_LATCH_PIN) || !defined(LCD_EN_PIN) || !defined(LCD_RS_PIN) || !defined(LCD_D4_PIN) || !defined(LCD_LINES) || !defined(LCD_COLS))
#error for LCD_MODE_SHIFT_4BIT the constants LCD_SHIFT_PORT, LCD_SHIFT_DDR, LCD_SHIFT_DATA_PIN, LCD_SHIFT_CLOCK_PIN, LCD_SHIFT_LATCH_PIN, LCD_EN_PIN, LCD_RS_PIN, LCD_D4_PIN, LCD_LINES, LCD_COLS need to be defined
#endif

#if !defined(LCD_MODE_4BIT) && !defined(LCD_MODE_8BIT) && !defined(LCD_MODE_SHIFT_4BIT)
#error please define LCD interface mode (currently only LCD_MODE_4BIT and LCD_MODE_SHIFT_4BIT is supported)
#endif
// TODO: planned: 8bit, 8bit with two shift registers soft-spi (3 pins)


#if (LCD_LINES>2) && !defined(LCD_TWO_CONTROLLERS) && !defined(LCD_SPECIAL_LINE_LAYOUT)
#error LCDs with more than two lines usually need two controllers. If there is a second LCD_EN pin, define LCD_TWO_CONTROLLERS and LCD_EN2_PIN. If there is no second LCD_EN_PIN, try using double LCD_COLS and half LCD_LINES. Otherwise, try defining a memory layout with LCD_SPECIAL_LINE_LAYOUT. (see documentation)
#endif

#if defined(LCD_SPECIAL_LINE_LAYOUT) && ( !defined(LCD_LINE1_ADDR) || !defined(LCD_LINE2_ADDR) || !defined(LCD_LINE3_ADDR) || !defined(LCD_LINE4_ADDR) )
#error For LCD_SPECIAL_LINE_LAYOUT you need to define LCD_LINE1_ADDR, LCD_LINE2_ADDR, LCD_LINE3_ADDR and LCD_LINE4_ADDR. If you have less than four lines, simply define the unneeded constans as 0.
#endif

#if defined(LCD_SPECIAL_LINE_LAYOUT) && ( (!defined(LCD_HALF_LINES) && (LCD_LINES>4)) || (defined(LCD_HALF_LINES) && (LCD_LINES>8)))
#error LCD_SPECIAL_LINE_LAYOUT only works for up to 4 lines (or up to 8 "half" lines with LCD_HALF_LINES)
#endif

#if (LCD_LINES*LCD_COLS>80) && !defined(LCD_TWO_CONTROLLERS)
#error There can be maximum 80 characters per controller, you should define LCD_TWO_CONTROLLERS and LCD_EN2_PIN.
#endif

#if defined(LCD_TWO_CONTROLLERS) && !defined(LCD_EN2_PIN)
#error For LCD_TWO_CONTROLLERS you need LCD_EN2_PIN
#endif


#if defined(LCD_MODE_4BIT) || defined(LCD_MODE_8BIT)
	#ifdef LCD_RS_PORT
		#define LCD_RS_OUT LCD_RS_PORT
		#ifndef LCD_RS_DDR
			#error please define both of LCD_RS_PORT and LCD_RS_DDR or none of them
		#endif
	#else
		#define LCD_RS_PORT LCD_PORT
		#define LCD_RS_DDR LCD_DDR
	#endif
	#ifdef LCD_EN_PORT
		#ifndef LCD_EN_DDR
			#error please define both of LCD_EN_PORT and LCD_EN_DDR or none of them
		#endif
		#if defined(LCD_TWO_CONTROLLERS) && (!defined(LCD_EN2_PORT) || !defined(LCD_EN2_DDR))
			#error please also define LCD_EN2_PORT and LCD_EN2_DDR
		#endif
	#else
		#define LCD_EN_PORT LCD_PORT
		#define LCD_EN2_PORT LCD_PORT
		#define LCD_EN_DDR LCD_DDR
		#define LCD_EN2_DDR LCD_DDR
	#endif
	
	#define LCD_OUT LCD_PORT
	#define LCD_EN_OUT LCD_EN_PORT
	#define LCD_RS_OUT LCD_RS_PORT
	#define LCD_EN2_OUT LCD_EN2_PORT
#else
	// buffer for shift-register output
	uint8_t lcd_out_buf=0;
	#define LCD_OUT lcd_out_buf
#endif

#ifndef LCD_RS_OUT
	#define LCD_RS_OUT LCD_OUT
#endif
#ifndef LCD_EN_OUT
	#define LCD_EN_OUT LCD_OUT
#endif
#ifndef LCD_EN2_OUT
	#define LCD_EN2_OUT LCD_OUT
#endif
// Prototypes created with cat lcd.c | sed -n "s/^\([a-z].*(.*)\)[ \\t]*[\{][^;]*\$/\1;/p"
inline void LCD_DATA_OUT(uint8_t data);
inline void LCD_RS(uint8_t rs);
inline void LCD_EN(uint8_t en);
void LCD_OUT_UPDATE(void);
void lcd_command(uint8_t data);
void lcd_data(uint8_t data);
void lcd_write(uint8_t data, uint8_t rs);
void lcd_nibble(uint8_t data);
void lcd_init(void);
void lcd_home(void);
void lcd_set_controller(uint8_t num);
void lcd_clear(void);
void lcd_set_line(uint8_t x);
void lcd_set_position(uint8_t position);
void lcd_set_cursor(uint8_t mode);
void lcd_linewrap(void);
void lcd_putchar(char chr);
void lcd_putstr(char *str);
void lcd_putstr_P(PGM_P str);
void lcd_print(char *str);
void lcd_customchar(uint8_t number, uint8_t *data);
void lcd_customchar_P(uint8_t number, const uint8_t * PROGMEM data);
#endif // _LCD_H_INCLUDED