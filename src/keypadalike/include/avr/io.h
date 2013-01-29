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
#include "common.h"

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

	REG_MCUSR=0x34,
	REG_WDTCSR=0x21,

	// Timer 0
	REG_TCCR0A=0x30,
	REG_TCCR0B=0x33,
	REG_TCNT0=0x32,
	REG_OCR0A=0x36,
	REG_OCR0B=0x3C,
	// Note both Timer 0 and Timer 1 use different bits in TIMSK and TIFR
	REG_TIMSK=0x39,
	REG_TIFR=0x38,

	// Timer 1
	REG_TCCR1A=0x2F,
	REG_TCCR1B=0x2E,
	REG_TCCR1C=0x22,
	REG_TCNT1=0x2C, // 16 bit
	REG_TCNT1L=0x2C,
	REG_TCNT1H=0x2D,
	REG_OCR1A=0x2A, // 16 bit
	REG_OCR1AL=0x2A,
	REG_OCR1AH=0x2B,
	REG_OCR1B=0x28, // 16 bit
	REG_OCR1BL=0x28,
	REG_OCR1BH=0x29,
	REG_ICR1=0x24, // 16 bit
	REG_ICR1L=0x24,
	REG_ICR1H=0x25,
	// Note both Timer 0 and Timer 1 use different bits in TIMSK and TIFR
	/*
	REG_TIMSK=0x39,
	REG_TIFR=0x38,
	*/

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
	RegObj& operator+=(uint8_t value);
	RegObj& operator-=(uint8_t value);
	RegObj& operator|=(uint8_t value);
	RegObj& operator&=(uint8_t value);
	RegObj& operator^=(uint8_t value);
	RegObj& operator++();
	RegObj& operator--();
	// allow reading back as an integer
	operator uint8_t();
private:
	RegEnum Reg;
};

/* SREG holds the interrupt enable, which means it has to deal with
 * concurrency, but that can't be handled in ATtinyChip, it must be done
 * in ATtiny, so it is treated specially.
 */
class RegObj_SREG
{
public:
	RegObj_SREG() : Reg(REG_SREG) {}
	RegObj_SREG& operator=(uint8_t value);
	RegObj_SREG& operator+=(uint8_t value);
	RegObj_SREG& operator-=(uint8_t value);
	RegObj_SREG& operator|=(uint8_t value);
	RegObj_SREG& operator&=(uint8_t value);
	RegObj_SREG& operator^=(uint8_t value);
	RegObj_SREG& operator++();
	RegObj_SREG& operator--();
	// allow reading back as an integer
	operator uint8_t();
private:
	RegEnum Reg;
};

class RegObj16
{
public:
	RegObj16(RegEnum reg) : Reg(reg), RegH((RegEnum)(reg+1)) {}
	RegObj16& operator=(uint16_t value);
	RegObj16& operator+=(uint16_t value);
	RegObj16& operator-=(uint16_t value);
	RegObj16& operator|=(uint16_t value);
	RegObj16& operator&=(uint16_t value);
	RegObj16& operator^=(uint16_t value);
	RegObj16& operator++();
	RegObj16& operator--();
	// allow reading back as an integer
	operator uint16_t();
private:
	RegEnum Reg;
	RegEnum RegH;
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

extern RegObj MCUSR;
extern RegObj WDTCSR;

// Timer 0
extern RegObj TCCR0A;
extern RegObj TCCR0B;
extern RegObj TCNT0;
extern RegObj OCR0A;
extern RegObj OCR0B;
extern RegObj TIMSK;
extern RegObj TIFR;

// Timer 0
extern RegObj TCCR1A;
extern RegObj TCCR1B;
extern RegObj TCCR1C;
extern RegObj16 TCNT1;
extern RegObj TCNT1L;
extern RegObj TCNT1H;
extern RegObj16 OCR1A;
extern RegObj OCR1AL;
extern RegObj OCR1AH;
extern RegObj16 OCR1B;
extern RegObj OCR1BL;
extern RegObj OCR1BH;
extern RegObj16 ICR1;
extern RegObj ICR1L;
extern RegObj ICR1H;

extern RegObj_SREG SREG;

#endif // _AVR_IO_H_
