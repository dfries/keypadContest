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

#include <iostream>
#include <avr/io.h>
#include "ATtiny.h"

using namespace std;

RegObj PIND(REG_PIND);
RegObj DDRD(REG_DDRD);
RegObj PORTD(REG_PORTD);

RegObj PINB(REG_PINB);
RegObj DDRB(REG_DDRB);
RegObj PORTB(REG_PORTB);

RegObj PINA(REG_PINA);
RegObj DDRA(REG_DDRA);
RegObj PORTA(REG_PORTA);

RegObj CLKPR(REG_CLKPR);

RegObj MCUSR(REG_MCUSR);
RegObj WDTCSR(REG_WDTCSR);

// Timer 0
RegObj TCCR0A(REG_TCCR0A);
RegObj TCCR0B(REG_TCCR0B);
RegObj TCNT0(REG_TCNT0);
RegObj OCR0A(REG_OCR0A);
RegObj OCR0B(REG_OCR0B);
RegObj TIMSK(REG_TIMSK);
RegObj TIFR(REG_TIFR);

RegObj TCCR1A(REG_TCCR1A);
RegObj TCCR1B(REG_TCCR1B);
RegObj TCCR1C(REG_TCCR1C);
RegObj16 TCNT1(REG_TCNT1);
RegObj TCNT1L(REG_TCNT1L);
RegObj TCNT1H(REG_TCNT1H);
RegObj16 OCR1A(REG_OCR1A);
RegObj OCR1AL(REG_OCR1AL);
RegObj OCR1AH(REG_OCR1AH);
RegObj16 OCR1B(REG_OCR1B);
RegObj OCR1BL(REG_OCR1BL);
RegObj OCR1BH(REG_OCR1BH);
RegObj16 ICR1(REG_ICR1);
RegObj ICR1L(REG_ICR1L);
RegObj ICR1H(REG_ICR1H);

RegObj_SREG SREG;

RegObj& RegObj::operator=(uint8_t value)
{
	g_ATtiny=RegValue(Reg, value);
	return *this;
}

RegObj& RegObj::operator+=(uint8_t value)
{
	g_ATtiny+=RegValue(Reg, value);
	return *this;
}

RegObj& RegObj::operator-=(uint8_t value)
{
	g_ATtiny-=RegValue(Reg, value);
	return *this;
}

RegObj& RegObj::operator|=(uint8_t value)
{
	g_ATtiny|=RegValue(Reg, value);
	return *this;
}

RegObj& RegObj::operator&=(uint8_t value)
{
	g_ATtiny&=RegValue(Reg, value);
	return *this;
}

RegObj& RegObj::operator^=(uint8_t value)
{
	g_ATtiny^=RegValue(Reg, value);
	return *this;
}

RegObj& RegObj::operator++()
{
	*this+=1;
	return *this;
}

RegObj& RegObj::operator--()
{
	*this-=1;
	return *this;
}

RegObj::operator uint8_t()
{
	return g_ATtiny.GetValue(Reg);
}

// enable or disable the interrupts when the value changes
RegObj_SREG& RegObj_SREG::operator=(uint8_t value)
{
	uint8_t before=g_ATtiny.GetValue(Reg);
	g_ATtiny=RegValue(Reg, value);
	uint8_t after=g_ATtiny.GetValue(Reg);
	if((before ^ after) & _BV(SREG_I))
		g_ATtiny.EnableInterrupts(after & _BV(SREG_I));
	return *this;
}

RegObj_SREG& RegObj_SREG::operator|=(uint8_t value)
{
	uint8_t before=g_ATtiny.GetValue(Reg);
	g_ATtiny|=RegValue(Reg, value);
	uint8_t after=g_ATtiny.GetValue(Reg);
	if((before ^ after) & _BV(SREG_I))
		g_ATtiny.EnableInterrupts(after & _BV(SREG_I));
	return *this;
}

RegObj_SREG& RegObj_SREG::operator&=(uint8_t value)
{
	uint8_t before=g_ATtiny.GetValue(Reg);
	g_ATtiny&=RegValue(Reg, value);
	uint8_t after=g_ATtiny.GetValue(Reg);
	if((before ^ after) & _BV(SREG_I))
		g_ATtiny.EnableInterrupts(after & _BV(SREG_I));
	return *this;
}

RegObj_SREG& RegObj_SREG::operator^=(uint8_t value)
{
	uint8_t before=g_ATtiny.GetValue(Reg);
	g_ATtiny^=RegValue(Reg, value);
	uint8_t after=g_ATtiny.GetValue(Reg);
	if((before ^ after) & _BV(SREG_I))
		g_ATtiny.EnableInterrupts(after & _BV(SREG_I));
	return *this;
}

RegObj_SREG::operator uint8_t()
{
	return g_ATtiny.GetValue(Reg);
}

/* From 16-bit Timer/Counter1 "Accessing 16-bit Registers"
 * "To do a 16-bit write, the high byte must be written before the low byte.
 * For a 16-bit read, the low byte must be read before the high byte."
 */
RegObj16& RegObj16::operator=(uint16_t value)
{
	g_ATtiny=RegValue(RegH, value>>8);
	g_ATtiny=RegValue(Reg, value);
	return *this;
}

RegObj16& RegObj16::operator+=(uint16_t value)
{
	// split into read, modify, write
	// So it is read low, read high, modify, write high, write low,
	// to execute the operations in the documented order.
	uint16_t v16=*this;
	v16+=value;
	*this=v16;
	return *this;
}

RegObj16& RegObj16::operator-=(uint16_t value)
{
	// split into read, modify, write
	// So it is read low, read high, modify, write high, write low,
	// to execute the operations in the documented order.
	uint16_t v16=*this;
	v16-=value;
	*this=v16;
	return *this;
}

RegObj16& RegObj16::operator|=(uint16_t value)
{
	// split into read, modify, write
	// So it is read low, read high, modify, write high, write low,
	// to execute the operations in the documented order.
	uint16_t v16=*this;
	v16|=value;
	*this=v16;
	return *this;
}

RegObj16& RegObj16::operator&=(uint16_t value)
{
	// split into read, modify, write
	// So it is read low, read high, modify, write high, write low,
	// to execute the operations in the documented order.
	uint16_t v16=*this;
	v16&=value;
	*this=v16;
	return *this;
}

RegObj16& RegObj16::operator^=(uint16_t value)
{
	// split into read, modify, write
	// So it is read low, read high, modify, write high, write low,
	// to execute the operations in the documented order.
	uint16_t v16=*this;
	v16^=value;
	*this=v16;
	return *this;
}

RegObj16& RegObj16::operator++()
{
	// split into read, modify, write
	// So it is read low, read high, modify, write high, write low,
	// to execute the operations in the documented order.
	uint16_t v16=*this;
	++v16;
	*this=v16;
	return *this;
}

RegObj16& RegObj16::operator--()
{
	// split into read, modify, write
	// So it is read low, read high, modify, write high, write low,
	// to execute the operations in the documented order.
	uint16_t v16=*this;
	--v16;
	*this=v16;
	return *this;
}

RegObj16::operator uint16_t()
{
	// read the low byte first as that stores the high byte in a temporary
	uint16_t value=g_ATtiny.GetValue(Reg);
	value |= (uint16_t)g_ATtiny.GetValue(RegH)<<8;
	return value;
}

// Unlike compiling for the ATtiny where it only needs this function if
// the compile time value is out of range and the function is needed,
// it won't link here when it is undefined.
uint8_t hz_is_not_valid(uint32_t hz)
{
	cerr << __func__ << " invalid " << hz << " value\n";
	return CLKPR;
}
