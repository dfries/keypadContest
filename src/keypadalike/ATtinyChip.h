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

#ifndef _AT_TINY_CHIP_H
#define _AT_TINY_CHIP_H

#include <avr/io.h>
#include <functional>

class HallKeypad;
class Timer0;
class Timer1;

/* This class keeps track of the ATtiny register states and requied
 * emulations.  Use the ATtiny class as a wrapper when accessing the
 * class from multiple threads.
 */
class ATtinyChip
{
public:
	ATtinyChip();
	void SetPeripheral(HallKeypad *keypad) { Keypad = keypad; }
	// It is using the operator syntax just to make it obvious what
	// operation they represent.
	const ATtinyChip& operator=(RegValue arg);
	const ATtinyChip& operator+=(RegValue arg);
	const ATtinyChip& operator-=(RegValue arg);
	const ATtinyChip& operator|=(RegValue arg);
	const ATtinyChip& operator&=(RegValue arg);
	const ATtinyChip& operator^=(RegValue arg);
	uint8_t GetValue(RegEnum reg);
private:
	// Allow all the various assignment operations to be a lambda callback
	// to have a common before and after callback.
	typedef std::function<void (uint8_t &v)> RegOperation;

	const ATtinyChip& Set(RegEnum reg, RegOperation op);

	uint8_t Reg[REG_SREG];
	HallKeypad *Keypad;
	Timer0 *TimerObj0;
	Timer1 *TimerObj1;
	uint32_t SystemClockHz;
};

#endif // _AT_TINY_CHIP_H
