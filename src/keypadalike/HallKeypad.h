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

/* Emulates the Hall Research KP2B keypad connections to the microcontroller
 * registers.
 */
class HallKeypad : public QObject
{
	Q_OBJECT
public slots:
	// like the hardware bit 0 to 9 is, 0 top left to top right,
	// then bottom left to bottom right
	void SetButtons(uint16_t buttons);
signals:
	void SetLEDs(uint16_t led);
private:
	QMutex Mutex;
	uint16_t Buttons, LEDs;
};

#endif // _HALL_KEYPAD_H
