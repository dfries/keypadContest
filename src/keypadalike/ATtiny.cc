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

ATtiny::ATtiny() :
	ThreadsRunning(0),
	MainThread(NULL)
{
}

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

void ATtiny::MainStart()
{
	QMutexLocker locker(&Mutex);
	// The main thread can run if no other thread is running or if
	// interrupts are enabled.  As opposed to interrupt threads it can
	// still run even when interrupts are disabled.
	while(ThreadsRunning && !locked_IrqEnabled())
		Cond.wait(&Mutex);

	++ThreadsRunning;
}

void ATtiny::MainStop()
{
	QMutexLocker locker(&Mutex);
	--ThreadsRunning;
	Cond.wakeAll();
}

void ATtiny::MainSleep()
{
	QMutexLocker locker(&Mutex);
	// like MainStop let any other thread run
	--ThreadsRunning;
	Cond.wakeAll();

	// wait for an interrupt to broadcast
	Cond.wait(&Mutex);

	// like MainStart wait to run
	while(ThreadsRunning && !locked_IrqEnabled())
		Cond.wait(&Mutex);

	++ThreadsRunning;
}

void ATtiny::IntStart()
{
	QMutexLocker locker(&Mutex);
	// An interrupt thread can run if interrupts are enabled, but it
	// does disable interrupts to prevent any new threads from running.
	while(!locked_IrqEnabled())
		Cond.wait(&Mutex);

	// Would not get here unless interrupts were disabled, therefore
	// they can be disabled.
	locked_EnableInterrupts(false);
	++ThreadsRunning;
}

void ATtiny::IntStop()
{
	QMutexLocker locker(&Mutex);
	// Interrupts might be enabled or disabled, but the irq handler
	// wouldn't be running unless they started out enabled, so I assume
	// you always leave interrupts enabled, but I don't know for sure
	// if there is a way on the hardware that they would remain disabled.
	locked_EnableInterrupts(true);
	--ThreadsRunning;
	Cond.wakeAll();
}

void ATtiny::locked_EnableInterrupts(bool enable)
{
	uint8_t sreg=Chip.GetValue(REG_SREG);
	if(enable)
	{
		sreg |= _BV(SREG_I);
		Cond.wakeAll();
	}
	else
	{
		sreg &= ~_BV(SREG_I);
	}
	Chip=RegValue(REG_SREG, sreg);
}
