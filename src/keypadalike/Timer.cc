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

#include "Timer.h"
#include <QMutexLocker>
#include <dlfcn.h>

Timer::Timer(const uint8_t *reg, const char *capt, const char *comp_a,
	const char *comp_b, const char *ovf) :
	CompA(NULL),
	SystemClockHz(1)
{
	memcpy(Reg, reg, sizeof(Reg));
	struct
	{
		const char *name;
		void (**f)();
	} funcs[] = {
		{capt, &Capt},
		{comp_a, &CompA},
		{comp_b, &CompB},
		{ovf, &Ovf}};
	for(size_t i=0; i<sizeof(funcs)/sizeof(*funcs); ++i)
	{
		if(!funcs[i].name)
			continue;
		/* Note dlsym can only find symbols in shared objects,
		 * if the function is compiled into the executable it
		 * will not find it.
		 */
		if(void *sym=dlsym(RTLD_DEFAULT, funcs[i].name))
		{
			//printf("%s is %p\n", funcs[i].name, sym);
			*funcs[i].f=(void(*)())sym;
		}
		/*
		else
		{
			printf("%s not found\n", funcs[i].name);
		}
		*/
	}
}

void Timer::run()
{
	bool running;
	for(;;)
	{
		running=false;
		for(size_t i=0; i<sizeof(SleepSequence)/sizeof(*SleepSequence);
			++i)
		{
			Seq seq;
			{
				QMutexLocker locker(&Mutex);
				seq=SleepSequence[i];
			}
			if(seq.Duration.tv_sec || seq.Duration.tv_nsec)
			{
				if(!running)
				{
					// Store when the counter would have
					// been 0.
					gettimeofday(&Start, NULL);
					running=true;
				}
				nanosleep(&seq.Duration, NULL);
				Reg[seq.IrqFlagReg]|=seq.IrqFlag;
				if(seq.func)
				{
					Reg[seq.IrqFlagReg]&=~seq.IrqFlag;
					seq.func();
				}
			}
		}
		if(!running)
		{
			QMutexLocker locker(&Mutex);
			Cond.wait(&Mutex);
		}
	}
}
