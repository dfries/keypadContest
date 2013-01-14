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

#ifndef _TIMER_1_H
#define _TIMER_1_H

#include "Timer.h"
#include <avr/io.h>

/* deals with the registers specific to the 16 bit timer/counter 1
 */
class Timer1 : public Timer
{
public:
	/* reg is a pointer to the current register values. */
	Timer1(const uint8_t *reg);
	/* The ATtiny is an 8 bit microcontroller, all register writes are
	 * 8 bit, even to 16 bit registers.  Write to the high byte (which will
	 * go into the register array), then the low byte (which will combine
	 * the two and do the operation).
	 */
	virtual void Set(RegEnum reg, uint8_t value);
	/* Like read, only in reverse for 16 bit registers.  Read the low
	 * byte (which will read the 16 bit value and return the low byte),
	 * then read the high byte to the the previous read's high byte
	 * value.
	 */
	virtual uint8_t Get(RegEnum reg);
protected:
	virtual void UpdateSleep();
};

#endif // _TIMER_1_H
