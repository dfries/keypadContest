/*
This program is meant for use with an Atmel ATTiny2313, installed in a
Hall Research KP2B keypad. See hallResearchKP2B-reverseEngineering.txt
for details on the hardware.
Copyright 2012 Tom Judkins. Released under the GPL v2. See COPYING.
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
#define VALID_LEVELS_MASK 0b111
#define VALID_BONGOS_MASK 0b1000010000
#define VALID_LEVELS 3

unsigned char temp = 0;

// Controls the state of all ten LEDs by using a uint16 as a on/off bit array. 1 = LED on
// LEDs will remain on until further calls are made.
//
void writeLEDs( uint16_t values )
{
  DDRB = 0b11111111;                        // Enable Bus output mode
  PORTB = ~(values & 0xff);                 // Load values for first 8 LEDS (active low)
  PORTD |= (1 << LED_A_WRITE_LATCH);        // Latch LED state
  PORTD &= ~(1 << LED_A_WRITE_LATCH);       // Disable Latch
  PORTB = ~(values >> 8);                   // Load values for last 2 LEDs (active low)
  PORTD |= (1 << LED_B_WRITE_LATCH);        // Latch LED state
  PORTD &= ~(1 << LED_B_WRITE_LATCH);       // Disable Latch
  PORTB = 0;                                // Clear output Values
  DDRB = 0;                                 // Leave Bus in input (HiZ) mode
}


// Reads values of specified switches, and returns the result as a bit-array 1= LED on
// Read takes 2us
//
uint16_t readSwitches( uint16_t switchMask )
{
  uint16_t result = 0;

  PORTD &= ~(1 << SW_A_READ_OUTPUTENABLE);  // Enable Latch Output for first 8 switches (active low)
  asm volatile("nop");                      // Mandatory wait before Latch Output is valid
  result |= PINB;                           // Read first 8 switche states
  PORTD |= (1 << SW_A_READ_OUTPUTENABLE);   // Disable Latch Output (active low)
  PORTD &= ~(1 << SW_B_READ_OUTPUTENABLE);  // Enable Latch Output for last 2 switches (active low)
  asm volatile("nop");                      // Mandatory wait before Latch Output is valid
  result |= (PINB << 8);                    // Read last 2 switche states
  PORTD |= (1 << SW_B_READ_OUTPUTENABLE);   // Disable Latch Output (active low)
  result = ~result;                         // Invert result (switches are N-C)
  result &= switchMask;                     // Mask switch output
  return result;
}

// Converts switch bitmap to switch int.
// Returns 0 if no switch pressed.
// Returns multRet if multiple switches pressed.
//
uint8_t switchToUint8( uint16_t inSwitch, uint8_t multRet )
{
  if      ((inSwitch & 0x3ff) == 0b0000000001) return 1;
  else if ((inSwitch & 0x3ff) == 0b0000000010) return 2;
  else if ((inSwitch & 0x3ff) == 0b0000000100) return 3;
  else if ((inSwitch & 0x3ff) == 0b0000001000) return 4;
  else if ((inSwitch & 0x3ff) == 0b0000010000) return 5;
  else if ((inSwitch & 0x3ff) == 0b0000100000) return 6;
  else if ((inSwitch & 0x3ff) == 0b0001000000) return 7;
  else if ((inSwitch & 0x3ff) == 0b0010000000) return 8;
  else if ((inSwitch & 0x3ff) == 0b0100000000) return 9;
  else if ((inSwitch & 0x3ff) == 0b1000000000) return 10;
  else if ((inSwitch & 0x3ff) == 0) return 0;
  else return multRet;
}

void BlinkLeds( const uint8_t numOfTimes )
{
  // blink all leds 2 times
  writeLEDs( 0x0000 );
  uint8_t ii = 0;
  for (ii = 0; ii < numOfTimes; ++ii)
  {
  _delay_ms( 500 );
  writeLEDs( 0xffff );
  _delay_ms( 500 );
  writeLEDs( 0x0000 );
  }
}

void Initialize( void )
{
  // This lets the Makefile set F_CPU to the Hz requested and keeps
  // the delay calculations consistent.
  // It expands to 6 bytes of program text (ATtiny2313).
  CPU_PRESCALE(inline_cpu_hz_to_prescale(F_CPU));

  // configure port B
  DDRB = 0;
  PORTB = 0; // disable internal pull-ups

  // configure port D
  DDRD  = (1 << LED_A_WRITE_LATCH) |        // Set as Output
          (1 << LED_B_WRITE_LATCH) |        // Set as Output
          (1 << SW_A_READ_OUTPUTENABLE) |   // Set as Output
          (1 << SW_B_READ_OUTPUTENABLE);    // Set as Output
  PORTD = (1 << SW_A_READ_OUTPUTENABLE) |   // Output = 1 (Latch OutputEnable is active low)
          (1 << SW_B_READ_OUTPUTENABLE);    // Output = 1 (Latch OutputEnable is active low)

  TCCR1B |= ((1 << CS10) | (1 << CS11)); // Set up timer at Fcpu/64 = 125 kHz
}


int main(void)
{
  uint16_t switchValues;
  uint16_t prevSwitchValues;
  uint16_t ledValues;
  uint16_t period;
  uint8_t beatsA[9] = {0b00010001, // 1
                        0b00010001, // 2
                        0b00000000, // 3
                        0b00000000, // 4
                        0b01010101, // 5
                        0b00000000, // 6
                        0b00010001, // 7
                        0b00010001, // 8
                        0b00000000};// 9

  uint8_t beatsB[9] = {0b00000000, // 1
                        0b00000000, // 2
                        0b00010001, // 3
                        0b00010001, // 4
                        0b00000000, // 5
                        0b01010101, // 6
                        0b01000100, // 7
                        0b00010100, // 8
                        0b00000000};// 9

//uint8_t beatsB[16] = {0b00000000, // 1
//                      0b00000000, // 2
//                      0b00010001, // 3
//                      0b00010001, // 4
//                      0b00000000, // 5
//                      0b01010101, // 6
//                      0b01000100, // 7
//                      0b00010100};// 8
//                      0b00000000, // 9
//                      0b00000000, // 10
//                      0b00000000, // 11
//                      0b00000000, // 12
//                      0b00000000, // 13
//                      0b00000000, // 14
//                      0b00000000, // 15
//                      0b00000000};// 16
  uint8_t  sel_level;
  uint8_t ii;
  uint8_t jj;
  Initialize();

  // Test LEDs
  BlinkLeds(2);

  while(1)
  {
    // Let user select what level they want to play...
    sel_level = 0;
    ledValues = VALID_LEVELS_MASK;
    writeLEDs( ledValues );
    TCNT1 = 0;
    while (sel_level == 0)
    {
      if (TCNT1 >= 6250)  // This should make the LEDs flash at around 10Hz
      {
        ledValues ^= VALID_LEVELS_MASK; // Toggle LEDs
        writeLEDs( ledValues );
        TCNT1 = 0;
      }
      sel_level = switchToUint8( readSwitches( VALID_LEVELS_MASK ), 1 );
    }

    // TODO Set Period for each Level
    period = 12500;

    // Countdown 5-4-3-2-1
    writeLEDs( 0 ); // All LEDs off
    _delay_ms( 500 );
    ledValues = 0x03ff; // All LEDs on
    TCNT1 = 0;
    for (ii = 0; ii<5; ii++)
    {
      writeLEDs( ledValues );
      while (TCNT1 < period)
      {
        asm volatile("nop");
      }
      ledValues = ledValues <<1 & 0x03de;
      TCNT1 = 0;
    }

    prevSwitchValues = readSwitches( VALID_BONGOS_MASK );
    for (ii = 0; ii<9; ii++)
    {
      for (jj = 0; jj<8; jj++)
      {
        ledValues = (ledValues <<1 & 0x03de); // Shift all beats up one
        ledValues |= (beatsA[ii]>>jj & 0x01);
        ledValues |= (beatsB[ii]>>jj & 0x01) << 5;
        writeLEDs( ledValues );
        while (TCNT1 < period)
        {
                  switchValues = readSwitches( VALID_BONGOS_MASK );
                  if (switchValues != prevSwitchValues)
                  {
                    uint16_t hits = 0;
                        // Check for Right Bongo Hits
                        if ((switchValues & 0x10) != 0 &&
                            (ledValues & 0x10)    != 0 &&
                                TCNT1 < 5000)
                        {
                      hits = 0x001f;
                        }
//                      else if (switchValues & 0x10 != 0 &&
//                          (ledValues & 0x10 != 0 && TCNT1 < 4000) ||
//                              (ledValues & 0x08 != 0 && TCNT1 > 8500))
//                      {
//                    hits = 0x001c;
//                      }

                        // Check for Left Bongo Hits
                        if ((switchValues & 0x200) != 0 &&
                            (ledValues & 0x200)    != 0 &&
                                TCNT1 < 5000)
                        {
                      hits |= 0x03e0;
                        }
//                      else if (switchValues & 0x200 != 0 &&
//                          (ledValues & 0x200 != 0 && TCNT1 < 4000) ||
//                              (ledValues & 0x100 != 0 && TCNT1 > 8500))
//                      {
//                    hits |= 0x0380;
//                      }

                        if (hits != 0)
                        {
                          writeLEDs( hits );
                          _delay_ms( 50 );
                          writeLEDs( ledValues );
                        }
                        prevSwitchValues = switchValues;
                  }
        }
        TCNT1 = 0;
      }
    }
  }
  return 0;
}
