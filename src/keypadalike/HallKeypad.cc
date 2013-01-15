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

#include "HallKeypad.h"
#include <iostream>
#include <QMutexLocker>

using namespace std;

HallKeypad::HallKeypad() :
	Buttons(0xffff),
	LEDs(0),
	PortD(0),
	PortB(0)
{
}

void HallKeypad::SetPort(RegEnum reg, uint8_t value)
{
	QMutexLocker locker(&Mutex);
	if(reg == REG_PORTD)
	{
		PortD=value;
		UpdateLEDs();

		Audio.SetPins(value & _BV(PD1), value & _BV(PD6));
		// inputs are read from GetPort so skip any buttons enable bits
		return;
	}
	if(reg == REG_PORTB)
	{
		PortB=value;
		UpdateLEDs();
	}
	//if(reg == REG_PORTA) TODO
}

void HallKeypad::UpdateLEDs()
{
	uint16_t output=LEDs;
	if(PortD & _BV(PD2))
		output = (output &0xff00) | PortB;
	if(PortD & _BV(PD3))
		output = (output &0x00ff) | (PortB<<8);
	if(LEDs != output)
	{
		LEDs=output;
		SetLEDs(~LEDs);
	}
}

uint8_t HallKeypad::GetPort(RegEnum reg)
{
	QMutexLocker locker(&Mutex);
	if(reg == REG_PIND)
		return PortD;
	if(reg != REG_PINB)
		return 0xff & rand();
	uint8_t value=0;
	// button input "Output Enable" line is active low
	uint8_t invD=~PortD;
	if(invD & (_BV(PD4) | _BV(PD5)))
	{
		if(invD & _BV(PD4))
			value=Buttons;
		if(invD & _BV(PD5))
			value|=Buttons>>8;
	}
	else
	{
		cerr << "HallKeypad::GetPort read PORTB without input "
			"enabled\n";
		value=0xff & rand();
	}
	return value;
}

void HallKeypad::SetButtons(uint16_t buttons)
{
	QMutexLocker locker(&Mutex);
	// 0 for pressed, 1 for not pressed, invert
	Buttons=~buttons;
}
