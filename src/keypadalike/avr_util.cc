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

#include <util/delay.h>
#include "ATtiny.h"
#include <sys/time.h>
#include <sched.h>
#include <time.h>
#include <stdio.h>
#include "util.h"

using namespace std;

void _delay_ms(double ms)
{
	//printf("%s %10.6f seconds\n", __func__, ms/1000);
	#if 0
	// busy loop sub 1ms sleep times (doesn't seem to be required)
	if(ms < 1)
	{
		struct timeval start, now;
		gettimeofday(&start, NULL);
		double sec=ms/1000;
		for(;;)
		{
			gettimeofday(&now, NULL);
			double delta=now-start;
			if(delta>sec)
				break;
			// be nice for part of the remaining time
			#if 1
			if(delta<sec*.8)
				sched_yield();
			#endif
		}
		return;
	}
	#endif
	int is_main=g_ATtiny.IsMain();
	if(is_main)
		g_ATtiny.MainStop();
	/* Interrupts are already concurrent (other interrupts are allowed
	 * to run) or not, so they don't need the inverse stop/start.
	else
		g_ATtiny.IntStop();
		*/
	struct timespec req={(long int)(ms/1000)};
	req.tv_nsec=(ms - req.tv_sec*1000)*1000000;
	nanosleep(&req, NULL);
	if(is_main)
		g_ATtiny.MainStart();
	/*
	else
		g_ATtiny.IntStart();
	*/
}

void _delay_us(double us)
{
	_delay_ms(us/1000);
}

void sei()
{
	g_ATtiny.EnableInterrupts(true);
}

void cli()
{
	g_ATtiny.EnableInterrupts(false);
}
