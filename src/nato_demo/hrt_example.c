// This code demonstrates how to do a few things with the HRT keypad fitted with
//  an ATtiny2313/4313 using a co-operative multitasking framework.  It uses the
//   8-bit timer to measure 1.0 millisecond frames.  With this framework, each
//   task can be prioritized (earlier tasks have higher priority) and can each
//   have independent call rates (see the taskTimers[n]) or assign dependent
//   order and delays of specific tasks (setting taskTimers[n] in a taskm()
//   method different from task #m; no example in this code, but see avrfreaks
//   link for more information).  The tasks demonstrated below:
//    * Read current state of pushbuttons and logically debounce them.
//    * Determine which LEDs should be lit and if the song should be played.
//       This is where the bulk of the game logic is located.
//    * Update the LEDs' commanded states determined in the previous task.
//    * Update the song.
//
// The sound is played with an optional piezo speaker connected directly to pins
//  3 & 4 of the DB-9 connector.  According to the RS-232 transceiver's
//  datasheet, it can be shorted continuously, so this shouldn't be a problem.
//
// The code also demonstrates reading and writing to the EEPROM (see
//  lfsr_prand()).
//
// Original co-operative multitasking framework tutorial code
// (c)Russell Bull 2010. Free for any use.
// Code originally built for a Mega168 @ 16MHz
// See the following for more information:
// http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=95490
//
// Code ported by nato to ATtiny2313 @ 11.0592MHz and added example code to
//  demonstrate how to do some basic things with the HRT keypad.
//
// Low-level code for HRT KP2B keypad to read the switches and write the LEDs
//  adapted from code Copyright 2012 Andrew Sampson.  Released under the GPL v2.

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>

#define F_CPU 11059200UL
#define LED_A_WRITE_LATCH PD2
#define LED_B_WRITE_LATCH PD3
#define SW_A_READ_OUTPUTENABLE PD4
#define SW_B_READ_OUTPUTENABLE PD5
#define VALID_SWITCHES_MASK 0b1111111111
#define VALID_LIGHTS_MASK 0b1111111111
#define SPKR_PIN_1 PD1
#define SPKR_PIN_2 PD6
#define SPKR_MASK ((1<<SPKR_PIN_1)|(1<<SPKR_PIN_2))
#define NUM_TASKS 8
#define NUM_SWITCHES 10
#define NUM_LIGHTS 10
#define NUM_NOTES 128
#define NO_CHANGE 0
#define SONG_ON 1
#define SONG_OFF 2

static inline void init_devices(void);
static inline void timer0_init(void);
static inline void timer1_init(void);
uint8_t lfsr_prand(void);
static inline void task_0_read_switches(void);
static inline void task_1_game_action(void);
static inline void task_2_write_LEDs(void);
static inline void task_3_play_song(void);
//static inline void task_4(void); // Uncomment this if more tasks are required.
//static inline void task_5(void); // Uncomment this if more tasks are required.
//static inline void task_6(void); // Uncomment this if more tasks are required.
//static inline void task_7(void); // Uncomment this if more tasks are required.
static inline void task_dispatch(void);

// The seed stored in NVRAM for the pseudo-random number generator.
uint8_t EEMEM storedSeed = 0b01001110;

// An array of bits that correspond to the logically debounced switch states.
//  The LSB corresponds to the switch labeled "1".  A value of "0" represents a
//  logically debounced button that is currently being depressed.
uint16_t debouncedSwitches = 0;

// An array of bits that correspond to the LEDs' states.  The LSB corresponds to
//  the light labeled "1".  A value of "0" represents a light that is currently
//  on.
uint16_t lightStates = 0;

// The melody consists of 128 notes, played in a loop.  Each note takes 4 bits,
//  or half a byte.  These notes are indexes into notePeriods[].
uint8_t EEMEM melody[]={
        // Sequence Tone Index
 0xA6,  // 1        F#5  0xA
        // 2        C5   0x6
 0xB1,  // 3        G5   0xB
        // 4        G4   0x1
 0xB6,  // 5        G5   0xB
        // 6        C5   0x6
 0xB1,  // 7        G5   0xB
        // 8        G4   0x1
 0xB5,  // 9        G5   0xB
        // 10       B4   0x5
 0xB5,  // 11       G5   0xB
        // 12       B4   0x5
 0xB5,  // 13       G5   0xB
        // 14       B4   0x5
 0xB1,  // 15       G5   0xB
        // 16       G4   0x1
 0xB4,  // 17       G5   0xB
        // 18       Bb4  0x4
 0xB1,  // 19       G5   0xB
        // 20       G4   0x1
 0xB4,  // 21       G5   0xB
        // 22       Bb4  0x4
 0xB1,  // 23       G5   0xB
        // 24       G4   0x1
 0xB3,  // 25       G5   0xB
        // 26       A4   0x3
 0xB1,  // 27       G5   0xB
        // 28       G4   0x1
 0xB3,  // 29       G5   0xB
        // 30       A4   0x3
 0xB1,  // 31       G5   0xB
        // 32       G4   0x1
 0x99,  // 33       E5   0x9
        // 34       E5   0x9
 0x90,  // 35       E5   0x9
        // 36       F4   0x0
 0x72,  // 37       D5   0x7
        // 38       G#4  0x2
 0x99,  // 39       E5   0x9
        // 40       E5   0x9
 0x92,  // 41       E5   0x9
        // 42       G#4  0x2
 0x70,  // 43       D5   0x7
        // 44       F4   0x0
 0x92,  // 45       E5   0x9
        // 46       G#4  0x2
 0x70,  // 47       D5   0x7
        // 48       F4   0x0
 0x99,  // 49       E5   0x9
        // 50       E5   0x9
 0x91,  // 51       E5   0x9
        // 52       G4   0x1
 0x99,  // 53       E5   0x9
        // 54       E5   0x9
 0x90,  // 55       E5   0x9
        // 56       F4   0x0
 0x77,  // 57       D5   0x7
        // 58       D5   0x7
 0x73,  // 59       D5   0x7
        // 60       A4   0x3
 0x77,  // 61       D5   0x7
        // 62       D5   0x7
 0x75,  // 63       D5   0x7
        // 64       B4   0x5
 0x86,  // 65       D#5  0x8
        // 66       C5   0x6
 0x91,  // 67       E5   0x9
        // 68       G4   0x1
 0x96,  // 69       E5   0x9
        // 70       C5   0x6
 0x91,  // 71       E5   0x9
        // 72       G4   0x1
 0x95,  // 73       E5   0x9
        // 74       B4   0x5
 0x91,  // 75       E5   0x9
        // 76       G4   0x1
 0x95,  // 77       E5   0x9
        // 78       B4   0x5
 0x91,  // 79       E5   0x9
        // 80       G4   0x1
 0x94,  // 81       E5   0x9
        // 82       Bb4  0x4
 0x91,  // 83       E5   0x9
        // 84       G4   0x1
 0x94,  // 85       E5   0x9
        // 86       Bb4  0x4
 0x91,  // 87       E5   0x9
        // 88       G4   0x1
 0x93,  // 89       E5   0x9
        // 90       A4   0x3
 0x91,  // 91       E5   0x9
        // 92       G4   0x1
 0x93,  // 93       E5   0x9
        // 94       A4   0x3
 0x91,  // 95       E5   0x9
        // 96       G4   0x1
 0x99,  // 97       E5   0x9
        // 98       E5   0x9
 0x90,  // 99       E5   0x9
        // 100      F4   0x0
 0x72,  // 101      D5   0x7
        // 102      G#4  0x2
 0x99,  // 103      E5   0x9
        // 104      E5   0x9
 0x92,  // 105      E5   0x9
        // 106      G#4  0x2
 0x70,  // 107      D5   0x7
        // 108      F4   0x0
 0x92,  // 109      E5   0x9
        // 110      G#4  0x2
 0x70,  // 111      D5   0x7
        // 112      F4   0x0
 0xBB,  // 113      G5   0xB
        // 114      G5   0xB
 0xBB,  // 115      G5   0xB
        // 116      G5   0xB
 0x11,  // 117      G4   0x1
        // 118      G4   0x1
 0x11,  // 119      G4   0x1
        // 120      G4   0x1
 0x33,  // 121      A4   0x3
        // 122      A4   0x3
 0x33,  // 123      A4   0x3
        // 124      A4   0x3
 0x55,  // 125      B4   0x5
        // 126      B4   0x5
 0x55}; // 127      B4   0x5
        // 128      B4   0x5

// The half-period in F_CPU cycles of each tone.  Used with melody[].  Comment/
//  uncomment as needed.  If lower frequencies are needed, the prescaler will
//  need to be changed (see CS12:0 in the datasheet).
uint16_t EEMEM notePeriods[]={
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
       7053}; // G5   783.99    0xB
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

// Used for inter-task communications about the state of song toggled
//  (NO_CHANGE, SONG_ON, SONG_OFF).
uint8_t songStateChanged = SONG_OFF;

// Used for inter-task communications about a new note being played.
uint8_t newNote = 0;

// LSB is highest priority task.
uint8_t taskBits = 0;

// If non-zero, a tick has elapsed.
volatile uint8_t tickFlag = 0;

// Init the timers to 0 on startup.
unsigned int taskTimers[NUM_TASKS]={0,0,0,0,0,0,0,0};

// The main routine.  Everything starts here.
int main(void)
{

  // Initialize all MCU hardware.
  init_devices();

  // Start the active tasks
  taskBits = 0x0F; // Start tasks 0-3; change value if more tasks are needed.

  // This will repeat until power is lost, or the MCU is reset.
  while(1)
  {

    // If another time slice has elapsed
    if (tickFlag)
    {

      // Reset this flag so this code isn't run again until another slice has
      //  elapsed.
      tickFlag = 0;

      // Run the next appropriate task (if appropriate for this pass).
      task_dispatch();
    }
  }
}

// Call this routine to initialize all peripherals
void init_devices(void)
{

  // Disable errant interrupts until set up.
  //cli(); // Bit initialized to disabled, so this isn't needed.

  // Configure port B as all inputs.
  //DDRB = 0; // Register initialized to zero, so this isn't needed.

  // Configure port D latch, enable, and speaker pins as output.
  DDRD |= (1 << LED_A_WRITE_LATCH) | 
           (1 << LED_B_WRITE_LATCH) | 
           (1 << SW_A_READ_OUTPUTENABLE) | 
           (1 << SW_B_READ_OUTPUTENABLE) |
           SPKR_MASK;

  // Disable internal pull-ups
  //PORTB = 0; // Register initialized to zero, so this isn't needed.

  // The latch OutputEnable is active low, so disable it for now and set output
  //  of speaker pins so they are of opposite states.
  PORTD |= (1 << SW_A_READ_OUTPUTENABLE) |
            (1 << SW_B_READ_OUTPUTENABLE) |
            (1 << SPKR_PIN_1);
  //PORTD &= ~(1 << SPKR_PIN_2); // Register initialized to zero, so this isn't
  // needed

  // Initialize the LEDs to a random state.
  // Want bit 0 (corresponding to switch/light "1" to always be off so that the
  //  default is for the song to not be playing.
  lightStates = (lfsr_prand() << 2) | 0x1;

  sei(); // Enable interrupts.
  // All peripherals are now initialized.
}

// Initialize timer0, which is responsible for the task tick timer.
//  Prescale = CLK/64; Mode = CTC; Desired value = 1.0 millisecond;
//  Actual value = 1.001mSec (+0.1%)
void timer0_init(void)
{

  // Set CS02:0 to 0 to stop the timer while we set it up.
  //TCCR0B &= ~(1<<CS02); // Bit initialized to zero, so this isn't needed.
  //TCCR0B &= ~(1<<CS01); // Bit initialized to zero, so this isn't needed.
  //TCCR0B &= ~(1<<CS00); // Bit initialized to zero, so this isn't needed.

  // Reset the timer count to 0.
  //TCNT0 = 0; // Register is initialize to zero, so this isn't needed.

  // Set COM0A1:0 and COM0B2:0 to 0 disabling the compare output pin modes.
  //TCCR0A &= ~(1<<COM0A1); // Bit initialized to zero, so this isn't needed.
  //TCCR0A &= ~(1<<COM0A0); // Bit initialized to zero, so this isn't needed.
  //TCCR0A &= ~(1<<COM0B1); // Bit initialized to zero, so this isn't needed.
  //TCCR0A &= ~(1<<COM0B0); // Bit initialized to zero, so this isn't needed.

  // Set WGM02:0 to 0b010 enabling CTC mode.
  //TCCR0B &= ~(1<<WGM02); // Bit initialized to zero, so this isn't needed.
  TCCR0A |= (1<<WGM01);
  //TCCR0A &= ~(1<<WGM00); // Bit initialized to zero, so this isn't needed.

  // Set the compare value to run the timer interrupt every millisecond.
  //  Timer0 is an 8-bit timer, and of the possible prescalers (0, 8, 64, 256,
  //  and 1024), the smallest prescaler that will result in a value that will
  //  fit within 8-bits (has to be 255 or less) is 64.  Timer compare value will
  //  be:
  //  11059200 CPU cycle        second        timer cycle   172.8 timer cycle
  //  ------------------ * ---------------- * ----------- = -----------------
  //        second         1000 millisecond    64 cycle        millisecond
  //  Closest value is 173, but timer starts at 0, so subtract 1.
  OCR0A = 172;

  // Set CS02:0 to 0b011 to enable the timer's clock source as the system clock
  //  divided by 64.
  //TCCR0B &= ~(1<<CS02); Was zeroed above; don't need to do it again.
  TCCR0B |= (1<<CS01) |
             (1<<CS00);
 
  // Enable the TIMER0 interrupt.
  TIMSK |= (1<<OCIE0A);
}

// Initialize the music oscillator timer.
//  Prescale = CLK/64; Mode = CTC
void timer1_init(void)
{
 
  // Set CS12:0 to 0 to stop the timer while we set it up.
  //TCCR1B &= ~(1<<CS12); // Initialized to zero, so this isn't needed.
  //TCCR1B &= ~(1<<CS11); // Initialized to zero, so this isn't needed.
  //TCCR1B &= ~(1<<CS10); // Initialized to zero, so this isn't needed.

  // Reset the timer count to 0.
  //TCNT1 = 0; // Initialized to zero, so this isn't needed.

  // Set COM1A1:0 and COM1B2:0 to 0 disabling the compare output pin modes.
  //TCCR1A &= ~(1<<COM1A1); // Initialized to zero, so this isn't needed.
  //TCCR1A &= ~(1<<COM1A0); // Initialized to zero, so this isn't needed.
  //TCCR1A &= ~(1<<COM1B1); // Initialized to zero, so this isn't needed.
  //TCCR1A &= ~(1<<COM1B0); // Initialized to zero, so this isn't needed.

  // Set WGM13:0 to 0b0100 enabling CTC mode.
  //TCCR1B &= ~(1<<WGM13); // Initialized to zero, so this isn't needed.
  TCCR1B |= (1<<WGM12);
  //TCCR1A &= ~(1<<WGM11); // Initialized to zero, so this isn't needed.
  //TCCR1A &= ~(1<<WGM10); // Initialized to zero, so this isn't needed.

  // Disable input capture modes.
  //TCCR1B &= ~(1<<ICNC1); // Initialized to zero, so this isn't needed.
  //TCCR1B &= ~(1<<ICES1); // Initialized to zero, so this isn't needed.

  // Initialize timer compare register value to that of the first note.
  OCR1A = eeprom_read_word(&notePeriods[eeprom_read_byte(&melody[0])>>4]);

  // Set CS12:0 to 0b001 to enable the timer's clock source as the system clock
  //  with no prescaler.
  //TCCR1B &= ~(1<<CS12); Was zeroed above; don't need to do it again.
  //TCCR1B &= ~(1<<CS11); Was zeroed above; don't need to do it again.
  TCCR1B |= (1<<CS10);
}

// Returns a pseudo-random value between 1 and 255 by using a period-maximal
//  8-bit LFSR 
uint8_t lfsr_prand(void)
{
  uint8_t prand; // Psuedo-random value.

  // Retrieve the seed from EEPROM.
  prand = eeprom_read_byte(&storedSeed); 

  // Calculate the new LFSR value.
  prand = (prand << 1) + (1 &
           ((prand >> 1) + (prand >> 2) + (prand >> 3) + (prand >> 7)));

  // Store the new seed in the EEPROM.
  eeprom_write_byte (&storedSeed, prand) ;

  // Return the pseudo-random value.
  return prand;
}

// A task gets dispatched on every tickFlag tick.
void task_dispatch(void)
{

  /* Scan the task bits for an active task and execute it */

  uint8_t task;

  // Take care of the task timers. if the value is 0 skip it else decrement it.
  //  If it decrements to zero, activate the task associated with it.
  task=0;
  while (task < NUM_TASKS )
  {

    // If the timer for this task is not zero.
    if (taskTimers[task])
    {

      // Decrement the timer.
      taskTimers[task]--;

      // If the timer for this task is now zero
      if (taskTimers[task] == 0 )
      {

        // Activate the task bit.
        taskBits |= 1<<task;
      }
    }

    // Examine the next task.
    task++;
  }

  // If this task should be run this pass
  if (taskBits & 0x01)
  {

    // Run this task.
    task_0_read_switches();
  }

  // If this task should be run this pass
  else if (taskBits & 0x02)
  {

    // Run this task.
    task_1_game_action();
  }

  // If this task should be run this pass
  else if (taskBits & 0x04)
  {

    // Run this task.
    task_2_write_LEDs();
  }

  // If this task should be run this pass
  else if (taskBits & 0x08)
  {

    // Run this task.
    task_3_play_song();
  }

  // Uncomment these as additional tasks are needed.
  // If this task should be run this pass
  //else if (taskBits & 0x10)
  //{

    // Run this task.
    //task_4();
  //}

  // If this task should be run this pass
  //else if (taskBits & 0x20)
  //{

    // Run this task.
    //task_5();
  //}

  // If this task should be run this pass
  //else if (taskBits & 0x40)
  //{

    // Run this task.
    //task_6();
  //}

  // If this task should be run this pass
  //else if (taskBits & 0x80)
  //{

    // Run this task.
    //task_7();
  //}
}

// Read current state of pushbuttons and logically debounce them.
void task_0_read_switches(void)
{

  // Declare this as static so it will be remembered from one frame to the next.
  static uint16_t currentSwitches;

  // Local variables needed only by this routine.
  uint16_t lastSwitches, switchDiffs;
  uint8_t i;

  // Copy off the previous frame's switch states.
  lastSwitches = currentSwitches;

  // Set port B pins as input
  DDRB = 0;

  // Disable internal pull-ups
  PORTB = 0;

  // Enable bus output for the first bank of switches.  Keep in mind that the
  //  latch OutputEnable is active low
  PORTD &= ~(1 << SW_A_READ_OUTPUTENABLE);

  // I've found that a delay is necessary in order for the switch values to
  //  appear on the bus.  Datasheets suggest the propogation delay through the
  //  D-latch itself will be less than a quarter of a clock cycle, so there may
  //  be some capacitance in the circuit making this nop necessary.
  asm volatile("nop"); 

  // Grab the values from port B
  currentSwitches = PINB;

  // Disable bus output for the switches
  PORTD |= (1 << SW_A_READ_OUTPUTENABLE);

  // Enable bus output for the second bank of switches
  PORTD &= ~(1 << SW_B_READ_OUTPUTENABLE);

  // Another wait.
  asm volatile("nop"); 

  // Grab the values from port B
  currentSwitches |= (PINB << 8);

  // Disable bus output for the switches
  PORTD |= (1 << SW_B_READ_OUTPUTENABLE);

  // Figure out which switches changed in the last frame.
  switchDiffs = lastSwitches ^ currentSwitches;


  // Cycle through each of the switches to see which ones have changed.
  i = NUM_SWITCHES;
  do
  {

    // Decrement the index.
    i--;

    // If this switch's position hasn't changed this frame
    if (!(switchDiffs & (1<<i)))
    {

      // This switch is the same state as before, so it isn't bouncing so we'll
      //  consider it to be debounced.
      debouncedSwitches &= ~(1<<i);
      debouncedSwitches |= (currentSwitches & (1<<i));
    }

    // Else, it just changed this frame and might be bouncing, so we won't do
    //  anything yet.
    //else {}

  // Loop through again if this isn't the last time.
  } while (i != 0);

  // Reset the task timer.
  taskTimers[0] = 5; // Run every 5.0ms
  taskBits &= ~(0x01);
}

// Determine what the LEDs' states should be.
void task_1_game_action(void)
{

  // Declare these as static so they will be remembered from one frame to the
  //  next.
  static uint16_t lastDebouncedSwitches, switchDebouncedDiffs;

  // Local variable needed only by this routine.
  int8_t i;

  // Figure out which debounced switches changed in the last frame.
  switchDebouncedDiffs = lastDebouncedSwitches ^ debouncedSwitches;

  // Cycle through each of the debounced switches to see which ones have
  //  just been pushed.
  for ( i = 0; i < NUM_SWITCHES; i++ )
  {

    // If the button wasn't pressed before, and it has changed states (meaning
    //  it was just pressed)
    if (lastDebouncedSwitches & (1<<i) & switchDebouncedDiffs)
    {

      // Toggle the light with the corresponding number.
      lightStates ^= (1<<i);

      // If this is the button labeled "1"
      if (i == 0)
      {

        // If the song is turning off
        if (lightStates & 0x1) {

          // Record the song state as turning off.
          songStateChanged = SONG_OFF;
        }
        else
        {

          // Record the song state as turning on.
          songStateChanged = SONG_ON;
        }
      }
    }
  }

  // Determine if a new note started playing.
  if (newNote == 1)
  {

    // Toggle the light labeled "2"
    lightStates ^= 0x02;

    // Reset the flag so this doesn't happen again until a new note is played.
    newNote = 0;
  }

  // Remember the current state of the debounced switches for comparison during
  //  the next frame.
  lastDebouncedSwitches = debouncedSwitches;

  // Reset the task timer.
  taskTimers[1] = 5; // Run every 5.0ms
  taskBits &= ~(0x02);
}

// Update LED display.  Note that the LEDs are connected between U3/U5 and VCC.
//  Thus, they illuminate when the outputs on U3/U5 go *low*, so 0="LED on" and
//  1="LED off".
void task_2_write_LEDs(void)
{

  // Set port B pins as output
  DDRB = 0b11111111;

  // Load port B with the lower eight commanded LED states.
  PORTB = lightStates & 0xFF;

  // Enable the LED latches.  This actually turns the LEDs to their commanded
  //  state.
  PORTD |= (1 << LED_A_WRITE_LATCH);

  // Disable the LED latches so future changes to port B don't change the LEDs'
  //  states.
  PORTD &= ~(1 << LED_A_WRITE_LATCH);

  // Load port B with the upper two commanded LED states and repeat as before.
  PORTB = lightStates >> 8;
  PORTD |= (1 << LED_B_WRITE_LATCH);
  PORTD &= ~(1 << LED_B_WRITE_LATCH);

  // Reset the task timer.
  taskTimers[2] = 5; // Run every 5.0ms
  taskBits &= ~(0x04);
}

// Play a song.
void task_3_play_song(void)
{

  // Declare this as static so it will be remembered from one frame to the next.
  static uint8_t melodyIndex = 0;

  // Local variable needed only by this routine.
  uint8_t notePair;

  // If the state of the song was commanded to be changed
  if (songStateChanged != NO_CHANGE)
  {

    // If the song is turning off
    if (songStateChanged == SONG_OFF)
    {

      // Disable the TIMER1 interrupt.
      TIMSK &= ~(1<<OCIE1A);
    }

    // Else the song is turning on.
    else
    {

      // Enable the TIMER1 interrupt.
      TIMSK |= (1<<OCIE1A);

      // Reset the song to the beginning of the melody.
      melodyIndex = 0;
    }

    // Reset the song state change flag.
    songStateChanged = NO_CHANGE;
  }

  // Get the new pair of notes (only one of which we'll be playing this frame).
  notePair = eeprom_read_byte(&melody[melodyIndex>>1]);

  // If melodyIndex is odd
  if (melodyIndex & 0x01)
  {

    // Select the least significant nibble.
    notePair &= 0x0F;
  }

  // Else melodyIndex is even.
  else
  {

    // Select the least significant nibble.
    notePair >>= 4;
  }

  // Set the period of the timer using the nibble corresponding to the note in
  //  the notePeriods array.
  OCR1A = eeprom_read_word(&notePeriods[notePair]);

  // Increment the melody index for the next frame.
  // If this is the last note in the melody.
  if (melodyIndex == (NUM_NOTES - 1))
  {

    // Reset the index to the beginning of the melody.
    melodyIndex = 0;
  }

  // This isn't the last note in the melody.
  else
  {

    // Increment the melody index.
    melodyIndex++;
  }

  // Let the game logic task know another note was played.
  newNote = 1;

  // Reset the task timer.
  // Run at a speed that's based on the current states of lights labeled 3
  //  through 10--anywhere from 8 to 1024 milliseconds.
  taskTimers[3] = (lightStates & 0x03FC) + 4;
  taskBits &= ~(0x08);
}

// Uncomment these as additional tasks are needed.
//void task_4(void)
//{

  // Reset the task timer.
  //taskTimers[4] = X; // Run every X.0 milliseconds.
  //taskBits &= ~(0x10);
//}

//void task_5(void)
//{

  // Reset the task timer.
  //taskTimers[5] = X; // Run every X.0 milliseconds.
  //taskBits &= ~(0x20);
//}

//void task_6(void)
//{

  // Reset the task timer.
  //taskTimers[6] = X; // Run every X.0 milliseconds.
  //taskBits &= ~(0x40);
//}

//void task_7(void)
//{

  // Reset the task timer.
  //taskTimers[7] = X; // Run every X.0 milliseconds.
  //taskBits &= ~(0x80);
//}

// This interrupt routine will automagically be called everytime TIMER0 has
//  overflowed.  See timer0 stuff in init_devices to see how the timer was
//  setup.
ISR(TIMER0_COMPA_vect)
{

  // TIMER0 has overflowed
  tickFlag = 1;
} 

// This interrupt routine will automagically be called everytime TIMER1 has
//  overflowed.  See timer1 stuff in init_devices() to see how the timer was
//  setup.
ISR(TIMER1_COMPA_vect)
{

  // Toggle the speaker pins to make a click.
  PORTD ^= SPKR_MASK;
} 
