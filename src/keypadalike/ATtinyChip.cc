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

#include "ATtinyChip.h"
#include "HallKeypad.h"

ATtinyChip::ATtinyChip()
{
	memset(Reg, 0, sizeof(Reg));
}

const ATtinyChip& ATtinyChip::operator=(RegValue arg)
{
	return Set(arg.Reg, [arg](uint8_t &v) { v=arg.Value; });
}

const ATtinyChip& ATtinyChip::operator|=(RegValue arg)
{
	return Set(arg.Reg, [arg](uint8_t &v) { v|=arg.Value; });
}

const ATtinyChip& ATtinyChip::operator&=(RegValue arg)
{
	return Set(arg.Reg, [arg](uint8_t &v) { v&=arg.Value; });
}

const ATtinyChip& ATtinyChip::Set(RegEnum reg, RegOperation op)
{
	uint8_t v=Reg[reg];
	uint8_t copy=v;
	op(v);
	if(v==copy)
		return *this;
	Reg[reg]=v;
	switch(reg)
	{
	case REG_PORTD:
	case REG_PORTB:
	case REG_PORTA:
		Keypad->SetPort(reg, v);
		break;
	default:
		break;
	}
	return *this;
}

uint8_t ATtinyChip::GetValue(RegEnum reg)
{
	return Reg[reg];
}
