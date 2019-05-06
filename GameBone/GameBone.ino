/* 
GameBone Handheld Electronic Game
ATtiny85 @ 1 MHz (internal oscillator; BOD disabled)

David Johnson-Davies - www.technoblogy.com - 23rd January 2017

CC BY 4.0
Licensed under a Creative Commons Attribution 4.0 International license:
http://creativecommons.org/licenses/by/4.0/

Circuit:
ATtiny85:
  http://www.technoblogy.com/show?1K2Z
  PB0 = Orange LED & button
  PB1 = Buzzer (OC1A)
  PB2 = Blue LED & button
  PB3 = Red LED & button
  PB4 = Green LED & button
  PB5 = Reset

ATtiny84:
  PA0 = Orange
  PA1 = Blue
  PA2 = Red
  PA3 = Green
  PA4 = NA
  PA5 = NA
  PA6 = NA
  PA7 = NA

  PB0 = NA
  PB1 = NA
  PB2 = Buzzer (OC0A)
  PB3 = reset

*/

#include <avr/power.h>
#include <avr/sleep.h>

// #define USE_ATTINY85
#define USE_ATTINY84

const int BEAT = 250;
const int MAXIMUM = 32;

// #ifdef USE_ATTINY85
//   #define PB0 0
//   #define PB1 1
//   #define PB2 2
//   #define PB3 3
//   #define PB4 4
//   #define PB5 5
// #endif

// #ifdef USE_ATTINY84
//   #define 

// static_assert(PB2==3, "nope");

// Define Arduino Pins:

#ifdef USE_ATTINY85
#define PIN_BLUE    2 // PB2 // Arduino pin 2
#define PIN_ORANGE  0 // PB0 // Arduino pin 0
#define PIN_RED     3 // PB3 // Arduino pin 3
#define PIN_GREEN   4 // PB4 // Arduino pin 4
#endif

#ifdef USE_ATTINY84
#define PIN_BLUE    9  // PA1 // Arduino pin 9
#define PIN_ORANGE  10 // PA0 // Arduino pin 10
#define PIN_RED     8  // PA2 // Arduino pin 8
#define PIN_GREEN   7  // PA3 // Arduino pin 7
#endif

// // Button pin *indices*:
// enum button_e 
// {
//   BLUE = 0,
//   ORANGE,
//   RED,
//   GREEN,
// };

// Buttons (*Arduino* pins):
int pins[] = {PIN_BLUE, PIN_ORANGE, PIN_RED, PIN_GREEN};

// Notes:       E4  C#4 A4  E3
int notes[] = { 52, 49, 57, 40 };

int sequence[MAXIMUM];

// Simple Tones **********************************************

const uint8_t scale[] PROGMEM = {239, 226, 213, 201, 190, 179, 169, 160, 151, 142, 134, 127};

// @param[in] n       Note (pd? freq?)
// @param[in] octave  
void note(int n, int octave) 
{
  #ifdef USE_ATTINY85
  DDRB |= (1 << DDB1);                   // PB1 (Arduino pin 1) as output for buzzer
  int prescaler = 8 - (octave + n / 12);
  if (prescaler < 1 || prescaler > 7) prescaler = 0;

  OCR1C = pgm_read_byte(&scale[n % 12]) - 1;
  TCCR1 = (1 << CTC1) | (1 << COM1A0) | prescaler;
  #endif

  #ifdef USE_ATTINY84
  DDRB |= (1 << DDB2);                   // PB2 as output for buzzer
  int prescaler = 8 - (octave + n / 12);
  if (prescaler < 1 || prescaler > 7) prescaler = 0;

  // 1. Set Waveform Generation Mode to 2 (CTC): WGM02/WGM01/WGM00 must be 0/1/0
  // - see ATtiny84 datasheet p83
  // 2. Set Compare Output Mode, non-PWM Mode to Toggle OC0A on Compare Match: COMOA1/COM0A0 = 0/1
  // - see datasheet p80
  OCR0A = pgm_read_byte(&scale[n % 12]) - 1;
  TCCR0A &= ~(1 << COM0A1 | 1 << WGM00);
  TCCR0A |= 1 << COM0A0 | 1 << WGM01;
  TCCR0B &= ~(1 << WGM02);
  #endif
}

// Button routines **********************************************

// Pin change interrupt on any pin wakes us up
// ATtiny85: PCINT0 to PCINT5, inclusive (6 pins)
ISR (PCINT0_vect)
{
  // do nothing
}

// Turn on specified button's LED
void button_on(int button) 
{
  int p = pins[button];
  pinMode(p, OUTPUT);
  digitalWrite(p, LOW);
}

// Turn off specified button
void button_off(int button) 
{
  int p = pins[button];
  pinMode(p, INPUT_PULLUP);
}

// Flash an LED and play the corresponding note of the correct frequency
void flashbeep(int button) 
{
  button_on(button);
  note(notes[button], 0);
  delay(BEAT);
  note(0, 0);
  button_off(button);
}

// Play a note and flash the wrong button
void misbeep(int button) 
{
  int wrong = (button + random(3) + 1) % 4;
  button_on(wrong);
  note(notes[button], 0);
  delay(BEAT);
  note(0, 0);
  button_off(wrong);
}

// Wait until a button is pressed and play it
int check()
{
  #ifdef USE_ATTINY85
  GIMSK = GIMSK | 1 << PCIE;          // Enable pin change interrupt
  sleep();
  GIMSK = GIMSK & ~(1 << PCIE);       // Disable pin change interrupt
  #endif

  #ifdef USE_ATTINY84
  GIMSK = GIMSK | 1 << PCIE0;          // Enable pin change interrupt
  sleep();
  GIMSK = GIMSK & ~(1 << PCIE0);       // Disable pin change interrupt
  #endif

  int button = 0;
  do {
    button = (button + 1) % 4;
  } while (digitalRead(pins[button]));
  flashbeep(button);
  return button;
}

void success_sound() 
{
  note(48, 0); delay(125);
  note(0, 0);  delay(125);
  note(52, 0); delay(125);
  note(0, 0);  delay(125);
  note(55, 0); delay(125);
  note(0, 0);  delay(125);
  note(60, 0); delay(375);
  note(0, 0);
}

void fail_sound() 
{
  note(51, 0); delay(125);
  note(0, 0);  delay(125);
  note(48, 0); delay(375);
  note(0, 0);
}

// Simon **********************************************

void simon() 
{
  int turn = 0;
  sequence[0] = random(4);
  do {
    for (int n = 0; n <= turn; n++) 
    {
      delay(BEAT);
      flashbeep(sequence[n]);
    }
    for (int n = 0; n <= turn; n++) 
    {
      if (check() != sequence[n]) 
      {
        fail_sound();
        return;
      }
    }
    sequence[turn + 1] = (sequence[turn] + random(3) + 1) % 4;
    turn++;
    delay(BEAT);
  } while (turn < MAXIMUM);
  success_sound();
}

// Echo **********************************************

void echo() 
{
  int turn = 0;
  sequence[turn] = check();
  do {
    for (int n = 0; n <= turn; n++) 
    {
      if (check() != sequence[n]) 
      {
        fail_sound();
        return;
      }
    }
    sequence[turn + 1] = check();
    turn++;
    delay(BEAT);
  } while (turn < MAXIMUM);
  success_sound();
}

// Quiz **********************************************

void quiz() 
{
  do {
    int button = check();
    button_on(button);
    delay(3000);
    button_off(button);
  } while (1);
}

// Confusion **********************************************

void confusion() 
{
  int turn = 0;
  sequence[0] = random(4);
  do {
    for (int n = 0; n <= turn; n++) 
    {
      delay(BEAT);
      if (turn > 1 && n < turn) 
      {
        misbeep(sequence[n]);
      }
      else 
      {
        flashbeep(sequence[n]);
      }
    }
    for (int n = 0; n <= turn; n++) 
    {
      if (check() != sequence[n]) 
      {
        fail_sound();
        return;
      }
    }
    sequence[turn + 1] = (sequence[turn] + random(3) + 1) % 4;
    turn++;
    delay(BEAT);
  } while (turn < MAXIMUM);
  success_sound();
}

// Setup and loop **********************************************

void sleep(void)
{
  sleep_enable();
  sleep_cpu();
}

void setup() 
{
  // Set up pin change interrupts for all 4 buttons
  #ifdef USE_ATTINY85
  PCMSK = 1 << PCINT0 | 1 << PCINT2 | 1 << PCINT3 | 1 << PCINT4;
  #endif

  #ifdef USE_ATTINY84
  PCMSK0 = 1 << PCINT0 | 1 << PCINT2 | 1 << PCINT3 | 1 << PCINT4;
  #endif

  ADCSRA &= ~(1 << ADEN); // Disable ADC to save power
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  // After reset flash all four lights
  for (int button = 0; button < 4; button++) 
  {
    button_on(button);
    delay(100);
    button_off(button);
  }
  // Wait for button to select game
  int game = 0;
  do {
    game = (game + 1) % 4;
    if (millis() > 10000) sleep();
  } while (digitalRead(pins[game]));
  randomSeed(millis());
  delay(250);
  if (game == 3) simon();
  else if (game == 2) echo();
  else if (game == 1) quiz();
  else confusion();
}

// Only reset should wake us now
void loop() 
{
  sleep();
}
