/*  Programming competition entry.

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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <util/atomic.h>
#ifndef __AVR
#include <util/delay.h>
#include <stdio.h>
#else
#define printf(msg, ...)
#endif

#include "../../include/ATtiny2313_clock.h"

// This code is designed for an ATtiny2313 in the HRT Model KP-2B keypad.
//
// The sound is played with an optional piezo speaker connected directly to pins
//  3 & 4 of the DB-9 connector.  According to the RS-232 transceiver's
//  datasheet, it can be shorted continuously, so this shouldn't be a problem.
//
// Low-level code for HRT KP2B keypad to read the switches and write the LEDs
//  adapted from code Copyright 2012 Andrew Sampson.  Released under the GPL v2.

#define LED_A_WRITE_LATCH PD2
#define LED_B_WRITE_LATCH PD3
#define SW_A_READ_OUTPUTENABLE PD4
#define SW_B_READ_OUTPUTENABLE PD5
#define SPKR_PIN_1 PD1
#define SPKR_PIN_2 PD6
#define SPKR_MASK (_BV(SPKR_PIN_1) | _BV(SPKR_PIN_2))
#define NUM_SWITCHES 10
#define NUM_LIGHTS 10

// Set the compare value to run the timer interrupt every millisecond.
//  Timer0 is an 8-bit timer, and of the possible prescalers (0, 8, 64, 256,
//  and 1024), the smallest prescaler that will result in a value that will
//  fit within 8-bits (has to be 255 or less) is 64.  Timer compare value will
//  be:
//   8000000 CPU cycle        second        timer cycle   125.0 timer cycle
//  ------------------ * ---------------- * ----------- = -----------------
//        second         1000 millisecond    64 cycle        millisecond
//
// The game speed will be increased by decreasing timer0 period, start at
// 2ms by couting to 250.
const uint8_t Timer0_TOP=250;

// The half-period in F_CPU cycles of each tone.  Used with melody[].  Comment/
//  uncomment as needed.  If lower frequencies are needed, the prescaler will
//  need to be changed (see CS12:0 in the datasheet).
#if 0
uint16_t PROGMEM notePeriods[]={
 //    Cycles-1  Note Frequency Index
    // 63333, // F2   87.31
    // 59779, // F#2  92.50
    // 56424, // G2   98.00
    // 53256, // G#2  103.83
    // 50269, // A2   110.00
    // 47448, // Bb2  116.54
    // 44785, // B2   123.47
    // 42272, // C3   130.81
    // 39899, // C#3  138.59
    // 37660, // D3   146.83
    // 35546, // Eb3  155.56
    // 33551, // E3   164.81
    // 31668, // F3   174.61
    // 29890, // F#3  185.00
    // 28212, // G3   196.00
    // 26629, // G#3  207.65
    // 25135, // A3   220.00
    // 23724, // Bb3  233.08
    // 22392, // B3   246.94
    // 21135, // C4   261.63
    // 19949, // C#4  277.18
    // 18829, // D4   293.67
    // 17773, // Eb4  311.13
    // 16775, // E4   329.63
       15834, // F4   349.23    0x0
    // 14945, // F#4  369.99
       14106, // G4   392.00    0x1
       13314, // G#4  415.31    0x2
       12567, // A4   440.00    0x3
       11862, // Bb4  466.16    0x4
       11196, // B4   493.88    0x5
       10568, // C5   523.25    0x6
    // 9975,  // C#5  554.37
       9415,  // D5   587.33    0x7
       8886,  // Eb5  622.25    0x8
       8388,  // E5   659.26    0x9
    // 7917,  // F5   698.46
       7473,  // F#5  739.99    0xA
       7053,  // G5   783.99    0xB
    // 6657,  // G#5  830.61
    // 6284,  // A5   880.00
    // 5931,  // Bb5  932.33
    // 5598,  // B5   987.77
    // 5284,  // C6   1046.50
    // 4987,  // C#6  1108.73
    // 4707,  // D6   1174.66
    // 4443,  // Eb6  1244.51
    // 4194,  // E6   1318.51
    // 3958,  // F6   1396.91
    // 3736,  // F#6  1479.98
    // 3527,  // G6   1567.98
    // 3329,  // G#6  1661.22
    // 3142,  // A6   1760.00
    // 2965,  // Bb6  1864.66
    // 2799,  // B6   1975.53
    // 2642,  // C7   2093.00
    // 2494,  // C#7  2217.46
    // 2354,  // D7   2349.32
    // 2222,  // Eb7  2489.02
    // 2097,  // E7   2637.02
    // 1979,  // F7   2793.83
    // 1868,  // F#7  2959.96
    // 1763,  // G7   3135.96
    // 1664,  // G#7  3322.44
    // 1571,  // A7   3520.00
    // 1483,  // Bb7  3729.31
    // 1400,  // B7   3951.07
    // 1321,  // C8   4186.01
};
#endif
enum ToneValues
{
	TONE_START,
	TONE_CAPTURE,
	TONE_FAIL,
	TONE_GAME_OVER,
	TONE_GAME_OVER_PT2
};
#if 0
uint16_t PROGMEM notePeriods[TONE_GAME_OVER_PT2+1]={
       8388,  // E5   659.26    0x9
       7473,  // F#5  739.99    0xA
       14106, // G4   392.00    0x1
       12567, // A4   440.00    0x3
       15834, // F4   349.23    0x0
#endif
const uint16_t PROGMEM notePeriods[]={
       15834, // F4   349.23    0x0
    // 14945, // F#4  369.99
       14106, // G4   392.00    0x1
       13314, // G#4  415.31    0x2
       12567, // A4   440.00    0x3
       11862, // Bb4  466.16    0x4
       11196, // B4   493.88    0x5
       10568, // C5   523.25    0x6
    // 9975,  // C#5  554.37
       9415,  // D5   587.33    0x7
       8886,  // Eb5  622.25    0x8
       8388,  // E5   659.26    0x9
    // 7917,  // F5   698.46
       7473,  // F#5  739.99    0xA
       7053,  // G5   783.99    0xB
};

// If non-zero, a tick has elapsed.
volatile uint8_t tickFlag = 0;

// Initialize timer0, which is responsible for the task tick timer.
//  Prescale = CLK/64; Mode = CTC; Desired value = 1.0 millisecond;
//  Actual value = 1.001mSec (+0.1%)
void timer0_init()
{
	// CTC mode
	TCCR0A = _BV(WGM01);

	OCR0A = Timer0_TOP;

	// Enable clock io source with prescaler of 64.
	TCCR0B = _BV(CS01) | _BV(CS00);

	// Enable output compare match A interrupt
	TIMSK = _BV(OCIE0A);
}

// Initialize the music oscillator timer.
//  Prescale = CLK; Mode = CTC
void timer1_init()
{
	// enable CTC mode
	// Enable clock io source with no prescaler.
	TCCR1B = _BV(WGM12) | _BV(CS10);

	// The timer is left running, give it a reasonable period, just so it
	// isn't going off all the time.
	OCR1A = 20000;


	// Enable output enable compare match A interrupt
	// shared with timer0, bit OR it in.
	TIMSK |= _BV(OCIE1A);
}

// Call this routine to initialize all peripherals
void init_devices()
{
	// Configure port B as all inputs.
	//DDRB = 0; // Register initialized to zero, so this isn't needed.

	// Configure port D latch, enable, and speaker pins as output.
	DDRD = (1 << LED_A_WRITE_LATCH) |
		(1 << LED_B_WRITE_LATCH) |
		(1 << SW_A_READ_OUTPUTENABLE) |
		(1 << SW_B_READ_OUTPUTENABLE) |
		SPKR_MASK;

	// Disable internal pull-ups
	//PORTB = 0; // Register initialized to zero, so this isn't needed.

	// The latch OutputEnable is active low, so disable it for now
	PORTD = _BV(SW_A_READ_OUTPUTENABLE) | _BV(SW_B_READ_OUTPUTENABLE);

	timer0_init();
	timer1_init();

	sei(); // Enable interrupts.
	// All peripherals are now initialized.
}

void start_tone(ToneValues tone)
{
	// set speaker pins to opposite states, they will be toggled from here
	// This assumes they are already the same value.
	PORTD ^= _BV(SPKR_PIN_1);
	/*
	uint8_t portd=PORTD;
	PORTD=_BV(SPKR_PIN_1) | (portd & ~_BV(SPKR_PIN_2));
	portd=PORTD;
	*/
	//PORTD ^= _BV(SPKR_PIN_1);

	// Timer1 output compare A for tone period
	OCR1A = pgm_read_word(&notePeriods[tone]);
}

void stop_tone()
{
	// Set speaker pins to the same states, they will be toggled together
	// and produce a zero net output (and use less power as compared
	// to keeping them opposite.
	// Set both to 0 so they are in a known state for start_tone.
	PORTD &= ~SPKR_MASK;
}

// Read current state of pushbuttons, this program doesn't require
// debouncing, so none is done.
// The switches will be read more often than the LEDs will be written, so
// optimize for switch reading by expecting port B set to input and pullup
// resistors disabled.
uint16_t read_switches()
{
	uint16_t currentSwitches;
	// Disable interrupts because TIMER1_COMPA_vect also modifies PORTD.
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// copy PORTD to avoid extra reads and clears
		uint8_t portd = PORTD;

		// Enable bus output for the first bank of switches.  Keep in
		// mind that the latch OutputEnable is active low
		PORTD = portd & ~_BV(SW_A_READ_OUTPUTENABLE);

		// I've found that a delay is necessary in order for the switch
		// values to appear on the bus.  Datasheets suggest the
		// propogation delay through the D-latch itself will be less
		// than a quarter of a clock cycle, so there may be some
		// capacitance in the circuit making this nop necessary.
		//
		// I added a memory barrier to make sure the compiler doesn't
		// reorder instructions.
		asm volatile("nop" ::: "memory");

		// Grab the values from port B
		currentSwitches = PINB;

		// Disable bus output for the first bank, enable second
		PORTD = portd & ~_BV(SW_B_READ_OUTPUTENABLE);

		// Another wait.
		asm volatile("nop" ::: "memory");

		// Grab the values from port B
		currentSwitches |= PINB << 8;

		// Disable bus output for the switches
		PORTD = portd;
	}
	return currentSwitches;
}

// Update LED display.  Note that the LEDs are connected between U3/U5 and VCC.
// Thus, they illuminate when the outputs on U3/U5 go *low*, so 0="LED on" and
// 1="LED off".
// The switches will be read more often than the LEDs will be written, so
// optimize for switch reading by leaving port B as input with the pullup
// resistors disabled.
void write_LEDs(uint16_t off_state)
{
	// Disable interrupts because TIMER1_COMPA_vect also modifies PORTD.
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// Set port B pins as output
		DDRB = 0b11111111;

		// Load port B with the lower eight commanded LED states.
		PORTB = (uint8_t)off_state;

		// copy PORTD to avoid extra reads and clears
		uint8_t portd=PORTD;

		// Enable the LED latches.  This actually turns the LEDs to
		// their commanded state.
		PORTD = portd | _BV(LED_A_WRITE_LATCH);

		// Disable the LED latches so future changes to port B don't
		// change the LEDs' states.
		PORTD = portd;

		// Load port B with the upper two commanded LED states and
		// repeat as before.
		PORTB = off_state >> 8;
		PORTD = portd | _BV(LED_B_WRITE_LATCH);
		PORTD = portd;

		// return to input and pullups disabled
		DDRB = 0;
		PORTB = 0;
	}
}

// The store for the pseudo-random number generator.
static uint8_t prand = 0b01001110;
// Returns a pseudo-random value between 1 and 255 by using a period-maximal
//  8-bit LFSR
uint8_t lfsr_prand()
{
	// Calculate the new LFSR value.
	prand = (prand << 1) + (1 & ((prand >> 1) + (prand >> 2) +
		(prand >> 3) + (prand >> 7)));
	return prand;
}

static uint8_t sequence, counter, data;
void SetSequence(uint8_t num)
{
	sequence=0;
	counter=0;
	data=0;
}

void SelectTone()
{
	if(!counter)
	{
		uint16_t value=read_switches();
		switch(value)
		{
		case 0xfffe:
			start_tone((ToneValues)5);
			++counter;
			break;
		case 0xfffd:
			start_tone((ToneValues)6);
			++counter;
			break;
		case 0xfffb:
			start_tone((ToneValues)7);
			//OCR0A += 20;
			++counter;
			break;
		case 0xfff7:
			start_tone((ToneValues)8);
			//OCR0A -= 20;
			++counter;
			break;
		case 0xffef:
			start_tone((ToneValues)9);
			//stop_tone();
			++counter;
			break;
		case 0xffdf:
			start_tone(TONE_START);
			++counter;
			break;
		case 0xffbf:
			start_tone(TONE_CAPTURE);
			++counter;
			break;
		case 0xff7f:
			start_tone(TONE_FAIL);
			++counter;
			break;
		case 0xfeff:
			start_tone(TONE_GAME_OVER);
			++counter;
			break;
		case 0xfdff:
			start_tone(TONE_GAME_OVER_PT2);
			++counter;
			break;
		}
	}
	else
	{
		if(++counter == 255)
		{
			SetSequence(sequence+1);
		}
	}
}

void task_dispatch()
{
	switch(sequence)
	{
	case 0:
		SelectTone();
		break;
	default:
		SetSequence(0);
	}
}

int main()
{
	// This lets the Makefile set F_CPU to the Hz requested and keeps
	// the delay calculations consistent.
	// It expands to 6 bytes of program text (ATtiny2313).
	CPU_PRESCALE(inline_cpu_hz_to_prescale(F_CPU));

	// Initialize all MCU hardware.
	init_devices();

	// Use some non-determinism in when the CPU speed changes to generate
	// a random initial seed value.  Don't use the fastest speed in
	// case it is a lower voltage environment that doesn't support the
	// higher rates.  The timer counter clocks need to be running.
	// The datasheet says it isn't know when it switches CPU clock
	// speeds, apparently all the clocks and timers are switching
	// together and it always comes up with the same answer.
	#if 0
	for(uint8_t h=0; h<16; ++h)
	{
		for(uint8_t i=2; i<8; ++i)
		{
			CPU_PRESCALE(i);
			uint8_t clock = TCNT0 ^ TCNT1L;
			for(uint8_t j=0; j<clock; ++j)
			{
				write_LEDs(lfsr_prand());
			}
		}
	}
	#endif

	uint16_t leds=lfsr_prand() | lfsr_prand()<<8;
	write_LEDs(leds);

	for(;;)
	{
		#ifndef __AVR
		// Be a little nicer when not on the avr hardware since the
		// speaker interrupt really is on another thread, this thread
		// doesn't need to wake up every time the interrupt goes off.
		_delay_us(1000);
		#endif
		// sleep while waiting for an interrupt
		// see avr/sleep.h by disabling the interrupt the check is
		// atomic
		/*
		set_sleep_mode(SLEEP_MODE_IDLE);
		cli();
		*/

		// If another time slice has elapsed
		if(tickFlag)
		{
			//sei();

			// Reset this flag so this code isn't run again until
			// another slice has elapsed.
			tickFlag = 0;

			// Run the next appropriate task (if appropriate for
			// this pass).
			task_dispatch();

			//leds ^= 1;
			static int delay;
			if(!(++delay%200))
				leds ^= 2;
			write_LEDs(leds);
		}
		/*
		else
		{
			sleep_enable();
			sei();
			sleep_cpu();
			sleep_disable();
		}
		*/
	}

	return -1;
}

// Called when Timer0 counter matches OCR0A
ISR(TIMER0_COMPA_vect)
{
	tickFlag = 1;
}

// Called when Timer1 counter matches OCR1A
ISR(TIMER1_COMPA_vect)
{
	// Toggle the speaker pins to make a click.
	PORTD ^= SPKR_MASK;
	/*
	static uint16_t state;
	write_LEDs(++state);
	*/
	write_LEDs(OCR1A);
}
