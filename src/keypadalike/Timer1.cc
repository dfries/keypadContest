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

#include "Timer1.h"
#include "util.h"
#include <math.h>

Timer1::Timer1(const uint8_t *reg) :
	Timer(reg, "TIMER1_CAPT_vect", "TIMER1_COMPA_vect", "TIMER1_COMPB_vect",
		"TIMER1_OVF_vect")
{
}

void Timer1::Set(RegEnum reg, uint8_t value)
{
	if(reg==REG_TIFR)
	{
		// writing 1 clears the flag
		Reg[REG_TIFR] &= ~value;
		return;
	}

	Reg[reg]=value;

	// Writing to the high value of the clock would write to a shared
	// 8 bit register with the write not taking place until the low
	// 8 bit register is written, then both are applied in the same
	// clock cycle.
	switch(reg)
	{
	case REG_TCNT1H:
	case REG_OCR1AH:
	case REG_OCR1BH:
	case REG_ICR1H:
		return;
	default:
		break;
	}

	UpdateSleep();
}

void Timer1::UpdateSleep()
{
	// There isn't currently any way for the timer thread to restart
	// based on updated registers values.  It will only complete the
	// current sleep and start the next one.
	uint8_t mode=
		((Reg[REG_TCCR1B] & _BV(WGM13))>>1) |
		((Reg[REG_TCCR1B] & _BV(WGM12))>>1) |
		(Reg[REG_TCCR1A] & _BV(WGM11)) |
		(Reg[REG_TCCR1A] & _BV(WGM10));
	// splits register A and B
	enum WaveformGeneration16
	{
		Normal_WGM=0,
		/* phase correct */
		PWM_8b_WGM=1,
		PWM_9b_WGM=2,
		PWM_10b_WGM=3,

		CTC_WGM=4,

		FastPWM_8b_WGM=5,
		FastPWM_9b_WGM=6,
		FastPWM_10b_WGM=7,

		/* phase and frequency correct */
		PWM_phfqc_ICR_WGM=8,
		PWM_phfqc_OCR_WGM=9,

		/* phase correct */
		PWM_phc_ICR_WGM=10,
		PWM_phc_OCR_WGM=11,

		CTC_ICR_WGM=12,

		FastPWM_ICR_WGM=14,
		FastPWM_OCR_WGM=15
	};

	if(mode && mode != CTC_WGM)
		printf("Timer1::Set timer mode %u not implemented, using CTC\n",
			mode);

	uint8_t clock=Reg[REG_TCCR1B] & 0x7;
	if(clock >= 6)
		printf("Timer1::Set clock source %u not implemented, timer "
			"will be slow\n", clock);
	// clock zero is stopped, REG_OCR1A would keep it at zero?
	if(!clock || (!Reg[REG_OCR1A] && !Reg[REG_OCR1AH]))
	{
		QMutexLocker locker(&Mutex);
		memset(SleepSequence, 0, sizeof(SleepSequence));
		return;
	}
	// CTC mode clears when the counter gets to OCR1A, other modes
	// will use other registers.
	double duration = SecPerTick(REG_TCCR1B);
	// ignoring B for now and only using CTC register A
	uint16_t top;
	memcpy(&top, Reg+REG_OCR1A, sizeof(top));
	// mode 0 is normal mode, maximum range
	if(mode == 0)
		top = 0xffff;
	// seconds per repitition
	duration *= top;
	duration /= 2;
	//printf("duration %8.6f\n", duration);

	Seq sleep_array[sizeof(SleepSequence)/sizeof(*SleepSequence)]={{{0}}};
	Seq &seq=sleep_array[0];
	seq.Duration.tv_sec=(long)duration;
	seq.Duration.tv_nsec=(duration-seq.Duration.tv_sec)*1e9;
	seq.IrqFlag=_BV(OCF1A);
	if(Reg[REG_TIMSK] & _BV(OCIE1A))
		seq.func=CompA;

	{
		QMutexLocker locker(&Mutex);
		memcpy(&SleepSequence, &sleep_array, sizeof(SleepSequence));
		Cond.wakeAll();
	}
}

uint8_t Timer1::Get(RegEnum reg)
{
	if(reg==REG_TIFR)
		return Reg[REG_TIFR];

	// Reading the low byte will store the high byte to this register.
	// Reading high will retrieve it, always read the low byte first.
	if(reg==REG_TCNT1H)
		return Reg[REG_TCNT1H];

	if(reg!=REG_TCNT1)
		return (uint8_t)rand();
	
	// calculate the counter TCNT1 value from the elapsed time
	struct timeval now;
	gettimeofday(&now, NULL);
	// This is really only valid if the timer is running and it is
	// less than or equal to the current TOP.  It can be greater than
	// top if the sleep is late.
	uint16_t ocr1a=Reg[REG_OCR1AL] | Reg[REG_OCR1AH]<<8;
	uint16_t counter=(uint16_t)fmod((now - Start) / SecPerTick(REG_TCCR1B),
		ocr1a);
	Reg[REG_TCNT1H]=counter>>8;
	return (uint8_t)counter;
}
