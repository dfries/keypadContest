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

#include <QApplication>
#include <QThread>
#include <QMetaType>
#include <stdlib.h>
#include "MicroMain.h"
#include "SoftIO.h"
#include "HallKeypad.h"
#include "ATtiny.h"

// include/avr/io.h uses a macro to rename main to avr_main
#ifdef AVR_MAIN
#undef main
#endif

/* overview
 * SoftIO, the GUI showing the LED status and buttons for input
 * ATtiny (interface through ATtinyChip), register and microcontroller status
 * HallKeypad, accessed through ATtinyChip to read from write to SoftIO LED
 * and button status in place of the Hall Research KP2B keypad,
 * the object is given to ATtiny to call into (as the real microcontroller
 * would interface with)
 * MicroMain runs the main microcontroller routine, interfaces with ATtiny
 */
int main(int argc, char **argv)
{
	// Register the types to be used in indirect signals
	qRegisterMetaType<uint16_t>("uint16_t");

	QApplication app(argc, argv);
	SoftIO io;
	HallKeypad keypad;
	QObject::connect(&io, SIGNAL(SetButtons(uint16_t)),
		&keypad, SLOT(SetButtons(uint16_t)));
	QObject::connect(&keypad, SIGNAL(SetLEDs(uint16_t)),
		&io, SLOT(SetLEDs(uint16_t)));
	io.show();

	QThread main_thread;
	MicroMain micro_main;
	micro_main.moveToThread(&main_thread);
	QObject::connect(&main_thread, SIGNAL(started()),
		&micro_main, SLOT(Run()));
	main_thread.start();

	g_ATtiny.SetPeripheral(&keypad);
	int ret = app.exec();
	// The microprocessor main is not expected to return, just exit instead.
	exit(2);
	return ret;
}
