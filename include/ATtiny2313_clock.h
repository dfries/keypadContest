/*
Copyright 2012 David Fries.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef ATtiny2313_clock_H
#define ATtiny2313_clock_H

#include <avr/io.h>

/* Clock prescaler, MHz, and change logic for the ATtiny2313 assuming the
 * internal 8 MHz oscillator is selected for the clock source.
 */

/* Setting the prescale speed requires two writes, _BV(CLKPCE) enables change,
 * then the new value must be written within 4 clock cycles, disable
 * interrupts to be sure.
 */
#define CPU_PRESCALE(n)	\
{uint8_t __read_hz=n; CLKPR = _BV(CLKPCE); CLKPR = __read_hz;}
#define CPU_8MHz        0x00
#define CPU_4MHz        0x01
#define CPU_2MHz        0x02
#define CPU_1MHz        0x03
#define CPU_500kHz      0x04
#define CPU_250kHz      0x05
#define CPU_125kHz      0x06
#define CPU_62500Hz     0x07
#define CPU_31250Hz     0x08

/* Leave this undefined to catch non-constant (invalid) input values.
 */
uint8_t hz_is_not_valid(uint32_t hz);
/* Takes the number of CPU cycles per second and returns the CPU prescaler
 * CLKPR value.  The hardware only supports 9 clock rates.
 * returns the prescale CLKPR value
 *
 * The compile will optimize the entire call to a constant.
 * Note, if the argument isn't a compiler constant, the compiler will generate
 * the function in assembly and as long as hz_is_not_valid isn't found
 * it will fail to link.  As long as it is a compiler constant value it won't
 * try to call hz_is_not_valid and work.
 */
uint8_t cpu_hz_to_prescale(uint32_t hz);
inline uint8_t inline_cpu_hz_to_prescale(uint32_t hz)
{
	return	hz==8000000 ? CPU_8MHz :
		hz==4000000 ? CPU_4MHz :
		hz==2000000 ? CPU_2MHz :
		hz==1000000 ? CPU_1MHz :
		hz==500000 ? CPU_500kHz :
		hz==250000 ? CPU_250kHz :
		hz==125000 ? CPU_125kHz :
		hz==62500 ? CPU_62500Hz :
		hz==31250 ? CPU_31250Hz :
		hz_is_not_valid(hz);
}

// Add -DF_CPU=8000000 (or a valid Hz) to the compile command line, and
// place the following line early in main.
//
// CPU_PRESCALE(inline_cpu_hz_to_prescale(F_CPU));
//
// This lets the Makefile set F_CPU to the Hz requested and keeps
// the delay calculations consistent.
// It expands to 6 bytes of program text (ATtiny2313).

#endif /* ATtiny2313_clock_H */
