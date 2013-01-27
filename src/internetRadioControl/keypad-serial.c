/*
 * Keypad.c
 *
 * Created: 12/17/2012 1:46:28 PM
 *  Author: ASchamp
 */ 

#define F_CPU 11059200
#define USART_BAUDRATE 38400
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE * 16UL))) - 1)

#include <stdint.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

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
	_delay_us( 100 );
	// grab the values from port B
	result |= (PINB << 8);
	// disable bus output for the switches
	PORTD |= (1 << SW_B_READ_OUTPUTENABLE);

	// the switches are low when pressed; the values are inverted here
	// so that the rest of the program can treat 1=pressed.
	return ~result;
}

uint16_t readSwitchesDebounce() {
	uint16_t sw = readSwitches();
	_delay_ms(1);
	return sw & readSwitches();
}

/* Initialize UART */
void InitUART ()
{
	// Set the baud rate //
	UBRRH = (BAUD_PRESCALE >> 8);
	UBRRL = BAUD_PRESCALE;

	// Enable UART receiver and transmitter, 
	// enable receive interrupt 
	UCSRB = (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);

	// set to 8 data bits, 1 stop bit 
	UCSRC = (1 << UCSZ1) | (1 << UCSZ0);
	
	// enable global interrupts
	sei();
}
/*
uint8_t UARTByteAvail()
{
	return UCSRA & (1<<RXC);
}

uint8_t UARTReadByte() 
{ 
	// blocking
    while(!(UCSRA & (1<<RXC))) {}
    return UDR;
}
*/
void UARTWriteByte(uint8_t data) { 
	// blocking
    while(!(UCSRA & (1<<UDRE))) {}
    UDR=data;
	// wait until transmit complete
	while(!(UCSRA & (1 << TXC))) {}
}

// globals for maintaining state of leds and switches
static uint16_t switches = 0;
//static uint16_t leds     = 0;

// send a command for a given switch being pressed or released
// bit 0 = switch 10
void UARTWriteCmd(uint8_t bit, uint8_t pressed) {
	if (pressed) {
		UARTWriteByte('P');
	} else {
		UARTWriteByte('R');
	}
	UARTWriteByte(bit + 0x30); // convert switch number to character
	UARTWriteByte('\n');
}

// read a command for a given LED to be illuminated
// L# means turn on LED #
// l# means turn off LED #
// bit 0 = led 10
/*uint16_t UARTReadCmd() {
	uint8_t data = 0;
	if (UARTByteAvail()) {
		data = UARTReadByte();
		UARTWriteByte('R');
		UARTWriteByte(data);
	} else {
		UARTWriteByte('X');
	}
	if (data == 'L') {
		uint8_t b = UARTReadByte() - 0x30; // convert character to numeral
		leds |= (1 << b);
	} else if (data == 'l') {
		uint8_t b = UARTReadByte() - 0x30; // convert character to numeral
		leds &= ~(1 << b);
	}
	return leds;
}
*/

int main(void)
{
	// configure port B
	DDRB  = 0;
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


	// 36 for 19200, 18 for 38400
	InitUART(); // 11.0592 MHz xtal / (16 * 38400 baud)

	//uint8_t c = 'A';
    while(1)
    {
		// read serial & write leds
		//uint16_t newLeds = UARTReadCmd();
		//leds = newLeds;
		//writeLEDs(newLeds);
		
		// read switches
		uint16_t newSwitches = readSwitchesDebounce();
		uint16_t changedOn  = (newSwitches ^ switches) & newSwitches;
		uint16_t changedOff = (newSwitches ^ switches) & switches;
		for (uint8_t i = 0; i < 10; i++) {
			if (changedOn & (1 << i)) {
				UARTWriteCmd(i, 1);
				writeLEDs(1 << i);
			} else if (changedOff & (1 << i)) {
				UARTWriteCmd(i, 0);
			}						
		}
		switches = newSwitches;
		
		// use xor to toggle
		// "(switches ^ switchDebounce) & switchDebounce" =
		// "the switches that changed and are now active"
		//leds ^= changedOn;
		//writeLEDs( leds );
		/*
		UARTWriteChar(c);
		c++;
		if (c >= 'z') {
			c = 'A';
		}
	
		_delay_ms(500);
		*/
		// echo one char at a time
		/*if (UARTByteAvail()) {
			uint8_t c = UARTReadByte();
			UARTWriteByte(c);
		}		
*/
    }
}

/* not working,  HW problem?
ISR(UASRT_RXC_Vect)
{
	uint8_t byte = UDR;
	UDR = byte;
}
*/