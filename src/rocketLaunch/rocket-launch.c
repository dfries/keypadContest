/*
This program is meant for use with an Atmel ATTiny2313, installed in a 
Hall Research KP2B keypad.  See hallResearchKP2B-reverseEngineering.txt 
for details on the hardware.

This program is forked from keypad-input Copyright 2013 by Andrew Sampson
  David Hans
  Released under the GPL v2. 

*/



#include <stdint.h>
#include <avr/io.h>


#define LED_A_WRITE_LATCH PD2
#define LED_B_WRITE_LATCH PD3
#define SW_A_READ_OUTPUTENABLE PD4
#define SW_B_READ_OUTPUTENABLE PD5

#define VALID_SWITCHES_MASK 0b1111111111
#define VALID_LED_MASK 0b1111111111


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
	asm volatile("nop");
	// grab the values from port B
	result |= PINB;
	// disable bus output for the switches
	PORTD |= (1 << SW_A_READ_OUTPUTENABLE);
	
	// enable bus output for the second bank of switches
	PORTD &= ~(1 << SW_B_READ_OUTPUTENABLE);
	// I've found that a delay is necessary in order for the switch values 
	// to appear on the bus.  This delay could possibly be shortened to 
	// just a few nops.
	asm volatile("nop");
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
	
	
	uint16_t switches = 0;
	uint16_t leds = 0;
	uint16_t timer = 0;
	uint8_t count = 10;
	uint16_t armed_blink_rate = 2000;
	uint16_t one_sec_rate = 20000;  // Just a guess
	uint16_t ten_button = 0b1000000000;
	
	// read switches and update leds (momentary style), 
	// until user enters the password 1 2 3 4 
	
while( 1 )
	{
//  very crude way to do this for now

		while( (switches & VALID_SWITCHES_MASK) != 0b0000000001 )
		{
			switches = readSwitches();
			writeLEDs( switches );
		}
		while( (switches & VALID_SWITCHES_MASK) != 0b0000000010 )
		{
			switches = readSwitches();
			writeLEDs( switches );
		}
		while( (switches & VALID_SWITCHES_MASK) != 0b0000000100 )
		{
			switches = readSwitches();
			writeLEDs( switches );
		}
		while( (switches & VALID_SWITCHES_MASK) != 0b0000001000 )
		{
			switches = readSwitches();
			writeLEDs( switches );
		}
		
		
	// blink all leds until switch 10 is pressed
	// This the armed state waiting for countdown to start	

		switches = readSwitches();
		timer = 0;
		leds = 0;
		writeLEDs( leds );

		
		while ((switches & VALID_SWITCHES_MASK) != ten_button)
		{
			switches = readSwitches();
			timer++;
			
			if (timer >= armed_blink_rate)
			{
				timer = 0;
				leds = ~ leds;
				writeLEDs( leds );
			}
		}
			
// turn off lights and start countdown  
// 1 button cancels and starts password entry again
// start count at 0 then invert when we turn on lights

		count = 0;
		timer = 0;
		leds= 0;
		writeLEDs ( leds );
		leds = ten_button;
		
		while ((switches & VALID_SWITCHES_MASK) != 0b0000000001)
		{
			switches = readSwitches();
			timer++;
		
			if (timer >= one_sec_rate)
			{
				timer = 0;
				count++;
				writeLEDs( leds );
				leds = (leds >> 1) ^ ten_button;
				
				if (count >= 10)
				{ 
///  launch here ???
// for now just start over				
				break; }
				
			}	
		}
		
		
	}
	return 0;
}	
