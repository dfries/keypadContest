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

#ifndef _HALL_KEYPAD_H
#define _HALL_KEYPAD_H

#include <QObject>
#include <QMutex>
#include <avr/io.h>
#include "SquareAudio.h"

/* Emulates the Hall Research KP2B keypad connections to the microcontroller
 * registers.
 */
class HallKeypad : public QObject
{
	Q_OBJECT
public:
	HallKeypad();
	// Only call with registers connected to output ports when the
	// direction is out.  Input pullup resistors aren't delt with right
	// now, so if the direction is input the bit will always be 0.
	void SetPort(RegEnum reg, uint8_t value);
	// Call to read from a port that is in input direction.
	uint8_t GetPort(RegEnum reg);
public slots:
	// like the hardware bit 0 to 9 is, 0 top left to top right,
	// then bottom left to bottom right
	void SetButtons(uint16_t buttons);
signals:
	void SetLEDs(uint16_t led);
private:
	/* Sets LEDs from PortD enable bits and the current PortB values,
	 * if it changed signal SetLEDs
	 */
	void UpdateLEDs();
	QMutex Mutex;
	uint16_t Buttons, LEDs;
	// True if that chip is enabled to pass the bus bits (port B) to
	// drive the LEDs or to output the buttons to the bus.
	// U5LED driven by PD2 LED 0-7
	// U3LED driven by PD3 LED 8-9
	// U4Input driven active low by PD4 0-7
	// U2Input driven active low by PD5 8-9
	uint8_t PortD;
	// data bus bits
	uint8_t PortB;

	SquareAudio Audio;
};

#endif // _HALL_KEYPAD_H
