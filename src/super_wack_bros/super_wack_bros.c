// Super Wack Bros.
//  https://github.com/oelgern/keypadContest
//
// This code demonstrates a super-wack whack-a-mole game for two players
//  head-to-head action on a Hall Research Technologies KP-2B keypad fitted with
//  an ATtiny2313 or ATtiny4313 with factory fuse settings.  See
//  hallResearchKP2B-reverseEngineering.txt for more information about the
//  hardware.
//
// The game controls and display consist of the pushbuttons, LEDs, and an
//  optional speaker.  The buttons and lights are used as follows:
//   The left player's moles are at action switches/lights on the left two
//    columns and are labeled "1", "2", "6", and "7".
//   The right player's moles are at action switches/lights on the right two
//    columns and are labeled "4", "5", "9", and "10".
//   The left player's status light/control switch is in the middle column on
//    the top and is labeled "3".
//   The right player's status light/control switch is in the middle column on
//    the bottom and is labeled "8".
//
// At power-on attract mode, the players' status lights flash alternately.
//  Pressing player 1's control switch begins the game with the sound enabled.
//  Pressing player 2's control switch begins the game with the sound disabled.
//  At any point during the game play, pressing either players' control switch
//  will toggle the sound.
//
// During game play both players are presented with the same psuedo-random
//  sequence of moles.  A mole consists of a random delay followed by the
//  presentation of a mole.  The presentation of the mole is indicated by a
//  light appearing next to one of each of the players' action switches, and a
//  high-pitched mole chirp.  When the mole is presented, the player will
//  respond (before the mole disappears) by pushing the button adjacent to the
//  light.  This will advance that player to the next mole and immediately start
//  the delay counter for that next mole.  A correct and timely squashing of the
//  mole will be indicated by a medium-low pitch buzz and the extinguishing of
//  the mole light.  An incorrect or late hitting of an empty hole will be
//  indicated by a low pitch dissonant penalty sound.
//
// The game stage is determined by how far, in moles, the players are from each
//  other.  As the game progresses to later stages, the gameplay and music
//  speeds up.  To make the game fair, both players are presented with a
//  sequence of moles that are in the same positions and have the same initial
//  delays.  The player with a faster reaction time will advance ahead of the
//  player that doesn't react as quickly.  Additionally, time penalties are
//  added to the initial delays of moles for a player that smashs an empty hole
//  or who fails to smash a mole before it disappears.  These time penalties
//  cause the appearance of that player's next mole to be delayed, which also
//  will cause that player to fall further behind the other player.
//
// The status light of the player that is ahead blinks, and the speed at which
//  it blinks indicates which stage of the game is currently being played.  When
//  one player reaches a point where they are five moles ahead of the other
//  player, the game ends and the winning player is indicated by that player's
//  lights illuminating momentarily.  After this the game will reset.
//
// The sound is played with an optional piezo speaker connected directly to pins
//  3 & 4 of the DB-9 connector.  According to the RS-232 transceiver's
//  datasheet, it can be shorted continuously, so this shouldn't be a problem.
//
// Low-level code for HRT KP2B keypad to read the switches and write the LEDs
//  adapted from code Copyright 2012 Andrew Sampson.  Released under the GPL v2.
//
// Original co-operative multitasking framework tutorial code
// (c)Russell Bull 2010. Free for any use.
// Code originally built for a Mega168 @ 16MHz
// See the following for more information:
// http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=95490
//
// Bull's co-operative multitasking framework code ported by Nathan Oelger to
//  ATtiny2313.
//
// Game authored by and released to public domain (whatever that means) by
//  Nathan Oelger on 12-January-2013.
//
// Song compositions also authored by Nathan Oelger, but inspired by Dig-Dug's
//  original music.
//
// Built with -Os
//  2042 out of 2048 (for ATtiny2313) bytes program space used
//  104 out of 128 (for ATtiny2313) bytes EEPROM used
//
// Possible improvements:
//  Perhaps reduce the number of game stages.
//  If more program space is needed for additions/tweaks, a bit of this could
//   probably be accomplished through reorganizing the multi-tasking framework.
//  Fix speaker (hardware tweak).
//  Clean up spaghetti code.

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define F_CPU 8000000UL
#define LED_A_WRITE_LATCH PD2
#define LED_B_WRITE_LATCH PD3
#define SW_A_READ_OUTPUTENABLE PD4
#define SW_B_READ_OUTPUTENABLE PD5
#define SPKR_PIN_1 PD1
#define SPKR_PIN_2 PD6
#define SPKR_MASK 0b01000010
#define NUM_TASKS 8
#define NUM_PLAYER_SWITCHES 5
#define NUM_PLAYER_ACTION_SWITCHES 4
#define NUM_NOTES 128
#define NUM_NUTS NUMB_NUTS
#define FINAL_NOTE 151
#define PLAYER_1 0
#define PLAYER_2 1
#define SOUND_OFF 0
#define SOUND_ON 1
#define SOUND_START 2
#define PLAYER_LIGHTS_ON      0b00000000
#define PLAYER_LIGHTS_OFF     0b00011111
#define PLAYER_STATUS_MASK    0b00010000
#define PLAYER_MOLES_MASK     0b00001111
#define PLAYER_COMMAND_BUTTON 0b00010000

static inline void init_devices(void);
static inline void timer0_init(void);
static inline void timer1_init(void);
static inline void lfsr_prand(void);
static inline void task_0_read_switches(void);
static inline void task_1_game_action(void);
static inline void task_2_write_LEDs(void);
static inline void task_3_play_song(void);
//static inline void task_4(void); // Uncomment this if more tasks are required.
//static inline void task_5(void); // Uncomment this if more tasks are required.
//static inline void task_6(void); // Uncomment this if more tasks are required.
//static inline void task_7(void); // Uncomment this if more tasks are required.
static inline void task_dispatch(void);
static inline void process_sound_effects(void);
static inline void process_game_buttons(void);
static inline void process_mole_mode(void);
static inline void process_game_stage(void);
static inline void process_status_lights(void);

// An array of bytes (2) for each player.  Each of these contains the debounced
//  switches for each of the two players.
//  Byte          Bits and switches (starting at least significant bit)
//  PLAYER_1 (0)  "1", "2", "6", "7", "3" (3 MSB aren't used)
//  PLAYER_2 (1)  "4", "5", "9", "10", "8" (3 MSB aren't used)
//  Notes:
//   The first four (least significant) bits are for the action switches, and
//    the last is for the command switch.
//   A value of "0" represents a logically debounced button that is currently
//    being depressed.
uint8_t debouncedPlayerSwitches[2];

// An array of bits that correspond to the LEDs' states.  The LSB corresponds
//  to the light labeled "1".  A value of "0" represents a light that is
//  currently on.
// An array of bytes (2) for each player.  Each of these contains the states of
//  the lights for each of the two players.
//  Byte          Bits and lights (starting at least significant bit)
//  PLAYER_1 (0)  "1", "2", "6", "7", "3" (3 MSB aren't used)
//  PLAYER_2 (1)  "4", "5", "9", "10", "8" (3 MSB aren't used)
//  Notes:
//   The first four (least significant) bits are for the mole lights, and the
//   last is for the status light.  A value of "0" represents a light that is
//   on.
uint8_t lightPlayerStates[2];

// The current game stage.
//  0: Not playing right now.
//  1: Players are less than 2 moles between each other.
//  2: A player is 2 moles further than the other player.
//  3: A player is 3 moles further than the other player.
//  4: A player is 4 moles further than the other player.
//  5: Game is ending (a player is 5 moles further than the other player.
uint8_t gameStage = 0;

// The music consists of two melodies.  The first is 128 notes, played in a loop
//  during game play.  The second is the end of game melody played a single time
//  at the end of the game.  Each note takes 4 bits, or half a byte.  These
//  notes are indexes into notePeriods[].
uint8_t EEMEM music[] = {
        // Sequence Tone Index
 // Main game play melody
 0xB6,  // 0        F#5  0xB
        // 1        C5   0x6
 0xC1,  // 2        G5   0xC
        // 3        G4   0x1
 0xC6,  // 4        G5   0xC
        // 5        C5   0x6
 0xC1,  // 6        G5   0xC
        // 7        G4   0x1
 0xC5,  // 8        G5   0xC
        // 9       B4   0x5
 0xC5,  // 10       G5   0xC
        // 11       B4   0x5
 0xC5,  // 12       G5   0xC
        // 13       B4   0x5
 0xC1,  // 14       G5   0xC
        // 15       G4   0x1
 0xC4,  // 16       G5   0xC
        // 17       Bb4  0x4
 0xC1,  // 18       G5   0xC
        // 19       G4   0x1
 0xC4,  // 20       G5   0xC
        // 21       Bb4  0x4
 0xC1,  // 22       G5   0xC
        // 23       G4   0x1
 0xC3,  // 24       G5   0xC
        // 25       A4   0x3
 0xC1,  // 26       G5   0xC
        // 27       G4   0x1
 0xC3,  // 28       G5   0xC
        // 29       A4   0x3
 0xC1,  // 30       G5   0xC
        // 31       G4   0x1
 0x99,  // 32       E5   0x9
        // 33       E5   0x9
 0x90,  // 34       E5   0x9
        // 35       F4   0x0
 0x72,  // 36       D5   0x7
        // 37       G#4  0x2
 0x99,  // 38       E5   0x9
        // 39       E5   0x9
 0x92,  // 40       E5   0x9
        // 41       G#4  0x2
 0x70,  // 42       D5   0x7
        // 43       F4   0x0
 0x92,  // 44       E5   0x9
        // 45       G#4  0x2
 0x70,  // 46       D5   0x7
        // 47       F4   0x0
 0x99,  // 48       E5   0x9
        // 49       E5   0x9
 0x91,  // 50       E5   0x9
        // 51       G4   0x1
 0x99,  // 52       E5   0x9
        // 53       E5   0x9
 0x90,  // 54       E5   0x9
        // 55       F4   0x0
 0x77,  // 56       D5   0x7
        // 57       D5   0x7
 0x73,  // 58       D5   0x7
        // 59       A4   0x3
 0x77,  // 60       D5   0x7
        // 61       D5   0x7
 0x75,  // 62       D5   0x7
        // 63       B4   0x5
 0x86,  // 64       D#5  0x8
        // 65       C5   0x6
 0x91,  // 66       E5   0x9
        // 67       G4   0x1
 0x96,  // 68       E5   0x9
        // 69       C5   0x6
 0x91,  // 70       E5   0x9
        // 71       G4   0x1
 0x95,  // 72       E5   0x9
        // 73       B4   0x5
 0x91,  // 74       E5   0x9
        // 75       G4   0x1
 0x95,  // 76       E5   0x9
        // 77       B4   0x5
 0x91,  // 78       E5   0x9
        // 79       G4   0x1
 0x94,  // 80       E5   0x9
        // 81       Bb4  0x4
 0x91,  // 82       E5   0x9
        // 83       G4   0x1
 0x94,  // 84       E5   0x9
        // 85       Bb4  0x4
 0x91,  // 86       E5   0x9
        // 87       G4   0x1
 0x93,  // 88       E5   0x9
        // 89       A4   0x3
 0x91,  // 90       E5   0x9
        // 91       G4   0x1
 0x93,  // 92       E5   0x9
        // 93       A4   0x3
 0x91,  // 94       E5   0x9
        // 95       G4   0x1
 0x99,  // 96       E5   0x9
        // 97       E5   0x9
 0x90,  // 98       E5   0x9
        // 99      F4   0x0
 0x72,  // 100      D5   0x7
        // 101      G#4  0x2
 0x99,  // 102      E5   0x9
        // 103      E5   0x9
 0x92,  // 104      E5   0x9
        // 105      G#4  0x2
 0x70,  // 106      D5   0x7
        // 107      F4   0x0
 0x92,  // 108      E5   0x9
        // 109      G#4  0x2
 0x70,  // 110      D5   0x7
        // 111      F4   0x0
 0xCC,  // 112      G5   0xC
        // 113      G5   0xC
 0xCC,  // 114      G5   0xC
        // 115      G5   0xC
 0x11,  // 116      G4   0x1
        // 117      G4   0x1
 0x11,  // 118      G4   0x1
        // 119      G4   0x1
 0x33,  // 120      A4   0x3
        // 121      A4   0x3
 0x33,  // 122      A4   0x3
        // 123      A4   0x3
 0x55,  // 124      B4   0x5
        // 125      B4   0x5
 0x55,  // 126      B4   0x5
        // 127      B4   0x5
        // Sequence Tone Index
 // End of game melody
 0x99,  // 128      E5   0x9
        // 129      E5   0x9
 0xAC,  // 130      F5   0xA
        // 131      G5   0xC
 0xC9,  // 132      G5   0xC
        // 133      E5   0x9
 0xAA,  // 134      F5   0xA
        // 135      F5   0xA
 0x79,  // 136      D5   0x7
        // 137      E5   0x9
 0x96,  // 138      E5   0x9
        // 139      C5   0x6
 0x77,  // 140      D5   0x7
        // 141      D5   0x7
 0x56,  // 142      B4   0x5
        // 143      C5   0x6
 0x6D,  // 144      C5   0x6
        // 145      C6   0xD
 0xD6,  // 146      C6   0xD
        // 147      C5   0x6
 0x66,  // 148      C5   0x6
        // 149      C5   0x6
 0x66}; // 150      C5   0x6
        // 151      C5   0x6

// The half-period in F_CPU cycles of each tone.  Used with music[].  Comment/
//  uncomment as needed.  If lower frequencies are needed, the prescaler will
//  need to be changed (see CS12:0 in the datasheet).
uint16_t EEMEM notePeriods[] = {
 //    Cycles-1  Note Frequency Index
    // 45814, // F2   87.31
    // 43243, // F#2  92.50
    // 40816, // G2   98.00
    // 38525, // G#2  103.83
    // 36364, // A2   110.00
    // 34323, // Bb2  116.54
    // 32397, // B2   123.47
    // 30579, // C3   130.81
    // 28862, // C#3  138.59
    // 27242, // D3   146.83
    // 25714, // Eb3  155.56
    // 24270, // E3   164.81
    // 22908, // F3   174.61
    // 21622, // F#3  185.00
    // 20408, // G3   196.00
    // 19263, // G#3  207.65
    // 18182, // A3   220.00
    // 17161, // Bb3  233.08
    // 16198, // B3   246.94
    // 15289, // C4   261.63
    // 14431, // C#4  277.18
    // 13621, // D4   293.67
    // 12856, // Eb4  311.13
    // 12135, // E4   329.63
       11454, // F4   349.23    0x0
    // 10811, // F#4  369.99
       10204, // G4   392.00    0x1
       9631,  // G#4  415.31    0x2
       9091,  // A4   440.00    0x3
       8581,  // Bb4  466.16    0x4
       8099,  // B4   493.88    0x5
       7645,  // C5   523.25    0x6
    // 7215,  // C#5  554.37
       6810,  // D5   587.33    0x7
       6428,  // Eb5  622.25    0x8
       6067,  // E5   659.26    0x9
       5727,  // F5   698.46    0xA
       5405,  // F#5  739.99    0xB
       5102,  // G5   783.99    0xC
    // 4816,  // G#5  830.61
    // 4545,  // A5   880.00
    // 4290,  // Bb5  932.33
    // 4050,  // B5   987.77
       3822}; // C6   1046.50   0xD
    // 3608,  // C#6  1108.73
    // 3405,  // D6   1174.66
    // 3214,  // Eb6  1244.51
    // 3034,  // E6   1318.51
    // 2863,  // F6   1396.91
    // 2703,  // F#6  1479.98
    // 2551,  // G6   1567.98
    // 2408,  // G#6  1661.22
    // 2273,  // A6   1760.00
    // 2145,  // Bb6  1864.66
    // 2025,  // B6   1975.53
    // 1911,  // C7   2093.00
    // 1804,  // C#7  2217.46
    // 1703,  // D7   2349.32
    // 1607,  // Eb7  2489.02
    // 1517,  // E7   2637.02
    // 1432,  // F7   2793.83
    // 1351,  // F#7  2959.96
    // 1276,  // G7   3135.96
    // 1204,  // G#7  3322.44
    // 1136,  // A7   3520.00
    // 1073,  // Bb7  3729.31
    // 1012,  // B7   3951.07
    // 956,   // C8   4186.01

// Used for inter-task communications about the state of sound.
uint8_t soundEnabled;

// Used for inter-task communications indicating a new note should be played.
uint8_t newNote;

// The number of oscillations yet to be played for the advance tone.
uint8_t advanceToneFlips;

// The number of oscillations yet to be played for the mole tone.
uint8_t moleToneFlips;

// The number of oscillations yet to be played for the primary penalty tone.
uint8_t penaltyTone1Flips;

// The players' current mole (index into the moleDelay[] and moleLocation[]).
uint8_t playerIndex[2];

// The players' current timer.  These are decremented each pass and when it
//  reaches 0, something will happen.  This will be set at the beginning of a
//  mole to the delay before the mole is presented.  After it is zeroed, it
//  will be set to the penalty time (if any), after which it will be set to
//  the mole presentation time.
uint8_t playerTimer[2];

// Players' penalties accumulated during the current mole.  These add extra
//  delay time to the following mole for the player that has acquired the
//  penalties.  Penalties are acquired for:
//   Pushing a button adjacent to a space where no mole was presented; and
//   Failing to push the button adjacent to a presented mole during the time
//    the mole is presented.
uint8_t playerPenalty[2];

// Mode of the moles (is used primarily for the timers)
//  Mode Meaning
//  0    Initial wait (not presented); 1.5 seconds in gameStage 1
//  1    Psuedo-random wait and penalty wait
//  2    Mole has head up (presented)
uint8_t moleMode[2];

// Status of if players' mole indexed this frame.  0 == not advanced
uint8_t moleAdvance[2];

// LSB is highest priority task.
uint8_t taskBits = 0;

// If ten, a tick has elapsed.
volatile uint8_t tickFlag = 0;

// Init the timers to 0 on startup.
uint8_t taskTimers[NUM_TASKS]={0,0,0,0,0,0,0,0};

// Psuedo-random value.
uint8_t prand;

// Player number. Note player #1 has a value of '0' and player #2 has a value
//  of '1'.  This is declared globally as it is used in more than one routine
//  (but it's value being changed in one routine won't cause an issue), and
//  (going against conventional wisdom) declaring it globally reduced the
//  program space by 20 bytes (but it increases RAM usage by 1 byte).
uint8_t player;

// Each mole consists of a location and a time delay.  The length of these
//  arrays is important.  The length has to be this long (or longer) so that
//  one player can be up to 5 moles ahead of the other player, and so the
//  logic can determine who is ahead of who based solely on the players'
//  current indexes into the array (and the fact that the difference can't be
//  more than 5).
uint8_t moleLocation[11];
uint8_t moleDelay[11];

// Status of which player (if any) is ahead of the other.  0 == not ahead
uint8_t playerAhead[2];

// The main routine.  Everything starts here.
int main(void)
{

  // Declare this as static so it will be remembered from one frame to the next.
  static uint8_t lastTickFlag;

  // Initialize all MCU hardware.
  init_devices();

  // Start the active tasks
  taskBits = 0x0F; // Start tasks 0-3; change value if more tasks are needed.

  // This will repeat until power is lost, or the MCU via the watchdog timer.
  while(1)
  {

    // If another 200 microsecond period has passed.
    if (lastTickFlag != tickFlag)
    {

      // If another millisecond time slice has elapsed
      if (tickFlag >= 5)
      {

        // Reset this flag so this code isn't run again until another slice has
        //  elapsed.
        tickFlag -= 5;

        // Run the next appropriate task (if appropriate for this pass).
        task_dispatch();
      }

      lastTickFlag = tickFlag;

      // Process the sound effects (all sound except the music).
      //  This was actually a last-minute hack, and doesn't fit cleanly into the
      //  multi-tasking framework under task_dispatch(), but it seems to work
      //  and I'm out of time.
      process_sound_effects();
    }
  }
}

// Process the sound effects (all sound except the music).
void process_sound_effects(void)
{

  // Counter of 200 usec periods since the last time the advance tone was
  //  oscillated.
  static uint8_t advanceToneClock;

  // Counter of 200 usec periods since the last time the primary penalty tone
  //  was oscillated.
  static uint8_t penaltyTone1Clock;

  // Counter of 200 usec periods since the last time the secondary penalty tone
  //  was oscillated.
  static uint8_t penaltyTone2Clock;

  // Keeps track of if we flip the pins to the speakers this frame.
  uint8_t flipThisFrame;

  // If we're not done playing the advance tone
  if (advanceToneFlips)
  {

    // Decrement the clock.
    advanceToneClock --;

    // If the tone clock says its time to make the speaker click again
    //  This will make the tone 200 Hz.
    if (advanceToneClock == 0)
    {

      // Reset the clock.
      advanceToneClock = 5;

      // Decrement the counter for the number of clicks for this tone.
      advanceToneFlips --;

      // Keep track of if we flip the pins to the speakers this frame.
      flipThisFrame ^= 0x01;
    }
  }

  // If we're not done playing the mole tone
  if (moleToneFlips)
  {

    // Decrement the counter for the number of clicks for this tone.
    moleToneFlips --;

    // Keep track of if we flip the pins to the speakers this frame.
    flipThisFrame ^= 0x01;
  }

  // If we're not done playing the mole tone
  if (penaltyTone1Flips)
  {

    // Decrement the clock.
    penaltyTone1Clock --;

    // If the tone clock says its time to make the speaker click again
    //  This will make the tone approximately 125 Hz.
    if (penaltyTone1Clock == 0)
    {

      // Reset the clock.
      penaltyTone1Clock = 20;

      // Decrement the counter for the number of clicks for this tone.
      penaltyTone1Flips --;

      // Keep track of if we flip the pins to the speakers this frame.
      flipThisFrame ^= 0x01;

    }

    // Decrement the clock.
    penaltyTone2Clock --;

    // If the tone clock says its time to make the speaker click again
    //  The penalty tone is supposed to be a dissonant sound, and this is
    //   done by playing two frequencies that are close to each other at the
    //   same time.  This will make the tone approximately 114 Hz.
    if (penaltyTone2Clock == 0)
    {

      // Reset the clock.
      penaltyTone2Clock = 22;

      // Keep track of if we flip the pins to the speakers this frame.
      flipThisFrame ^= 0x01;
    }
  }

  // If sound effects are enabled and an odd number of slips are supposed to happen this frame
  if ((soundEnabled == SOUND_ON) && flipThisFrame)
  {

    // Make the speaker click.
    PORTD ^= SPKR_MASK;
  }
}

// Call this routine to initialize all peripherals
void init_devices(void)
{

  // Disable errant interrupts until set up.
  //cli(); // Bit initialized to disabled, so this isn't needed.

  // Disable the watchdog timer.  It is only used at the conclusion of the game
  //  to reset the MCU for another game.
  // Reset the watchdog timer.
#ifdef __AVR
  asm volatile("wdr");
#endif

  // Clear WDRF in MCUSR.
  MCUSR &= ~(1<<WDRF);

  // Write logical one to WDCE and WDE.
  WDTCSR |= (1<<WDCE) | (1<<WDE);

  // Turn off WDT.
  WDTCSR = 0x00;

  // Clear the divide-by-eight clock bit so MCU runs at full speed.
  clock_prescale_set(clock_div_1);

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

  // The latch OutputEnable is active low, so disable it for now and set output
  //  of speaker pins so they are of opposite states.
  PORTD = (1 << SW_A_READ_OUTPUTENABLE) |
           (1 << SW_B_READ_OUTPUTENABLE) |
           (1 << SPKR_PIN_1);
  //PORTD &= ~(1 << SPKR_PIN_2); // Register initialized to zero, so this isn't
  // needed

  // Initialize timer 0--the 8-bit timer used to player sound effects and
  //  measure the frames.
  timer0_init();

  // Initialize timer 1--the 16-bit timer used to create the tones for the
  //  music.
  timer1_init();

  sei(); // Enable interrupts.
  // All peripherals are now initialized.
}

// Initialize timer0, which is responsible for the task tick timer and sound
//  effects.
//  Prescale = CLK/64; Mode = CTC; Desired value = 200 microseconds;
//  Actual value = 200 microseconds
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
  TCCR0A = (1<<WGM01);
  //TCCR0A &= ~(1<<WGM00); // Bit initialized to zero, so this isn't needed.

  // Set the compare value to run the timer interrupt every 200 microseconds.
  //  Timer0 is an 8-bit timer, and of the possible prescalers (0, 8, 64, 256,
  //  and 1024), the smallest prescaler that will result in a value that will
  //  fit within 8-bits (has to be 255 or less) is 64.  Timer compare value will
  //  be:
  //  8000000 CPU cycle        second        timer cycle   1.00 timer cycle
  //  ----------------- * ---------------- * ----------- = ----------------
  //       second         10^6 microsecond   8 CPU cycle     microsecond
  //  1.00 timer cycle
  //  ---------------- * 200 microsecond = 200 timer cycles
  //    microsecond
  //  Closest value is 200, but timer starts at 0, so subtract 1.
  OCR0A = 199;

  // Set CS02:0 to 0b010 to enable the timer's clock source as the system clock
  //  divided by 8.
  //TCCR0B &= ~(1<<CS02); // Bit initialized to zero, so this isn't needed.
  TCCR0B = (1<<CS01);
  //TCCR0B &= ~(1<<CS00); // Was zeroed above; don't need to do it again.

  // Enable the TIMER0 interrupt.
  TIMSK = (1<<OCIE0A);
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

  // Set WGM13:0 to 0b0000 enabling Normal mode.  We'll eventually enable CTC
  //  mode to play the music tones, but for now we'll be waiting for the first
  //  user input and use the psuedo-random time it takes the user to push the
  //  first button as the seed for the LFSR.
  //TCCR1B &= ~(1<<WGM13); // Initialized to zero, so this isn't needed.
  //TCCR1B &= ~(1<<WGM12); // Initialized to zero, so this isn't needed.
  //TCCR1A &= ~(1<<WGM11); // Initialized to zero, so this isn't needed.
  //TCCR1A &= ~(1<<WGM10); // Initialized to zero, so this isn't needed.

  // Set WGM13:0 to 0b0100 enabling CTC mode for Timer1 (since we won't
  //  need it to be in Normal mode now that we have our first
  //  psuedo-random value.
  //TCCR1B &= ~(1<<WGM13); // Initialized to zero, so this isn't needed.
  TCCR1B = (1<<WGM12) | /*;*/
  //TCCR1A &= ~(1<<WGM11); // Initialized to zero, so this isn't needed.
  //TCCR1A &= ~(1<<WGM10); // Initialized to zero, so this isn't needed.


  // Disable input capture modes.
  //TCCR1B &= ~(1<<ICNC1); // Initialized to zero, so this isn't needed.
  //TCCR1B &= ~(1<<ICES1); // Initialized to zero, so this isn't needed.

  // Set CS12:0 to 0b001 to start the timer incrementing and to enable the
  //  timer's clock source as the system clock with no prescaler (that is, a
  //  scale divider value of "1").
  //TCCR1B &= ~(1<<CS12); Was zeroed above; don't need to do it again.
  //TCCR1B &= ~(1<<CS11); Was zeroed above; don't need to do it again.
  /*TCCR1B |=*/ (1<<CS10);

  // Set the period of the timer so it will be cycling for the psuedo-random
  //  number generator seed.  Set to one less than the maximum 8-bit value.  One
  //  will be added to it when read (psuedo-random seed can't be zero).
  //OCR1AH = 0; // Initialized to zero, so this isn't needed.
  OCR1AL = 0xFE;
}

// Calculates a pseudo-random value between 1 and 255 by using a period-maximal
//  8-bit LFSR
void lfsr_prand(void)
{

  // Calculate the new LFSR value.
  prand = (prand << 1) + (1 &
           ((prand >> 1) + (prand >> 2) + (prand >> 3) + (prand >> 7)));
}

// A task gets dispatched on every millisecond.
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
  if (taskBits & (1 << 0))
  {

    // Run this task.
    task_0_read_switches();
  }

  // If this task should be run this pass
  if (taskBits & (1 << 1))
  {

    // Run this task.
    task_1_game_action();
  }

  // If this task should be run this pass
  if (taskBits & (1 << 2))
  {

    // Run this task.
    task_2_write_LEDs();
  }

  // If this task should be run this pass
  if (taskBits & (1 << 3))
  {

    // Run this task.
    task_3_play_song();
  }

  // Uncomment these as additional tasks are needed.
  // If this task should be run this pass
  //if (taskBits & (1 << 4))
  //{

    // Run this task.
    //task_4();
  //}

  // If this task should be run this pass
  //if (taskBits & (1 << 5))
  //{

    // Run this task.
    //task_5();
  //}

  // If this task should be run this pass
  //if (taskBits & (1 << 6))
  //{

    // Run this task.
    //task_6();
  //}

  // If this task should be run this pass
  //if (taskBits & (1 << 7))
  //{

    // Run this task.
    //task_7();
  //}
}

// Read current state of pushbuttons and logically debounce them.
void task_0_read_switches(void)
{

  // Declare this as static so it will be remembered from one frame to the next.
  static uint8_t currentPlayerSwitches[2];

  // Local variables needed only by this routine.
  uint8_t lastPlayerSwitches[2];
  uint8_t switchPlayerDiffs[2];
  uint8_t temp;

  // Copy off the previous frame's switch states.
  lastPlayerSwitches[PLAYER_1] = currentPlayerSwitches[PLAYER_1];
  lastPlayerSwitches[PLAYER_2] = currentPlayerSwitches[PLAYER_2];

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

  // Copy the PINB to a temporary variable.  Since PINB is treated as volatile,
  //  this ends up saving 4 bytes of program space (compiler would copy PINB
  //  every time it is called out in the following code).
  temp = PINB;

  // Grab the values from port B.  See definition of currentPlayerSwitches[] to
  //  make sense of the following.
  //                                Bits 0 and 1 of
  //                                 currentPlayerSwitches[PLAYER_1] correspond
  //                                 to bits 0 and 1 of PINB.
  currentPlayerSwitches[PLAYER_1] = (temp & 0x03) |
  //                                Bits 2 and 3 of
  //                                 currentPlayerSwitches[PLAYER_1] correspond
  //                                 to bits 5 and 6 of PINB.
                                    ((temp & 0x60) >> 3) |
  //                                Bit 4 of currentPlayerSwitches[PLAYER_1]
  //                                 corresponds to bit 2 of PINB.
                                    ((temp & 0x04) << 2);

  // Disable bus output for the switches
  PORTD |= (1 << SW_A_READ_OUTPUTENABLE);

  // Enable bus output for the second bank of switches
  PORTD &= ~(1 << SW_B_READ_OUTPUTENABLE);

  // Another wait.
  asm volatile("nop");

  // Grab the values from port B.  See definition of currentPlayerSwitches[] to
  //  make sense of the following.
  //                                 Bits 2 and 3 of
  //                                  currentPlayerSwitches[PLAYER_2] correspond
  //                                  to bits 0 and 1 of PINB.
  currentPlayerSwitches[PLAYER_2] = ((PINB & 0x03) << 2) |
  //                                Bits 0, 1, and 4 of
  //                                 currentPlayerSwitches[PLAYER_2] correspond
  //                                 to bits 3, 4, and 7 of PINB.
                                    ((temp & 0x98) >> 3);

  // Disable bus output for the switches
  PORTD |= (1 << SW_B_READ_OUTPUTENABLE);

  // Cycle through each of both players' switches to see which ones have
  //  changed.
  // Changed what was a for loop to a do loop to save a few more bytes to
  //  squeeze the software inside the 2K program space.
  player = PLAYER_2;
  do
  {

    // Figure out which switches changed in the last frame.
    switchPlayerDiffs[player] = lastPlayerSwitches[player] ^
                                   currentPlayerSwitches[player];

    // Cycle through each of the switches for this player to see which ones have
    //  changed
    for ( temp = 1; temp < (1 << NUM_PLAYER_SWITCHES); temp <<= 1 )
    {

      // If this switch's position hasn't changed this frame
      if (!(switchPlayerDiffs[player] & temp))
      {

        // This switch is the same state as before, so it isn't bouncing so
        //  we'll consider it to be debounced.

        // Zero out the bit.
        debouncedPlayerSwitches[player] &= ~temp;

        // Set the bit (if this bit of currentPlayerSwitches is set)
        debouncedPlayerSwitches[player] |= (currentPlayerSwitches[player] &
                                            temp);
      }

      // Else, it just changed this frame and might be bouncing, so we won't do
      //  anything yet.
      //else {}
    }

    // Decrement the index
    player--;

    // Part of the program space-saving hack.
  } while (player != 255);

  // Reset the task timer.
  taskTimers[0] = 4; // Run every 4.0ms
  taskBits &= ~(0x01);
}

// The spaghetti-mess logic of the game.  Based on timers and switch inputs, and
//  previous states of lights, figure out what the new times and state of the
//  lights should be, and what the music/sound should be doing.
void task_1_game_action(void)
{

  // Handle the processing of logically debounced game button depressions.
  process_game_buttons();

  // Figure out which player is ahead and which game stage is currently being
  //  played.
  process_game_stage();

  // Figure out what the status lights should be doing.
  process_status_lights();

  // Reset the task timer.  Speed of game depends on the game stage.  This
  //  routine is run every:
  //   Game stage Milliseconds
  //   0          14 (probably doesn't matter)
  //   1          12
  //   2          10
  //   3          8
  //   4          6
  taskTimers[1] = (7 - gameStage) << 1;
  taskBits &= ~(0x02);
}

// Handle the processing of logically debounced game button depressions.
void process_game_buttons(void)
{

  // Declare this as static so they will be remembered from one frame to the
  //  next.  Keeps track of the last frames state of the debounced buttons.
  static uint8_t lastDebouncedSwitches[2];

  // Declare this as static so they will be remembered from one frame to the
  //  next.  Keeps track of which switches changed between this frame and the
  //  previous frame.
  static uint8_t switchDebouncedDiffs[2];

  // Local variable needed only by this routine that is used for one or more
  //  temporary tasks.
  uint8_t temp;

  // Cycle through each of both players' debounced switches to see which ones
  //  have just been pushed.
  // Changed what was a for loop to a do loop to save a few more bytes to
  //  squeeze the software inside the 2K program space.
  player = PLAYER_2;
  do
  {

    // Figure out which debounced switches changed in the last frame.
    switchDebouncedDiffs[player] = lastDebouncedSwitches[player] ^
                                   debouncedPlayerSwitches[player];

    // Cycle through each of the debounced switches to see which ones have
    //  just been pushed.
    for ( temp = 1; temp < (1 << NUM_PLAYER_SWITCHES); temp <<= 1 )
    {

      // If the button wasn't pressed before, and it has changed states (meaning
      //  it was just pressed)
      if (lastDebouncedSwitches[player] & switchDebouncedDiffs[player] & temp)
      {

        // If the game is currently being played
        if ((gameStage > 0) && (gameStage < 5))
        {

          // If a mole button
          if (temp < (1 << NUM_PLAYER_ACTION_SWITCHES))
          {

            // If this player's mole selection is the correct selection and the
            //  mole is currently being presented
            if ((temp == moleLocation[playerIndex[player]]) &&
                (moleMode[player] == 2))
            {

              // Note that this player's mole has advanced.
              moleAdvance[player] = 1;

              // Play advance tone.  This sets the counter for the number of
              //  oscillations to be played for the advance tone.  This will
              //  produce a 0.1 second duration.
              advanceToneFlips = 100;
            }

            // Else this is an incorrect selection
            else
            {

              // Assess ~2.0 seconds penalty
              playerPenalty[player] += 2;

              // Play penalty tone. This sets the counter for the number of
              //  primary penalty tone oscillations to be played.  This will
              //  produce an approximately 0.5 second duration.
              penaltyTone1Flips = 125;
            }
          }

          // Else if this is a player command button
          else if (temp == PLAYER_COMMAND_BUTTON)
          {

            // If the sound is currently enabled
            if (soundEnabled == SOUND_ON)
            {

              // Turn off the sound.
              soundEnabled = SOUND_OFF;
            }

            // else if the sound is currently disabled
            else if (soundEnabled == SOUND_OFF)
            {

              // Start the sound.
              soundEnabled = SOUND_START;
            }
          }
        }

        // Else, the game hasn't started yet and a control switch was just
        //  pressed.
        else if ((gameStage == 0) && (temp == PLAYER_COMMAND_BUTTON))
        {

          // Neither player is ahead or has a penalty.
          // playerAhead[PLAYER_1] = 0; // Already done at reset.
          // playerAhead[PLAYER_2] = 0; // Already done at reset.
          // playerPenalty[PLAYER_1] = 0; // Already done at reset.
          // playerPenalty[PLAYER_2] = 0; // Already done at reset.

          // Set the first mole.
          // playerIndex[PLAYER_1] = 0; // Already done at reset.
          // playerIndex[PLAYER_2] = 0; // Already done at reset.

          // Grab the current running timer value (hoping this is psuedo-random
          //  as this depends on the user's input).  Adding 1 to it as the seed
          //  can't be 0 (limits of TCNT1L are 0x00 and 0xFE).
          prand = TCNT1L + 1;

          // Set the delay for the first mole (equivalent to between 0 and 3.06
          //  seconds for gameStage 1).  Note: random number generator has
          //  already been seeded and advanced.
          moleDelay[0] = prand;

          // Set the location for the first mole.
          moleLocation[0] = 1 << (prand & 0x3);

          // Set the timers for both players (~1.0 seconds for gameStage 1).
          playerTimer[PLAYER_1] = 83;
          playerTimer[PLAYER_2] = 83;

          // If the player 1 command switch was pressed
          if (player == PLAYER_1)
          {

            // Turn the sound on (starting at the beginning of the song).
            soundEnabled = SOUND_START;
          }

          // Neither mole is present.
          // moleMode[PLAYER_1] = 0; // Already done at reset.
          // moleMode[PLAYER_2] = 0; // Already done at reset.

          // Start the game.
          gameStage = 1;
        }
      }
    }

    // Remember the current state of the debounced switches for comparison
    //  during the next frame.
    lastDebouncedSwitches[player] = debouncedPlayerSwitches[player];

    // Decrement the index
    player--;

    // Part of the program space-saving hack.
  } while (player != 255);
}

// Figure out which mode the moles are in.
void process_mole_mode(void)
{

  // Take care of the timing and modes of the moles for both players.
  // Changed what was a for loop to a do loop to save a few more bytes to
  //  squeeze the software inside the 2K program space.
  player = PLAYER_2;
  do
  {

    // Decrement timer
    playerTimer[player] --;

    // If the timer ran out
    if (playerTimer[player] == 0)
    {

      // If this is the end of the initial wait
      if (moleMode[player] == 0)
      {

        // Set the timer to the psuedo-random wait.
        playerTimer[player] = moleDelay[playerIndex[player]];

        // Advance to the next mole mode.
        moleMode[player] ++;
      }

      // Else if this is the end of the psuedo-random wait plus penalty
      else if (moleMode[player] == 1)
      {

        // if there is a penalty assessed
        if (playerPenalty[player])
        {

          // Begin the penalty timer (~1.0 seconds in gameStage 1).
          playerTimer[player] = 83;

          // Decrement the penalty counter
          playerPenalty[player] --;
        }

        // else (there is no penalty assessed)
        else
        {

          // Turn the mole light on.
          lightPlayerStates[player] &= ~moleLocation[playerIndex[player]];

          // Set the timer for the presentation (~1.0 seconds in gameStage 1).
          playerTimer[player] = 83;

          // Play the mole tone.  This sets the counter for the number of
          //  oscillations to be played.  This will produce a 0.051 second
          //  duration mole tone.
          moleToneFlips = 255;

          // Advance to the next mole mode.
          moleMode[player] ++;
        }
      }

      // Else (this is not the end of the initial wait or the
      //  psuedo-random/penalty wait; therefore, the mole must currently be
      //  presented and the player failed to make smash the mole in time.
      else // if (moleMode[player] == 2)
      {

        // Assess ~3.0 seconds penalty.
        playerPenalty[player] += 3;

        // Play penalty tone. This sets the counter for the number of primary
        //  penalty tone oscillations to be played.  This will produce an
        //  approximately 0.5 second duration.
        penaltyTone1Flips = 125;

        // Advance to the next mole.
        moleAdvance[player] = 1;
      }
    }

    // Calculate the new index used for moleLocation[] and moleDelay[].
    //  If player mole has advanced
    if (moleAdvance[player])
    {

      // Advance this player's current mole
      playerIndex[player] ++;

      // If the index is beyond the end of the location/delay arrays
      if (playerIndex[player] >= 11)
      {

        // Set the index to the beginning of the array.
        playerIndex[player] = 0;
      }
    }

    // Decrement the index
    player--;

    // Part of the program space-saving hack.
  } while (player != 255);
}

// Figure out which player is ahead and which game stage is currently being
//  played.
void process_game_stage(void)
{

  // Used to remember a previous game stage to add some hysteresis to the
  //  behaviour of the song restarting when the pace of the game changes.
  static uint8_t previousGameStage;

  // Local variable needed only by this routine that is used to calculate the
  //  difference between the two players' progress.
  int8_t moleDifference;

  // Local variable needed only by this routine that is used to keep track of if
  //  the two players are tied.
  uint8_t playersTied;

  // If the game is currently being played
  if ((gameStage > 0) && (gameStage < 5))
  {

    // Figure out which mode the moles are in.
    process_mole_mode();

    // Determine which player is ahead (and by how much).
    //  Fiddling around with the logic led to setting the following to start:
    playerAhead[PLAYER_1] = 0;
    playerAhead[PLAYER_2] = 0;

    // The difference of progress (in moles)
    moleDifference = playerIndex[PLAYER_2] - playerIndex[PLAYER_1];

    // Figuring out the difference and which player is ahead relies on the
    //  maximum difference between the two players' indexes.
    if (moleDifference >= 6)
    {

      // Player 1's index is wrapped ahead and past the end of the array.
      moleDifference = 11 - moleDifference;
      playerAhead[PLAYER_1] = 1;
    }

    // else if player 2 is ahead (and isn't wrapped around end of array)
    else if (moleDifference > 0)
    {
      playerAhead[PLAYER_2] = 1;
    }

    // else if player 2's index is wrapped ahead and past the end of the array
    else if (moleDifference < -5)
    {

      // Mole difference is the length of the array plus (the negative)
      //  difference.
      moleDifference = 11 + moleDifference;
      playerAhead[PLAYER_2] = 1;
    }

    // else if player 1 is ahead (and isn't wrapped arround end of array)
    else if (moleDifference < 0)
    {

      // Set difference to negative of calculation above.
      moleDifference = -moleDifference;
      playerAhead[PLAYER_1] = 1;
    }

    // else if player 1's mole mode is less than player 2's mole mode (and
    //  the moleDifference == 0)
    else if (moleMode[PLAYER_1] < moleMode[PLAYER_2])
    {

      // The players are on the same mole, but player 2 is in a later mole mode.
      playerAhead[PLAYER_2] = 1;
    }

    // else if player 2's mole mode is less than player 1's mole mode (and
    //  the moleDifference == 0)
    else if (moleMode[PLAYER_2] < moleMode[PLAYER_1])
    {

      // The players are on the same mole, but player 2 is in a later mole mode.
      playerAhead[PLAYER_1] = 1;
    }

    // else if player 1's timer is less than player 2's timer (and the
    //  moleDifference == 0 and the players' mole modes are the same)
    else if (playerTimer[PLAYER_1] < playerTimer[PLAYER_2])
    {

      // The players are on the same mole and the same mole mode, but player 1's
      //  timer value is smaller.
      playerAhead[PLAYER_1] = 1;
    }

    // else if player 2's timer is less than player 1's timer (and the
    //  moleDifference == 0 and the players' mole modes are the same)
    else if (playerTimer[PLAYER_2] < playerTimer[PLAYER_1])
    {

      // The players are on the same mole and the same mole mode, but player 2's
      //  timer value is smaller.
      playerAhead[PLAYER_2] = 1;
    }

    // else (if the moleDifference == 0 and the players' mole modes are the same
    //  and player's timers are the same)
    else
    {

      // The players are tied.
      playersTied = 1;
    }

    // Take care of the timing and modes of the moles for both players.
    // Changed what was a for loop to a do loop to save a few more bytes to
    //  squeeze the software inside the 2K program space.
    player = PLAYER_2;
    do
    {

      // If player's mole has advanced.
      if (moleAdvance[player])
      {

        // If player is ahead or the players are tied.
        if (playerAhead[player] || playersTied)
        {

          // Advance random number generator to the next state.
          lfsr_prand();

          // Set the delay for the next mole (equivalent to between 0 and 1.524
          //  seconds for gameStage 1).
          moleDelay[playerIndex[player]] = prand >> 1;

          // Set the location for the mole.
          moleLocation[playerIndex[player]] = 1 << (prand & 0x3);

          // Reset this flag so we don't set the delay and location twice in the
          //  case of a tie.
          //playersTied = 0; // Actually, it doesn't matter if we do this, so
                             //  we'll save program space by not.
        }

        // Turn the mole lights off.
        lightPlayerStates[player] |= PLAYER_MOLES_MASK;

        // Set the timer for player (equivalent to 0.5 seconds for gameStage 1).
        // Note of interest: Changing this constant from 125 to 42 reduced the
        //  program space by 46 bytes.
        playerTimer[player] = 42;

        // Reset the penalty.
        playerPenalty[player] = 0;

        // Reset the mole mode.
        moleMode[player] = 0;

        // Zero out the mole advanced status since we've taken care of it.
        moleAdvance[player] = 0;
      }

      // Decrement the index
      player--;

      // Part of the program space-saving hack.
    } while (player != 255);

    // If conditions indicate that game stage has changed
    if (!((gameStage == moleDifference) || (moleDifference == 0)))
    {

      // The game's stage is set to the difference in the players' moles.
      gameStage = moleDifference;

      // If the difference in moles (progress) between the two players is not
      //  the previously saved-off game stage (this was done to add some
      //  hysteresis to the resetting of the song when both players were
      //  continuously taking the lead back from each other)
      if (moleDifference != previousGameStage)
      {

        // If end of game has been reached
        if (gameStage == 5)
        {

          // Display victor light
          // If player 1 is the winner
          if (playerAhead[PLAYER_1])
          {

            // Turn this player's lights on.
            lightPlayerStates[PLAYER_1] = PLAYER_LIGHTS_ON;

            // Turn this player's lights off.
            lightPlayerStates[PLAYER_2] = PLAYER_LIGHTS_OFF;
          }

          // else (player 2 is the winner)
          else // if (playerAhead[PLAYER_2])
          {

            // Turn this player's lights on.
            lightPlayerStates[PLAYER_2] = PLAYER_LIGHTS_ON;

            // Turn this player's lights off.
            lightPlayerStates[PLAYER_1] = PLAYER_LIGHTS_OFF;
          }
        }
      }

      // Save off the previous game stage so that next time the above comparison
      //  is done, it will be comparing what the game stage is going to be set
      //  that time to what it was two changes previously.
      previousGameStage = gameStage;
    }
  }
}

// Figure out what the status lights should be doing.
void process_status_lights(void)
{

  // Toggle the player status light.
  // If a new note started playing and this isn't the end of the game.
  if ((gameStage < 5) && newNote)
  {

    // Reset the flag so this doesn't happen again until a new note is played.
    newNote = 0;

    // If the game hasn't started yet, turn the mole lights off and toggle the
    //  status lights (and make the players' status lights be opposite of each
    //  other).
    if (gameStage == 0)
    {

      // Turn off player 1's mole lights.
      lightPlayerStates[PLAYER_1] |= PLAYER_MOLES_MASK;

      // Toggle player 1's status lights.
      lightPlayerStates[PLAYER_1] ^= PLAYER_STATUS_MASK;

      // Set player 2's lights the same as player 1's lights.
      lightPlayerStates[PLAYER_2] = lightPlayerStates[PLAYER_1];

      // Toggle player 2's status lights.
      lightPlayerStates[PLAYER_2] ^= PLAYER_STATUS_MASK;
    }

    // else (game stage is not 0; this means we're in normal game play mode)
    else
    {

      // Take care of the status lights for both players.
      // Changed what was a for loop to a do loop to save a few more bytes to
      //  squeeze the software inside the 2K program space.
      player = PLAYER_2;
      do
      {

        // If this player is currently ahead
        if (playerAhead[player])
        {

          // Toggle this player's status light.
          lightPlayerStates[player] ^= PLAYER_STATUS_MASK;
        }

        // else (this player is not currently ahead)
        else
        {

          // Turn this player's status light off.
          lightPlayerStates[player] |= PLAYER_STATUS_MASK;
        }

        // Decrement the index
        player--;

        // Part of the program space-saving hack.
      } while (player != 255);
    }
  }
}

// Update LED display.  Note that the LEDs are connected between U3/U5 and VCC.
//  Thus, they illuminate when the outputs on U3/U5 go *low*, so 0="LED on" and
//  1="LED off".
void task_2_write_LEDs(void)
{

  // Set port B pins as output
  DDRB = 0b11111111;

  // Load port B with the lower eight commanded LED states.  See definition of
  //  lightPlayerStates[] to make sense of the following.
  //      Bits 0 and 1 of PORTB correspond to bits 0 and 1 of
  //       lightPlayerStates[PLAYER_1].
  PORTB = (lightPlayerStates[PLAYER_1] & 0x03) |
  //      Bit 2 of PORTB corresponds to bit 4 of lightPlayerStates[PLAYER_1].
          ((lightPlayerStates[PLAYER_1] & 0x10) >> 2) |
  //      Bits 5 and 6 of PORTB correspond to bits 2 and 3 of
  //       lightPlayerStates[PLAYER_1].
          ((lightPlayerStates[PLAYER_1] & 0x0C) << 3) |
  //      Bits 3 and 4 of PORTB correspond to bits 0 and 1 of
  //       lightPlayerStates[PLAYER_2].
          ((lightPlayerStates[PLAYER_2] & 0x03) << 3) |
  //      Bit 7 of PORTB corresponds to bit 4 of lightPlayerStates[PLAYER_2].
          ((lightPlayerStates[PLAYER_2] & 0x10) << 3);

  // Enable the LED latches.  This actually turns the LEDs to their commanded
  //  state.
  PORTD |= (1 << LED_A_WRITE_LATCH);

  // Disable the LED latches so future changes to port B don't change the LEDs'
  //  states.
  PORTD &= ~(1 << LED_A_WRITE_LATCH);

  // Load port B with the upper two commanded LED states.
  //  See definition of lightPlayerStates[] to make sense of the following.
  //      Bits 0 and 1 of PORTB correspond to bits 2 and 3 of
  //       lightPlayerStates[PLAYER_2].
  PORTB = (lightPlayerStates[PLAYER_2] & 0x0C) >> 2;

  // Repeat as before.
  PORTD |= (1 << LED_B_WRITE_LATCH);
  PORTD &= ~(1 << LED_B_WRITE_LATCH);

  // Reset the task timer.
  taskTimers[2] = 4; // Run every 4.0ms
  taskBits &= ~(0x04);
}

// Play a song.
void task_3_play_song(void)
{

  // Declare this as static so it will be remembered from one frame to the next.
  static uint8_t musicIndex;

  // Local variable needed only by this routine.
  uint8_t notePair;

  // If the sound mode is commanded to turn on
  if (soundEnabled == SOUND_START)
  {

    // Reset to the beginning of the song.
    musicIndex = 0;

    // Set the sound mode to on.
    soundEnabled = SOUND_ON;
  }

  // If the sound is enabled
  if (soundEnabled == SOUND_ON)
  {

    // Enable the TIMER1 interrupt.
    TIMSK |= (1<<OCIE1A);
  }

  // If the sound mode is disabled
  else if (soundEnabled == SOUND_OFF)
  {

    // Disable the TIMER1 interrupt.
    TIMSK &= ~(1<<OCIE1A);
  }

  // Get the new pair of notes (only one of which we'll be playing this frame).
  notePair = eeprom_read_byte(&music[musicIndex>>1]);

  // If musicIndex is odd
  if (musicIndex & 0x01)
  {

    // Select the least significant nibble.
    notePair &= 0x0F;
  }

  // Else musicIndex is even.
  else
  {

    // Select the most significant nibble.
    notePair >>= 4;
  }

  // Increment the music index for the next frame.
  musicIndex++;

  // Set the period of the timer using the nibble corresponding to the note in
  //  the notePeriods array.
  OCR1A = eeprom_read_word(&notePeriods[notePair]);

  // Reset the task timer.
  // Run at a speed that's based on the current game stage.
  //  Stage 5: 96 msec
  //  Stage 4: 64 msec
  //  Stage 3: 96 msec
  //  Stage 2: 128 msec
  //  Stage 1: 160 msec
  //  Stage 0: 192 msec
  // Equivalent to (6 - gameStage) * 32
  // If game stage is 5
  if (gameStage == 5)
  {

    // Set to this constant vale.
    taskTimers[3] = 96;

    // If this is the end of the main melody
    if (musicIndex < NUM_NOTES)
    {

       // Set the index to start playing the final song.
       musicIndex = NUM_NOTES;
    }
  }

  // Else (is not game stage of 5)
  else
  {

    // The following equation equates to the table above.
    taskTimers[3] = (6 - gameStage) << 5;
  }
  taskBits &= ~(0x08);

  // If we're going to be beyond the end of the main melody
  if (musicIndex >= NUM_NOTES)
  {

    // If this is not the end of the game
    if (gameStage != 5)
    {

      // Reset the index to the beginning of the main game play melody.
      musicIndex = 0;
    }

    // Else (this is the end of the game)
    else if (musicIndex == FINAL_NOTE - 1)
    {

      // Disable the TIMER1 interrupt to turn the music off.
      TIMSK &= ~(1<<OCIE1A);

      // Enable the watchdog timer so the MCU will reset in 4 seconds.
      //  Resetting will zero out everything that needs to be zeroed out, and
      //  get the system ready to restart the game.  The procedure to do this
      //  is not intuitive.  See attiny2313's datasheet for code example (and
      //  caveats) of how to do this.
      // Disable all interrupts
      cli();

      //   Start timed equence
      WDTCSR |= (1<<WDCE) | (1<<WDE);

      // Set new prescaler(time-out) value = 128K cycles (~1.0 s) */
      WDTCSR = (1<<WDE) | (1<<WDP2) | (1<<WDP1);

      // Enable interrupts.
      sei();

      // Go into an infinite loop so the watchdog timer will reset the MCU.
      while (1)
      {
      }
    }
  }

  // Let the game logic task know another note was played.
  newNote = 1;
}

// Uncomment these as additional tasks are needed.
//void task_4(void)
//{

  // Reset the task timer.
  //taskTimers[4] = X; // Run every X.0 milliseconds.
  //taskBits &= ~(1 << 4);
//}

//void task_5(void)
//{

  // Reset the task timer.
  //taskTimers[5] = X; // Run every X.0 milliseconds.
  //taskBits &= ~(1 << 5);
//}

//void task_6(void)
//{

  // Reset the task timer.
  //taskTimers[6] = X; // Run every X.0 milliseconds.
  //taskBits &= ~(1 << 6);
//}

//void task_7(void)
//{

  // Reset the task timer.
  //taskTimers[7] = X; // Run every X.0 milliseconds.
  //taskBits &= ~(1 << 7);
//}

// This interrupt routine will automagically be called everytime TIMER0 has
//  overflowed.  See timer0 stuff in init_devices to see how the timer was
//  setup.
ISR(TIMER0_COMPA_vect)
{

  // TIMER0 has overflowed
  tickFlag++;
}

// This interrupt routine will automagically be called everytime TIMER1 has
//  overflowed.  See timer1 stuff in init_devices() to see how the timer was
//  setup.
ISR(TIMER1_COMPA_vect)
{

  // Toggle the speaker pins to make a click.
  PORTD ^= (1 << SPKR_PIN_1);
}
