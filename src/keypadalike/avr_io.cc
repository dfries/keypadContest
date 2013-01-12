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

RegObj& RegObj::operator=(uint8_t value)
{
	g_ATtiny=RegValue(Reg, value);
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

RegObj::operator uint8_t()
{
	return g_ATtiny.GetValue(Reg);
}

// Unlike compiling for the ATtiny where it only needs this function if
// the compile time value is out of range and the function is needed,
// it won't link here when it is undefined.
uint8_t hz_is_not_valid(uint32_t hz)
{
	cerr << __func__ << " invalid " << hz << " value\n";
	return CLKPR;
}
