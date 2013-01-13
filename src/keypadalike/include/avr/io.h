/*
This program allows limited support to compile and run programs written for
the Atmel ATTiny2313 to work like they were in a Hall Research KP2B keypad,
only compiled for a native desktop environment with a Qt GUI instead of
hardware buttons and LEDs.

    Copyright (C) Copyright 2012 David Fries.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _AVR_IO_H_
#define _AVR_IO_H_

#include <stdint.h>

#include "iotn2313.h"

// For source code compatibility rename main.  Don't include this from the
// program's main.
#ifndef AVR_MAIN
#define main avr_main
#define AVR_MAIN
#endif

#define _BV(bit) (1 << (bit))

// emulated registers
enum RegEnum
{
	REG_PIND=0x10,
	REG_DDRD=0x11,
	REG_PORTD=0x12,

	REG_PINB=0x16,
	REG_DDRB=0x17,
	REG_PORTB=0x18,

	REG_PINA=0x19,
	REG_DDRA=0x1A,
	REG_PORTA=0x1B,

	REG_CLKPR=0x26,

	// Timer 0
	REG_TCCR0A=0x30,
	REG_TCCR0B=0x33,
	REG_TCNT0=0x32,
	REG_OCR0A=0x36,
	REG_OCR0B=0x3C,
	REG_TIMSK=0x39,
	REG_TIFR=0x38,

	// status register (and also the last register)
	REG_SREG=0x3f

};

// used to allow C++ operator operations to have both the register and value
class RegValue
{
public:
	RegValue(RegEnum reg, uint8_t value) : Reg(reg), Value(value) {}
	RegEnum Reg;
	uint8_t Value;
};

/* The register object that gives source code compatibility for reading and
 * writing registers such as ports.
 */
class RegObj
{
public:
	RegObj(RegEnum reg) : Reg(reg) {}
	RegObj& operator=(uint8_t value);
	RegObj& operator|=(uint8_t value);
	RegObj& operator&=(uint8_t value);
	RegObj& operator^=(uint8_t value);
	// allow reading back as an integer
	operator uint8_t();
private:
	RegEnum Reg;
};

// ATtiny
extern RegObj PIND;
extern RegObj DDRD;
extern RegObj PORTD;

extern RegObj PINB;
extern RegObj DDRB;
extern RegObj PORTB;

extern RegObj PINA;
extern RegObj DDRA;
extern RegObj PORTA;

extern RegObj CLKPR;

// Timer 0
extern RegObj TCCR0A;
extern RegObj TCCR0B;
extern RegObj TCNT0;
extern RegObj OCR0A;
extern RegObj OCR0B;
extern RegObj TIMSK;
extern RegObj TIFR;

#endif // _AVR_IO_H_
