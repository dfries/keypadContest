/*
This program is meant for use with an Atmel ATTiny2313, installed in a
Hall Research KP2B keypad.  See hallResearchKP2B-reverseEngineering.txt
for details on the hardware.

This program is an example of how to read input from the switches.

Copyright 2012 Andrew Sampson.
Copyright 2012 David Fries.
Released under the GPL v2.  See COPYING.
*/


#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include "../../include/ATtiny2313_clock.h"

#define LED_A_WRITE_LATCH PD2
#define LED_B_WRITE_LATCH PD3
#define SW_A_READ_OUTPUTENABLE PD4
#define SW_B_READ_OUTPUTENABLE PD5

#define VALID_SWITCHES_MASK 0b1111111111

/*
Uses port B to talk to the LED latches.
Upon exiting, leaves port B in high-impedence state.
*/
void writeLEDs( uint16_t values )
{
	// The LEDs are connected between U3/U5 and VCC.  Thus, they illuminate
	// when the outputs on U3/U5 go *low*.  The LED values are inverted here,
	// so that the rest of the program can treat 1="LED on" and 0="LED off".
	uint8_t highByte = ~(values >> 8);
	uint8_t lowByte = ~(values & 0xff);

	// set port B pins as output
	DDRB = 0b11111111;

	PORTB = lowByte;
	PORTD |= (1 << LED_A_WRITE_LATCH);
	PORTD &= ~(1 << LED_A_WRITE_LATCH);
	PORTB = highByte;
	PORTD |= (1 << LED_B_WRITE_LATCH);
	PORTD &= ~(1 << LED_B_WRITE_LATCH);
	PORTB = 0;

	// set port B pins as input
	DDRB = 0;
	PORTB = 0; // disable internal pull-ups
}

/*
Uses port B to talk to the switch latches.
Upon exiting, leaves port B in high-impedence state.
*/
uint16_t readSwitches( void )
{
	uint16_t result = 0;

	// set port B pins as input
	DDRB = 0;
	PORTB = 0; // disable internal pull-ups

	// keep in mind that the latch OutputEnable is active low

	// enable bus output for the first bank of switches
	PORTD &= ~(1 << SW_A_READ_OUTPUTENABLE);
	// I've found that a delay is necessary in order for the switch values
	// to appear on the bus.  This delay could possibly be shortened to
	// just a few nops.
	_delay_ms( 1 );
	// grab the values from port B
	result |= PINB;
	// disable bus output for the switches
	PORTD |= (1 << SW_A_READ_OUTPUTENABLE);

	// enable bus output for the second bank of switches
	PORTD &= ~(1 << SW_B_READ_OUTPUTENABLE);
	// I've found that a delay is necessary in order for the switch values
	// to appear on the bus.  This delay could possibly be shortened to
	// just a few nops.
	_delay_ms( 1 );
	// grab the values from port B
	result |= (PINB << 8);
	// disable bus output for the switches
	PORTD |= (1 << SW_B_READ_OUTPUTENABLE);

	// the switches are low when pressed; the values are inverted here
	// so that the rest of the program can treat 1=pressed.
	return ~result;
}

int main(void)
{
	// This lets the Makefile set F_CPU to the Hz requested and keeps
	// the delay calculations consistent.
	// It expands to 6 bytes of program text (ATtiny2313).
	CPU_PRESCALE(inline_cpu_hz_to_prescale(F_CPU));

	// configure port B
	DDRB = 0;
	PORTB = 0; // disable internal pull-ups

	// configure port D
	DDRD =
		(1 << LED_A_WRITE_LATCH) |
		(1 << LED_B_WRITE_LATCH) |
		(1 << SW_A_READ_OUTPUTENABLE) |
		(1 << SW_B_READ_OUTPUTENABLE);
	PORTD = 0;
	// the latch OutputEnable is active low
	PORTD |= (1 << SW_A_READ_OUTPUTENABLE);
	PORTD |= (1 << SW_B_READ_OUTPUTENABLE);

	// Verify CPU speed and delay times, should take 4 seconds to
	// execute.
	for(uint8_t i=0; i<2; ++i)
	{
		writeLEDs(0b1111100000 | CLKPR);
		_delay_ms( 1000 );
		writeLEDs(CLKPR);
		_delay_ms( 1000 );
	}

	// blink all leds 2 times
	writeLEDs( 0x0000 );
	_delay_ms( 100 );
	writeLEDs( 0xffff );
	_delay_ms( 100 );
	writeLEDs( 0x0000 );
	_delay_ms( 100 );
	writeLEDs( 0xffff );
	_delay_ms( 100 );
	writeLEDs( 0x0000 );

	uint16_t switches=0, last=0;
	uint16_t leds=1;
	for(;;)
	{
		switches = readSwitches();
		// update the CPU clock speed on press, button 1 through 9
		// the higher the number the slower the speed
		if(switches != last)
		{
			last = switches;
			int8_t prescale;
			switch(switches)
			{
			case 0b00000001:
				prescale=1;
				break;
			case 0b00000010:
				prescale=2;
				break;
			case 0b00000100:
				prescale=3;
				break;
			case 0b00001000:
				prescale=4;
				break;
			case 0b00010000:
				prescale=5;
				break;
			case 0b00100000:
				prescale=6;
				break;
			case 0b01000000:
				prescale=7;
				break;
			case 0b10000000:
				prescale=8;
				break;
			default:
				prescale=0;
			}
			CPU_PRESCALE(prescale);
		}

		// led chaser sequence
		writeLEDs(leds);
		//_delay_ms(20);
		leds = leds << 1;
		if(leds >= 1<<10)
			leds=1;
	}
}
