/*
This program is meant for use with an Atmel ATTiny2313, installed in a 
Hall Research KP2B keypad.  See hallResearchKP2B-reverseEngineering.txt 
for details on the hardware.

This program will take a variable number of switch presses, and then repeat
that pattern until any other switch is pressed.  Below is the flow.
(1) On application of power/reset, all of the LEDs are turned on then off one
    after another from 1 to 10. This is then repeated.
(2) All of the LEDs flash twice.
(3) The code then waits for a user to select one of the 10 switchs.  The
    number of the switch represents the number of entries in the pattern.
(4) After the number of entries is selected, all of the LEDs flash once
    to indicate that the user can begin entering a pattern.
(5) Once the user has entered all of the required entries, the LEDs will be
    lit in that pattern, and will repeat indefinitely.
(6) If the user hits any switch, the pattern will stop and all LEDs will
    flash twice.  Then go back to step (3).

Copyright 2012 Chris Ciszewski.  Released under the GPL v2.  See COPYING.
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

unsigned char temp = 0;

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

uint16_t readSwitchesDebounce( void )
{
	uint16_t result = 0;
	// Simple debounce: read the switches, pause, and read them again.
	// Only count the switches that were pressed at both samplings as 
	// being pressed.
	result = readSwitches();
	_delay_ms( 10 );
	result &= readSwitches();
	result &= VALID_SWITCHES_MASK;
	return result;
}

uint8_t SwitchToUint8( uint16_t inSwitch )
{
	     if (inSwitch == 0b0000000001)	return 1;
	else if (inSwitch == 0b0000000010)	return 2;
	else if (inSwitch == 0b0000000100)	return 3;
	else if (inSwitch == 0b0000001000)	return 4;
	else if (inSwitch == 0b0000010000)	return 5;
	else if (inSwitch == 0b0000100000)	return 6;
	else if (inSwitch == 0b0001000000)	return 7;
	else if (inSwitch == 0b0010000000)	return 8;
	else if (inSwitch == 0b0100000000)	return 9;
	else if (inSwitch == 0b1000000000)	return 10;
	else					return 0;
}

void BlinkLeds( const uint8_t numOfTimes )
{
	// blink all leds 2 times
	writeLEDs( 0x0000 );
	uint8_t ii = 0;
	for (ii = 0; ii < numOfTimes; ++ii)
	{
		_delay_ms( 100 );
		writeLEDs( 0xffff );
		_delay_ms( 100 );
		writeLEDs( 0x0000 );
	}
}

void WaitForSwitchInput( uint16_t inSwitches )
{
	while (readSwitchesDebounce() != inSwitches)
	{
	}
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
	
	/*
	// BTW, as a curious side-effect of how the bus is wired, you can give 
	// control of the bus to the switch latch.  If you do this, and 
	// latch-enable one of the LED latches, the switch latch will write 
	// directly to the LED latch.  Pressing the switches will light up the LEDs
	// with no intervention required on the part of the microcontroller.  
	// Here's how to do that:
	PORTD |= (1 << LED_A_WRITE_LATCH);
	PORTD &= ~(1 << SW_A_READ_OUTPUTENABLE);
	while( 1 ) {}
	*/

	// led chaser sequence
	uint8_t ii;
	for( ii = 0 ; ii < 2; ii++ )
	{
		uint16_t leds = 1;
		// stop after all 10 leds have been illuminated
		while( leds < (1<<11) )
		{
			writeLEDs( leds );

			_delay_ms( 100 );

			leds = leds << 1;
		}
	}

	uint16_t switchDebounce;
//	uint16_t leds = 0;
	uint8_t level = 0;  // The number of entries to take
	uint16_t switchPattern[10];
	// read switches and toggle leds
	while( 1 )
	{
		// blink all leds 2 times
		BlinkLeds(2);

		// Initialize stuff
		for( ii = 0; ii < 10; ++ii)
		{
			switchPattern[ii] = 0;
		}

		// Wait for switch press which specifies the number of switch presses
		//  to include in the pattern
		switchDebounce = 0;
		while (switchDebounce == 0)
		{
			switchDebounce = readSwitchesDebounce();
		}

		level = SwitchToUint8(switchDebounce);
		// Turn on the LED for the pressed switch for a brief time to give
		//  user feedback on their selected item
		writeLEDs( switchDebounce );
		_delay_ms( 1000 );
		writeLEDs( 0 );

		// Blink Leds to inform user that pattern is ready to be entered
		BlinkLeds(1);

		// record pattern of up to 10 lights
		for ( ii = 0; ii < level; ++ii)
		{
			// Wait until all switches are released
			WaitForSwitchInput(0);
			// Now detect a switch push and save it
			switchDebounce = 0;
			while (switchDebounce == 0)
			{
				switchDebounce = readSwitchesDebounce();
			}
			switchPattern[ii] = switchDebounce;
			writeLEDs( switchDebounce );
			WaitForSwitchInput(0);  // Wait until all switches are released
			writeLEDs( 0 );
		}

		// Wait until all switches are released to begin repeating the pattern
		WaitForSwitchInput(0);

		// Now start repeating pattern until another switch is pressed
		switchDebounce = 0;
		while (switchDebounce == 0)
		{
			for ( ii = 0; ii < level; ++ii)
			{
				writeLEDs(switchPattern[ii]);
				// now check for a switch press between each pattern item
				//  to allow the user to exit the repeating pattern
				int8_t jj = 0;
				for (jj = 0; jj < 10; ++jj)
				{
					switchDebounce = readSwitchesDebounce();
					if ( switchDebounce != 0 )
					{
						// clear the LEDs
						writeLEDs(0);
						break;
					}
				}
			}
		}
		
		// Wait for all switchs to be released
		WaitForSwitchInput(0);
	}

	return 0;
}
