/*
This program is meant for use with an Atmel ATTiny2313, installed in a 
Hall Research KP2B keypad.  See hallResearchKP2B-reverseEngineering.txt 
for details on the hardware.

This program is an example of how to turn the LEDs on and off.

Copyright 2012 Andrew Sampson.  Released under the GPL v2.  See COPYING.
*/


#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>

#define LED_A_WRITE_LATCH PD2
#define LED_B_WRITE_LATCH PD3
#define SW_A_READ_OUTPUTENABLE PD4
#define SW_B_READ_OUTPUTENABLE PD5

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


int main(void)
{
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
	uint8_t i;
	for( i = 0 ; i < 5; i++ )
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

	// blink all leds 3 times
	writeLEDs( 0x0000 );
	_delay_ms( 500 );
	writeLEDs( 0xffff );
	_delay_ms( 500 );
	writeLEDs( 0x0000 );
	_delay_ms( 500 );
	writeLEDs( 0xffff );
	_delay_ms( 500 );
	writeLEDs( 0x0000 );
	_delay_ms( 500 );
	writeLEDs( 0xffff );
	_delay_ms( 500 );
	writeLEDs( 0x0000 );
	
	// alternate every other led indefinitely
	while( 1 )
	{
		_delay_ms( 1000 );
		writeLEDs( 0xaaaa );
		_delay_ms( 1000 );
		writeLEDs( ~0xaaaa );
	}
	
	return 0;
}
