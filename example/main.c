/*
 *    Filename: main.c
 *     Version: 0.1
 * Description: Example for HD44780 Display Library for ATMEL AVR
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
#define LCD_PORT PORTB
#define LCD_DDR DDRB
#define LCD_RS_PIN PB5
#define LCD_EN_PIN PB4
#define LCD_D4_PIN PB0
#define LCD_COLS 16
#define LCD_LINES 2
#define LCD_MODE_4BIT
#include <avr/io.h>
#include "../lcd.c"
int main(void) {
 	lcd_init();
	lcd_putstr_P(PSTR("Hello"));
}