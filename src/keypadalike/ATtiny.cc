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

#include "ATtiny.h"
#include <sched.h>
#include <string.h>

ATtiny g_ATtiny;

void ATtiny::SetThreadAffinity()
{
	// Schedule this thread only on the first CPU, any will work, but
	// the first will always be there to select.
	cpu_set_t mask;
	memset(&mask, 0, sizeof(mask));
	CPU_SET(0, &mask);
	// 0 is the caller's thread id
	sched_setaffinity(0, sizeof(mask), &mask);
}
