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

#ifndef _TIMER_H
#define _TIMER_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <sys/time.h>
#include <time.h>
#include <avr/io.h>

/* Base class for timer operations.  It contains timer and routines common
 * to all timers.  The derived timers deal with the actual registers and setup.
 * Don't add any slots because exec won't be called and they won't be received.
 */
class Timer : public QThread
{
	Q_OBJECT
public:
	/* The arguments are the string names of the interrupt handlers to
	 * call.  They are resolved at runtime because not all programs
	 * will have all interrupt handlers.  Pass NULL if the timer doesn't
	 * have that handler.
	 * Not all interrupts have been coded up to trigger yet.
	 */
	Timer(const uint8_t *reg, const char *capt, const char *comp_a,
		const char *comp_b, const char *ovf);
	virtual void Set(RegEnum reg, uint8_t value) = 0;
	virtual uint8_t Get(RegEnum reg) = 0;
	void SetSysteClock(uint32_t hz);
protected:
	// Where the sleep time should be updated.  Called from the base
	// class when the system clock rate chanes.
	virtual void UpdateSleep() = 0;
	void run();

	/* Capture interrupt is used to record the counter time to
	 * 16 bit ICR1 when an event occurs.
	 */
	void (*Capt)();
	// timer matches A
	void (*CompA)();
	// timer matches B
	void (*CompB)();
	// timer overflow */
	void (*Ovf)();

	double SecPerTick(RegEnum tccrxb);

	uint8_t Reg[REG_SREG];
	uint32_t SystemClockHz;

	// The time the counter last started at zero.
	struct timeval Start;
	/* The hardware timer would count from zero and match at three
	 * different locations, A, B, and finally overflow, it can be
	 * configured to reset at any.  This thread will sleep with the
	 * given duration one after the other and make the call back each
	 * time the sleep is finished.  If the duration is zero it will
	 * be skipped, if the callback is NULL the sleep will happen the
	 * callback won't, and if all the durations are zero it will
	 * block on the condition variable.
	 */
	struct Seq
	{
		struct timespec Duration;
		// When the interrupt goes off this flag is set, and cleared
		// by writing 1 to the register or when the interrupt vector
		// executes.
		uint8_t IrqFlag;
		void (*func)();
	} SleepSequence[3];

	// When the timer isn't actively running it is waiting on the Cond
	// variable.
	QMutex Mutex;
	QWaitCondition Cond;
};

#endif // _TIMER_H
