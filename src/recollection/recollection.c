/*
This program is my entry in the keypad programming contest.  It is a 
memory game, similar to "Computer Perfection" by Lakeside.  

The goal of the single-player game is to press the buttons in the correct order.
The order changes from round to round, and is revealed to the player at the 
beginning of each round during a "preview" mode.  When in preview mode, the 
user presses each button once (starting at button 1, then 2, and so on) and the 
game will light up the button that activates the corresponding light.  Once 
the preview is finished, gameplay begins.  The player must then press the 
button that activates light 1, then the button that activates light 2, and so 
forth.  Once all the lights are lit, the gameplay ends and the player is shown 
their score.

This program is meant for use with an Atmel ATTiny2313, installed in a 
Hall Research KP2B keypad.  See hallResearchKP2B-reverseEngineering.txt 
for details on the hardware.

Copyright 2012 Andrew Sampson.  Released under the GPL v2.  See COPYING.
*/


#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include <util/delay.h>

#define LED_A_WRITE_LATCH PD2
#define LED_B_WRITE_LATCH PD3
#define SW_A_READ_OUTPUTENABLE PD4
#define SW_B_READ_OUTPUTENABLE PD5

#define VALID_SWITCHES_MASK 0b1111111111
#define NUM_SWITCHES 10

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
	// to appear on the bus.
	asm("nop\n\t"
		"nop\n\t");
	// grab the values from port B
	result |= PINB;
	// disable bus output for the switches
	PORTD |= (1 << SW_A_READ_OUTPUTENABLE);
	
	// enable bus output for the second bank of switches
	PORTD &= ~(1 << SW_B_READ_OUTPUTENABLE);
	// I've found that a delay is necessary in order for the switch values 
	// to appear on the bus.
	asm("nop\n\t"
		"nop\n\t");
	// grab the values from port B
	result |= (PINB << 8);
	// disable bus output for the switches
	PORTD |= (1 << SW_B_READ_OUTPUTENABLE);
	
	// the switches are low when pressed; the values are inverted here 
	// so that the rest of the program can treat 1=pressed.
	return ~result;
}


#define RAND8 ((uint8_t)(rand() & 0xff))
#define RAND4 ((uint8_t)(rand() & 0x0f))
#define SOLUTION_SENTINEL 0xff

void generateSinglePlayerSolution( uint8_t solution[] )
{
	uint8_t i = 0;
	uint8_t scratch[NUM_SWITCHES];
	
	// initialize the arrays
	for( i = 0; i < NUM_SWITCHES; i++ )
	{
		solution[i] = SOLUTION_SENTINEL;
		scratch[i] = i;
	}
	
	// populate solution[] by selecting random entries from scratch[], copying 
	// them to solution[], and marking them in scratch[] as used
	for( i = 0; i < NUM_SWITCHES; i++ )
	{
		uint8_t selectedIndex = 0;
		// there is the remote chance that this loop could run forever
		do
		{
			selectedIndex = RAND4;
			if( selectedIndex >= NUM_SWITCHES )
				selectedIndex -= NUM_SWITCHES;
		} while( scratch[selectedIndex] == SOLUTION_SENTINEL );
		
		solution[i] = scratch[selectedIndex];
		scratch[selectedIndex] = SOLUTION_SENTINEL;
	}
	
}

void recollectionNormal( uint8_t difficulty )
{
	uint8_t i = 0;
	uint8_t score = 0;
	uint8_t solution[NUM_SWITCHES];

	// generate the solution
	generateSinglePlayerSolution( solution );
	
	// allow the user to preview the solution
	uint16_t shownLights = 0;
	uint16_t switches;
	while( shownLights != VALID_SWITCHES_MASK )
	{
		// Simple debounce: read the switches, pause, and read them again.
		// Only count the buttons that were pressed at both samplings as 
		// being pressed.
		switches = readSwitches();
		_delay_ms( 10 );
		switches &= readSwitches();
		
		if( switches )
		{
			/*
			fixme - enforce the following:
			- the player is penalized for stalling
			- display each light only once?
			- force player to select switches in sequential order?
			- [if a semi-momentary policy is implemented: ]
				- the light is on for a minimum time
				- the light is on for a maximum time
			*/
			
			uint8_t pressedSwitch = 0;
			i = 0;
			while( switches != 0 )
			{
				if( switches & 0x1 )
				{
					pressedSwitch = i;
					break;
				}
				switches = switches >> 1;
				i++;
			}

			writeLEDs( 1 << (solution[pressedSwitch]) );
			shownLights |= 1 << pressedSwitch;
			
			_delay_ms( 1000 );
			writeLEDs( 0 );
		}
	}
	
	// gameplay
	uint8_t step;
	for( step = 0; step < NUM_SWITCHES; step++ )
	{
		uint8_t guessIsCorrect = 0;
		while( !guessIsCorrect )
		{
			// Simple debounce: read the switches, pause, and read them again.
			// Only count the buttons that were pressed at both samplings as 
			// being pressed.
			switches = readSwitches();
			_delay_ms( 10 );
			switches &= readSwitches();
			
			if( switches )
			{
				uint8_t pressedSwitch = 0;
				i = 0;
				while( switches != 0 )
				{
					if( switches & 0x1 )
					{
						pressedSwitch = i;
						break;
					}
					switches = switches >> 1;
					i++;
				}
				
				if( pressedSwitch == solution[step] )
				{
					// player pressed the correct button
					writeLEDs( VALID_SWITCHES_MASK >> (NUM_SWITCHES-(step+1)) );
					guessIsCorrect = 1;
				}
				else
				{
					// player pressed the wrong button; flash the light above 
					// the switch they just pressed
					uint16_t currentLEDs = VALID_SWITCHES_MASK >> (NUM_SWITCHES-step);
					uint16_t currentLEDsWithWrongGuess = currentLEDs ^ (1<<pressedSwitch);
					
					writeLEDs( currentLEDsWithWrongGuess );
					_delay_ms( 250 );
					writeLEDs( currentLEDs );
					_delay_ms( 250 );
					writeLEDs( currentLEDsWithWrongGuess );
					_delay_ms( 250 );
					writeLEDs( currentLEDs );
					_delay_ms( 250 );
					writeLEDs( currentLEDsWithWrongGuess );
					_delay_ms( 250 );
					writeLEDs( currentLEDs );
				}
					
				// user pressed some button, so increment score
				score++;
				
				// wait until user lets go of all switches
				do
				{
					switches = readSwitches();
				} while( switches );
			}
			
		}
	}
	
	// show score
	writeLEDs( score );
	_delay_ms( 7000 );
	
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
	
	// led chaser sequence
	uint8_t i;
	for( i = 0 ; i < 2; i++ )
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
	
	
	while( 1 )
	{
		// menu, to select game mode
		uint8_t gameMode = 0xff;
		_delay_ms( 200 );
		writeLEDs( 0b0000001111 );
		uint16_t switches = 0;
		// wait for the user to select a mode and let go of the buttons
		while( gameMode == 0xff || switches != 0 )
		{
			switches = readSwitches();
			for( i = 0; i < 4; i++ )
			{
				if( (switches >> i) & 0x1 )
				{
					gameMode = i;
					break;
				}
			}
			_delay_ms( 10 );
		}
		writeLEDs( 1 << gameMode );
		
		// difficulty selection
		uint8_t difficulty = 0xff;
		_delay_ms( 200 );
		writeLEDs( (1 << gameMode) | 0b0011100000 );
		switches = 0;
		// wait for the user to select the difficulty and let go of the buttons
		while( difficulty == 0xff || switches != 0 )
		{
			switches = readSwitches();
			for( i = 0; i < 3; i++ )
			{
				if( (switches >> (i+5)) & 0x1 )
				{
					difficulty = i;
					break;
				}
			}
			_delay_ms( 10 );
		}
		writeLEDs( (1 << gameMode) | (1 << (difficulty+5)) );
		
		_delay_ms( 1000 );

		writeLEDs( 0 );

		// run the game mode that was selected
		switch( gameMode )
		{
		case 0:
			recollectionNormal( difficulty );
			break;
		default:
			break;
		}
	}

	return 0;
}
