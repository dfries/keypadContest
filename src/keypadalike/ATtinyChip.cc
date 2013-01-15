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

#include "ATtinyChip.h"
#include "HallKeypad.h"
#include "Timer0.h"
#include "Timer1.h"

ATtinyChip::ATtinyChip() :
	Keypad(NULL),
	TimerObj0(NULL),
	TimerObj1(NULL),
	SystemClockHz(1000000) // ATtiny2313 default, selectable by fuses
{
	memset(Reg, 0, sizeof(Reg));
}

const ATtinyChip& ATtinyChip::operator=(RegValue arg)
{
	return Set(arg.Reg, [arg](uint8_t &v) { v=arg.Value; });
}

const ATtinyChip& ATtinyChip::operator+=(RegValue arg)
{
	return Set(arg.Reg, [arg](uint8_t &v) { v+=arg.Value; });
}

const ATtinyChip& ATtinyChip::operator-=(RegValue arg)
{
	return Set(arg.Reg, [arg](uint8_t &v) { v-=arg.Value; });
}

const ATtinyChip& ATtinyChip::operator|=(RegValue arg)
{
	return Set(arg.Reg, [arg](uint8_t &v) { v|=arg.Value; });
}

const ATtinyChip& ATtinyChip::operator&=(RegValue arg)
{
	return Set(arg.Reg, [arg](uint8_t &v) { v&=arg.Value; });
}

const ATtinyChip& ATtinyChip::operator^=(RegValue arg)
{
	return Set(arg.Reg, [arg](uint8_t &v) { v^=arg.Value; });
}

const ATtinyChip& ATtinyChip::Set(RegEnum reg, RegOperation op)
{
	uint8_t v=Reg[reg];
	uint8_t copy=v;
	op(v);
	if(v==copy)
		return *this;
	Reg[reg]=v;

	// For output ports only keep the bits with an output direction.
	switch(reg)
	{
	case REG_PORTD:
		v&=Reg[REG_DDRD];
		break;
	case REG_PORTB:
		v&=Reg[REG_DDRB];
		break;
	case REG_PORTA:
		v&=Reg[REG_DDRA];
		break;
	default:
		break;
	}

	// update peripherals
	switch(reg)
	{
	case REG_CLKPR:
		// clock change lock-out not implemented, assume the program
		// is doing it correctly
		if(v == _BV(CLKPCE))
			break;
		if(v > 8)
		{
			printf("ATtinyChip::Set invalid CLKPR value %u\n", v);
			break;
		}
		SystemClockHz=8000000 / (1<<v);
		if(TimerObj0)
			TimerObj0->SetSysteClock(SystemClockHz);
		if(TimerObj1)
			TimerObj1->SetSysteClock(SystemClockHz);
		break;
	case REG_PORTD:
	case REG_PORTB:
	case REG_PORTA:
		if(Keypad)
			Keypad->SetPort(reg, v);
		break;
	case REG_TCCR0A:
	case REG_TCCR0B:
	case REG_TCNT0:
	case REG_OCR0A:
	case REG_OCR0B:
	case REG_TIMSK:
	case REG_TIFR:
		// Only allocate on the first non-zero write.
		if(!TimerObj0 && v)
		{
			TimerObj0=new Timer0(Reg);
			TimerObj0->SetSysteClock(SystemClockHz);
			TimerObj0->start();
		}
		if(TimerObj0)
			TimerObj0->Set(reg, v);
		// Fall through for REG_TIMSK or REG_TIFR as TimerObj1 needs
		// them as well.
		if(reg != REG_TIMSK && reg != REG_TIFR)
			break;
	case REG_TCCR1A:
	case REG_TCCR1B:
	case REG_TCCR1C:
	case REG_TCNT1:
	case REG_TCNT1H:
	case REG_OCR1A:
	case REG_OCR1AH:
	case REG_OCR1B:
	case REG_OCR1BH:
	case REG_ICR1:
	case REG_ICR1H:
		// Only allocate on the first non-zero write.
		if(!TimerObj1 && v)
		{
			TimerObj1=new Timer1(Reg);
			TimerObj1->SetSysteClock(SystemClockHz);
			TimerObj1->start();
		}
		if(TimerObj1)
			TimerObj1->Set(reg, v);
		break;
	// registers that just need to update the register store
	case REG_DDRD:
	case REG_DDRB:
	case REG_DDRA:
	case REG_SREG: // interrupt concurrency is handled in ATtiny
		break;
	default:
		printf("unhandled register 0x%2x\n", reg);
		break;
	}
	return *this;
}

uint8_t ATtinyChip::GetValue(RegEnum reg)
{
	switch(reg)
	{
	case REG_PINB:
		if(Keypad)
		return Keypad->GetPort(reg);
	// Only the counter and interrupt flag registers are modified
	// from the timer counter, the rest can use the last written value.
	case REG_TCNT0:
		if(TimerObj0)
			return TimerObj0->Get(reg);
	case REG_TCNT1:
	case REG_TCNT1H:
		if(TimerObj1)
			return TimerObj1->Get(reg);
	case REG_TIFR:
		{
			// Each timer has different bits in the same
			// register, combine them into one.
			uint8_t flags=0;
			if(TimerObj0)
				flags |= TimerObj0->Get(reg);
			if(TimerObj1)
				flags |= TimerObj1->Get(reg);
			return flags;
		}
	default:
		break;
	}
	return Reg[reg];
}
