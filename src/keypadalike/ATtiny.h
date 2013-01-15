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
#include <QWaitCondition>
#include <QMutexLocker>
#include <QThread>
#include "avr/io.h"
#include "ATtinyChip.h"

class HallKeypad;

/* This class wraps the main ATtinyChip to provide thread safe operations
 * so that ATtinyChip doesn't need to do any locking interally.
 */
class ATtiny
{
public:
	ATtiny();

	void SetPeripheral(HallKeypad *keypad)
	{
		QMutexLocker locker(&Mutex);
		Chip.SetPeripheral(keypad);
	}

	/* The microprocessor has the main thread execution and interrupts.
	 * There is only one CPU and so they are never executing concurrently,
	 * but the interrupts can happen any time interrupts are enabled,
	 * leading to the same kinds of concurrency problems.  To prevent
	 * the main function and interrputs from runing at the same time,
	 * call this function from each thread so they will only run on
	 * one CPU and never at the same time.  This will allow them to
	 * task share, but if that's a problem the microcontroller code
	 * probably has problems anyway.
	 */
	static void SetThreadAffinity();

	/* Call once to store which thread is the main thread.
	 * This is used to find out when the behavior is different
	 * between the main thread and interrupts.
	 */
	void RegisterMainThread() { MainThread=QThread::currentThread(); }
	bool IsMain() { return MainThread==QThread::currentThread(); }

	/* These functions exist to do the thread synchronization required
	 * to emulate a main program and interrupt handlers.  That means
	 * when interrupts are disabled only the main thread or an
	 * interrupt thread may run while the other are blocked, that is
	 * also how atomic operations are implemented on the chip, if the
	 * interrupts (or main thread when in an interrupt), can't run
	 * it looks like it is atomic.  See SetThreadAffinity for how only
	 * one thread is executing at a time, they will be allowed to
	 * task switch, which isn't how the hardware works.
	 *
	 * Interrupt handlers are executed with global interrupts initially
	 * disabled, until they are enabled only one interrupt handler can
	 * run at a time.  That is covered by checking if interrupts are
	 * enabled when start is called.
	 */
	void MainStart();
	void MainStop();
	void IntStart();
	void IntStop();
	/* Causes the main thread to sleep until an interrupt handler
	 * returns.  Unlike the real hardare there is a race condition.
	 * In the hardware enabling interrupts followed by sleep guarantees
	 * that the sleep will be executed before any interrupt goes off,
	 * but that's currently not the case here, an interrupt can be
	 * missed between those two calls.
	 */
	void MainSleep();
	void EnableInterrupts(bool enable)
	{
		QMutexLocker locker(&Mutex);
		locked_EnableInterrupts(enable);
	}

	// It is using the operator syntax just to make it obvious what
	// operation they represent.
	const ATtiny& operator=(RegValue arg)
	{
		QMutexLocker locker(&Mutex);
		Chip=arg;
		return *this;
	}
	const ATtiny& operator+=(RegValue arg)
	{
		QMutexLocker locker(&Mutex);
		Chip+=arg;
		return *this;
	}
	const ATtiny& operator-=(RegValue arg)
	{
		QMutexLocker locker(&Mutex);
		Chip-=arg;
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
	QWaitCondition Cond;
	int ThreadsRunning;
	QThread *MainThread;

	// Mutex must be held
	// returns true if interrupts are enabled
	bool locked_IrqEnabled()
	{
		return Chip.GetValue(REG_SREG) & _BV(SREG_I);
	}
	// Mutex must be held
	// interrupts are enabled if enable is true
	void locked_EnableInterrupts(bool enable);
};

extern ATtiny g_ATtiny;

#endif // _AT_TINY_H
