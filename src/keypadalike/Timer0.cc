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

#include "Timer0.h"
#include "util.h"
#include <math.h>

Timer0::Timer0(const uint8_t *reg) :
	Timer(reg, NULL, "TIMER0_COMPA_vect", "TIMER0_COMPB_vect",
		"TIMER0_OVF_vect")
{
}

void Timer0::Set(RegEnum reg, uint8_t value)
{
	if(reg==REG_TIFR)
	{
		// writing 1 clears the flag
		Reg[REG_TIFR] &= ~value;
		return;
	}

	Reg[reg]=value;

	UpdateSleep();
}

void Timer0::UpdateSleep()
{
	// There isn't currently any way for the timer thread to restart
	// based on updated registers values.  It will only complete the
	// current sleep and start the next one.
	uint8_t mode=
		((Reg[REG_TCCR0B] & _BV(WGM02))>>1) |
		(Reg[REG_TCCR0A] & _BV(WGM01)) |
		(Reg[REG_TCCR0A] & _BV(WGM00));
	// splits register A and B
	enum WaveformGeneration8
	{
		Normal_WGM=0,
		/* phase correct */
		PWM_8b_WGM=1,

		CTC_WGM=2,

		FastPWM_8b_WGM=3,

		/* phase correct */
		PWM_phc_OCR_WGM=5,

		FastPWM_OCR_WGM=7
	};

	if(mode && mode != CTC_WGM)
		printf("Timer0::Set timer mode %u not implemented, using CTC\n",
			mode);

	uint8_t clock=Reg[REG_TCCR0B] & 0x7;
	if(clock >= 6)
		printf("Timer0::Set clock source %u not implemented, timer "
			"will be slow\n", clock);
	// clock zero is stopped, REG_OCR0A would keep it at zero?
	if(!clock || !Reg[REG_OCR0A])
	{
		QMutexLocker locker(&Mutex);
		memset(SleepSequence, 0, sizeof(SleepSequence));
		return;
	}
	// CTC mode clears when the counter gets to OCR0A, other modes
	// will use other registers.
	double duration = SecPerTick(REG_TCCR0B);
	// ignoring B for now and only using CTC register A
	uint8_t top=Reg[REG_OCR0A];
	// mode 0 is normal mode, maximum range
	if(mode == 0)
		top = 0xff;
	// seconds per repitition
	duration *= top;

	Seq sleep_array[sizeof(SleepSequence)/sizeof(*SleepSequence)]={{{0}}};
	Seq &seq=sleep_array[0];
	seq.Duration.tv_sec=(long)duration;
	seq.Duration.tv_nsec=(duration-seq.Duration.tv_sec)*1e9;
	seq.IrqFlag=_BV(OCF0A);
	if(Reg[REG_TIMSK] & _BV(OCIE0A))
		seq.func=CompA;

	{
		QMutexLocker locker(&Mutex);
		memcpy(&SleepSequence, &sleep_array, sizeof(SleepSequence));
		Cond.wakeAll();
	}
}

uint8_t Timer0::Get(RegEnum reg)
{
	if(reg==REG_TIFR)
		return Reg[REG_TIFR];
	if(reg!=REG_TCNT0)
		return (uint8_t)rand();
	
	// calculate the counter TCNT0 value from the elapsed time
	struct timeval now;
	gettimeofday(&now, NULL);
	// This is really only valid if the timer is running and it is
	// less than or equal to the current TOP.  It can be greater than
	// top if the sleep is late.
	return (uint8_t)fmod((now - Start) / SecPerTick(REG_TCCR0B),
		Reg[REG_OCR0A]);
}
