/*
This program is meant for use with an Atmel ATTiny2313, installed in a 
Hall Research KP2B keypad.  See hallResearchKP2B-reverseEngineering.txt 
for details on the hardware.

This program is an example of how to read input from the switches.

Copyright 2012 Andrew Sampson.  Released under the GPL v2.  See COPYING.
*/


#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>
#include "../../include/ATtiny2313_clock.h"

#define LED_A_WRITE_LATCH PD2
#define LED_B_WRITE_LATCH PD3
#define SW_A_READ_OUTPUTENABLE PD4
#define SW_B_READ_OUTPUTENABLE PD5
#define SPKR_PIN_1 PD1
#define SPKR_PIN_2 PD6
#define SPKR_MASK ((1<<SPKR_PIN_1)|(1<<SPKR_PIN_2))

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
		(1 << SW_B_READ_OUTPUTENABLE) | SPKR_MASK;
	// one speaker pin high the other is already low
	PORTD = _BV(SPKR_PIN_1);
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

	// Verify CPU speed and delay times, should take 4 seconds to
	// execute.
	#if 0
	for(uint8_t i=0; i<2; ++i)
	{
		writeLEDs(0b1111100000 | CLKPR);
		_delay_ms( 1000 );
		writeLEDs(CLKPR);
		_delay_ms( 1000 );
	}
	#endif


	for(;;)
	{
		for(uint16_t i=1; i<16000; ++i)
		{
			PORTD ^= SPKR_MASK;
			//writeLEDs(i);
			/*
			for(uint16_t j=0; j<i; ++j)
				_delay_us(1);
				*/
			_delay_us(i);
		}
	}

	#if 0
	for(uint16_t binary=0; binary<1024; ++binary)
	{
			uint8_t low = binary >> 8;
			low = low >> 1 | (low&1) << 1;
			uint8_t high = binary & 3;
			high = high >> 1 | (high&1) << 1;
			uint8_t mid = binary;
			mid =
					(mid & 0b00000100) << 5 |
					(mid & 0b00001000) << 3 |
					(mid & 0b00010000) << 1 |
					(mid & 0b00100000) >> 1 |
					(mid & 0b01000000) >> 3 |
					(mid & 0b10000000) >> 5;
			uint16_t out = (low | mid) | high<<8;
			writeLEDs( out );
			_delay_ms( 10 );
	}

	for(uint8_t prescale=0; prescale<=CPU_31250Hz; ++prescale)
	{
		uint8_t i;
		int delay=128>>prescale;
		CPU_PRESCALE(prescale);
		writeLEDs(0b1111100000 | prescale);
		//for(i=0; i<delay; ++i)
			_delay_ms(100);
		// led chaser sequence
		for( i = 0 ; i < 8; i++ )
		{
			uint16_t leds = 1;
			// stop after all 10 leds have been illuminated
			while( leds < (1<<11) )
			{
				writeLEDs( leds );

				_delay_ms( 40 );

				leds = leds << 1;
			}
		}
	}
	#endif

#if 0
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

	// blink all leds 2 times
	writeLEDs( 0x0000 );
	_delay_ms( 40 );
	writeLEDs( 0xffff );
	_delay_ms( 40 );
	writeLEDs( 0x0000 );
	_delay_ms( 40 );
	writeLEDs( 0xffff );
	_delay_ms( 41 );
	writeLEDs( 0x0000 );
#endif

	uint16_t switches = 0;
	// read switches and update leds (momentary style), 
	// until user presses buttons 1 and 5
	while( (switches & VALID_SWITCHES_MASK) != 0b0000010001 )
	{
		switches = readSwitches();
		writeLEDs( switches );
	}
	
	uint16_t switchDebounce;
	uint16_t leds = 0;
	// read switches and toggle leds
	while( 1 )
	{
		// Simple debounce: read the switches, pause, and read them again.
		// Only count the buttons that were pressed at both samplings as 
		// being pressed.
		switchDebounce = readSwitches();
		_delay_ms( 10 );
		switchDebounce &= readSwitches();
		
		// use xor to toggle
		// "(switches ^ switchDebounce) & switchDebounce" = 
		// "the switches that changed and are now active"
		leds ^= (switches ^ switchDebounce) & switchDebounce;
		writeLEDs( leds );
		
		switches = switchDebounce;
	}
	
	return 0;
}
