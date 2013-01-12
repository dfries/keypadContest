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

#ifndef _AT_TINY_H
#define _AT_TINY_H

#include <QMutex>
#include <QMutexLocker>
#include "avr/io.h"
#include "ATtinyChip.h"

class HallKeypad;

/* This class wraps the main ATtinyChip to provide thread safe operations
 * so that ATtinyChip doesn't need to do any locking interally.
 */
class ATtiny
{
public:
	void SetPeripheral(HallKeypad *keypad)
	{
		QMutexLocker locker(&Mutex);
		Chip.SetPeripheral(keypad);
	}
	// It is using the operator syntax just to make it obvious what
	// operation they represent.
	const ATtiny& operator=(RegValue arg)
	{
		QMutexLocker locker(&Mutex);
		Chip=arg;
		return *this;
	}
	const ATtiny& operator|=(RegValue arg)
	{
		QMutexLocker locker(&Mutex);
		Chip|=arg;
		return *this;
	}
	const ATtiny& operator&=(RegValue arg)
	{
		QMutexLocker locker(&Mutex);
		Chip&=arg;
		return *this;
	}
	const ATtiny& operator^=(RegValue arg)
	{
		QMutexLocker locker(&Mutex);
		Chip^=arg;
		return *this;
	}
	uint8_t GetValue(RegEnum reg)
	{
		QMutexLocker locker(&Mutex);
		return Chip.GetValue(reg);
	}
private:
	ATtinyChip Chip;
	QMutex Mutex;
};

extern ATtiny g_ATtiny;

#endif // _AT_TINY_H
