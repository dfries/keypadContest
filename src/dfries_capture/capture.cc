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
#include <avr/eeprom.h>
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
// Used to make the tone go up each time the timer goes off
static uint8_t CaptureTone=0;

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
const uint16_t PROGMEM notePeriods[TONE_GAME_OVER_PT2+1]={
       7053,  // G5   783.99    0xB
       6284,  // Eb5  622.25    0x8
       12567, // A4   440.00    0x3
       14106, // G4   392.00    0x1
       15834, // F4   349.23    0x0
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
	uint8_t portd=PORTD;
	PORTD=_BV(SPKR_PIN_1) | (portd & ~_BV(SPKR_PIN_2));
	portd=PORTD;

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
	return ~currentSwitches;
}

// Update LED display.  Note that the LEDs are connected between U3/U5 and VCC.
// Thus, they illuminate when the outputs on U3/U5 go *low*, so 0="LED on" and
// 1="LED off" (inverted here to make 1 on).
// The switches will be read more often than the LEDs will be written, so
// optimize for switch reading by leaving port B as input with the pullup
// resistors disabled.
void write_LEDs(uint16_t on_state)
{
	on_state=~on_state;
	// Disable interrupts because TIMER1_COMPA_vect also modifies PORTD.
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// Set port B pins as output
		DDRB = 0b11111111;

		// Load port B with the lower eight commanded LED states.
		PORTB = (uint8_t)on_state;

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
		PORTB = on_state >> 8;
		PORTD = portd | _BV(LED_B_WRITE_LATCH);
		PORTD = portd;

		// return to input and pullups disabled
		DDRB = 0;
		PORTB = 0;
	}
}

// The store for the pseudo-random number generator.  It will return zero
// until seeded by setting prand to a different value.
static uint8_t prand = 0;
// Returns a pseudo-random value between 1 and 255 by using a period-maximal
//  8-bit LFSR
uint8_t lfsr_prand()
{
	// Calculate the new LFSR value.
	prand = (prand << 1) + (1 & ((prand >> 1) + (prand >> 2) +
		(prand >> 3) + (prand >> 7)));
	return prand;
}

enum GameState
{
	START_HERE,
	COUNT_DOWN,
	GAME_LOOP,
	CAPTURED,
	FAIL_TURN,
	CURRENT_SCORE1,
	CURRENT_SCORE2,
	CURRENT_SCORE3,
	HIGH_SCORE,
	RESTART,
	NEW_HIGH_SCORE,
	GO_TO_FIRST,
};
static GameState State;
static uint8_t counter, data;
// In the game it moves left or right.
static uint8_t direction, tries;
// One of the button that should have been pressed, or the last location
static uint16_t fail_position;

static uint8_t CurrentScore;
const uint8_t FAST_MOVING=20, MOVING_TIMEOUT=60, STATE_PAUSE=128,
	STATIC_TIMEOUT=254;

uint8_t EEMEM HighScore=0;

// State the state, this will clear additional data for the next state.
// The counter and data will be zero when each next state starts.
void SetState(GameState next)
{
	State=next;
	counter=0;
	data=0;

	// List all the states that set the clock rate back to default
	// basically anything not in a game mode, which is important in
	// starting a new game or displaying the current score without
	// going by too fast to read.
	switch(State)
	{
	case START_HERE:
	case HIGH_SCORE:
	case CURRENT_SCORE1:
	case RESTART:
	case NEW_HIGH_SCORE:
		// back to slow speed
		OCR0A = Timer0_TOP;
	default:
		;
	}
}

void AdvanceState()
{
	SetState((GameState)(State+1));
}

void DisplayNewHighScoreAnim()
{
	if(!(data&1))
	{
		write_LEDs(0b1111111111);
		if(counter==0)
			start_tone(TONE_GAME_OVER);
	}
	else
	{
		write_LEDs(0);
		if(counter==0)
			stop_tone();
	}
	if(++counter==MOVING_TIMEOUT)
	{
		counter=0;
		++data;
	}
	if(data==8)
		SetState(HIGH_SCORE);
}

void FailTurn()
{
	uint16_t led;
	if(!(data&1))
	{
		if(counter==0)
			start_tone(TONE_FAIL);
		led=fail_position;
	}
	else
	{
		led=0;
		if(counter==0)
			stop_tone();
	}
	write_LEDs(led);
	if(++counter==STATIC_TIMEOUT)
	{
		counter=0;
		++data;
	}
	if(data==6)
	{
		if(tries)
		{
			SetState(COUNT_DOWN);
		}
		else if(CurrentScore > eeprom_read_byte(&HighScore))
		{
			eeprom_write_byte(&HighScore, CurrentScore);
			SetState(NEW_HIGH_SCORE);
		}
		else
		{
			SetState(CURRENT_SCORE1);
		}
	}
}

void Captured()
{
	if(counter==0)
	{
		start_tone(TONE_CAPTURE);
		CaptureTone=1;
	}
	if(++counter==STATE_PAUSE)
	{
		CaptureTone=0;
		stop_tone();
		SetState(COUNT_DOWN);
	}
}

void RunGame()
{
	uint16_t led;
	if(data<10)
	{
		if(data<5)
		{
			// upper row
			if(direction)
				led=1<<(4-data);
			else
				led=1<<data;
		}
		else
		{
			// lower row
			if(direction)
				led=1<<(14-data);
			else
				led=1<<(data);
		}
		write_LEDs(led);
		uint16_t btn=read_switches();
		if(btn)
		{
			uint16_t pos;
			// check for the wrong button
			if(direction)
				pos=_BV(5);
			else
				pos=_BV(9);
			if(btn != pos)
			{
				--tries;
				fail_position=pos;
				SetState(FAIL_TURN);
				//printf("fail %d pos %x\n", __LINE__, pos);
			}
			else if(led == _BV(7))
			{
				CurrentScore+=2;
				SetState(CAPTURED);
				if(OCR0A>15)
					OCR0A-=10;
				//printf("good %d pos %x\n", __LINE__, led);
			}
			else if(led == _BV(6) || led == _BV(8))
			{
				++CurrentScore;
				SetState(CAPTURED);
				if(OCR0A>15)
					OCR0A-=10;
				//printf("good %d pos %x\n", __LINE__, led);
			}
			else
			{
				--tries;
				fail_position=led;
				SetState(FAIL_TURN);
				//printf("fail %d pos %x\n", __LINE__, led);
			}
			return;
		}
		if(++counter==MOVING_TIMEOUT)
		{
			++data;
			counter=0;
		}
		return;
	}
	if(data>=10)
	{
		// too late
		--tries;
		if(direction)
			fail_position=_BV(5);
		else
			fail_position=_BV(9);
		//printf("late %d pos %x\n", __LINE__, led);
		SetState(FAIL_TURN);
	}
}

void CountDown()
{
	// blinks and plays audio to prepare the user for the game turn

	if(data==0)
	{
		direction=lfsr_prand()&1;
		write_LEDs(0);
		if(++counter==STATE_PAUSE)
		{
			++data;
			counter=0;
		}
		return;
	}

	// indicate which side it will be coming from
	if(data & 1)
	{
		write_LEDs(direction ? _BV(4) : 1);
		if(counter==0)
			start_tone(TONE_START);
	}
	else
	{
		write_LEDs(0);
		if(counter==0)
			stop_tone();
	}

	if(++counter==STATE_PAUSE)
	{
		if(++data==7)
			AdvanceState();
		counter=0;
	}
}

void DisplayScore(uint8_t score, bool high_score)
{
	if(uint16_t sw=read_switches())
	{
		// Seed the random number generator based on the timer counter
		// value since start.  The timing is from an external source
		// and will provide a difference sequence each time, but this
		// doesn't use the timer directly, it uses it to select a
		// seed that has the same number of one's as zero's.
		if(!prand)
		{
			switch(TCNT0 & 7)
			{
			case 0:
				prand=0b01001110;
				break;
			case 1:
				prand=0b10110001;
				break;
			case 2:
				prand=0b01000111;
				break;
			case 3:
				prand=0b11000101;
				break;
			case 4:
				prand=0b10011001;
				break;
			case 5:
				prand=0b01101001;
				break;
			case 6:
				prand=0b01101010;
				break;
			case 7:
				prand=0b11000011;
				break;
			}
		}
		if(sw == (_BV(0) | _BV(4)))
		{
			// clear high score
			eeprom_write_byte(&HighScore, 0);
		}
		AdvanceState();
		return;
	}

	// high score is a double sweep right, current is left
	if(data<10)
	{
		uint16_t led=0;
		if(high_score)
		{
			switch(data)
			{
			case 0:
				led=0b0000100001;
				break;
			case 1:
				led=0b0001000010;
				break;
			case 2:
				led=0b0010000100;
				break;
			case 3:
				led=0b0100001000;
				break;
			case 4:
				led=0b1000010000;
				break;
			}
		}
		else
		{
			switch(data)
			{
			case 0:
				led=0b1000010000;
				break;
			case 1:
				led=0b0100001000;
				break;
			case 2:
				led=0b0010000100;
				break;
			case 3:
				led=0b0001000010;
				break;
			case 4:
				led=0b0000100001;
				break;
			}
		}
		write_LEDs(led);
		++counter;
		// fast for the sweep, then more of a pause
		if((data<=4 && counter==FAST_MOVING) || counter==MOVING_TIMEOUT)
		{
			++data;
			counter=0;

		}
		return;
	}

	// sweep to 10, pause, display 10's, pause, sweep for 1's, pause,
	// 1's

	// puase
	if(data==15 || data == 17 || data == 23)
	{
		write_LEDs(0);
		if(++counter==STATE_PAUSE)
		{
			++data;
			counter=0;
		}
		return;
	}
	// display 10's then 1's digits
	if(data == 16 || data == 24)
	{
		uint16_t led;
		uint8_t digit;
		uint8_t hundreds=0;
		if(data==16)
		{
			digit=score/10;
			hundreds=digit/10;
			digit%=10;
		}
		else
		{
			digit=score%10;
		}
		if(!digit)
		{
			led=0;
		}
		else
		{
			led=1<<(digit-1); // display is ones based
			// use the 10 led for hundreds
			led|=hundreds<<9;
		}
		write_LEDs(led);
		if(++counter==STATIC_TIMEOUT)
		{
			++data;
			// restart to the start until someone presses a button
			if(data==25)
			{
				// less this is a one shot score display
				if(!high_score)
					AdvanceState();
				data=0;
			}
			counter=0;
		}
		return;
	}
	// sweep from 6 to 10, indicating this will be the 10's digit
	// then 5 to 1, indicating this will be the 1's digit
	uint16_t led;
	if(data < 15)
		led=1<<(5+data-10);
	else
		led=1<<(12-(data-10));
	write_LEDs(led);
	if(++counter==FAST_MOVING)
	{
		++data;
		counter=0;
	}
}

void DisplayHighScore()
{
	DisplayScore(eeprom_read_byte(&HighScore), true);
}

void DisplayCurrentScore()
{
	DisplayScore(CurrentScore, false);
}

void task_dispatch()
{
	switch(State)
	{
	// just a pause
	case START_HERE:
		if(++counter > STATIC_TIMEOUT)
			SetState(HIGH_SCORE);
		break;
	case COUNT_DOWN:
		CountDown();
		break;
	case GAME_LOOP:
		RunGame();
		break;
	case CAPTURED:
		Captured();
		break;
	case FAIL_TURN:
		FailTurn();
		break;
	// repeat three times
	case CURRENT_SCORE1:
	case CURRENT_SCORE2:
	case CURRENT_SCORE3:
		DisplayCurrentScore();
		break;
	case HIGH_SCORE:
		DisplayHighScore();
		break;
	case RESTART:
		tries=3;
		CurrentScore=0;
		SetState(COUNT_DOWN);
		break;
	case NEW_HIGH_SCORE:
		DisplayNewHighScoreAnim();
		break;
	default:
	case GO_TO_FIRST:
		SetState(HIGH_SCORE);
		break;
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
	// runs the main loop through once
	tickFlag = 1;
}

// Called when Timer1 counter matches OCR1A
ISR(TIMER1_COMPA_vect)
{
	// Toggle the speaker pins to make a click.
	PORTD ^= SPKR_MASK;

	// increase the pitch at each tick
	if(CaptureTone && OCR1A > 50)
		--OCR1A;
}
