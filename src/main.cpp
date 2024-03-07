/*

  This code combines the Clock Multiplier and the Random Trigger which run on the same hardware in one 
  program which I use to power my eurorack module 'Clock-Multiply-and-Random-Trigger-O-Matic. 
  Switching between the 2 applications can be done by long pressing or double clicking the mode button.
  The Clock Multiplier and the Random Trigger software were published on www.BummBummGarage.com.
  Have a look at that website for more info on both programs.
  I've converted both pieces of code into C++ classes and built a wrapper around it (main.cpp).

  J.S. Bouten aka Zaphod B, Jan 2024

  Creative Commons yeah! License: CC BY-NC 4.0
*/

#include <Arduino.h>

#include "OneButton.h"

//#define DEBUG // Enables the Serial print in several functions. Slows down the frontend.

#ifdef DEBUG
  #define debug_begin(x) serial_begin(x)
  #define debug_print(x) printf(x)
  #define debug_print2(x, y) printf(x, y)
  #define debug_print3(x, y, z) printf(x, y, z)
#else
  #define debug_begin(x)
  #define debug_print(x)
  #define debug_print2(x, y)
  #define debug_print3(x, y, z)
#endif

#include "ClockMultiplier.hpp"
#include "RandomTriggers.hpp"

const bool CLOCK_MULTIPLIER = true;
const bool RANDOM_TRIGGER = false;

bool inClockMultiplierMode = CLOCK_MULTIPLIER;
int triggerInPin = A5;
int triggerInLEDPin = 3; 
int quantityPotiPin = A3;
int quantityCVPin = A4;
int distributionPotiPin = A2;
int toggleAndMutePin = 2;
int triggerOutLEDPin = 5;
int triggerOutPin = 6;
int modeClockMultiplierLedPin = 10; // D10 pwm capable pin for indicating Clock Multiplier mode.
int modeRandomTriggerLedPin = 9;    // D9  pwm capable pin for indicating Random Trigger mode.

// Note all outputs (3, 5, 9, 10) chosen to connect LEDs to are PWM capable!

const int MODE_LED_HIGH_BRIGHTNESS = 150;

int densitiyPotiPin = A2;
int lengthPotiPin = A3;

ClockMultiplier clockMultiplier = 
  ClockMultiplier(triggerInPin, 
                  triggerInLEDPin, 
                  quantityPotiPin, 
                  quantityCVPin, 
                  distributionPotiPin, 
                  triggerOutLEDPin, 
                  triggerOutPin);


RandomTriggers randomTriggers = 
  RandomTriggers(triggerInLEDPin, 
                 triggerInPin, 
                 densitiyPotiPin, 
                 lengthPotiPin, 
                 triggerOutLEDPin, 
                 triggerOutPin);

bool inMutedState = false;

OneButton button(toggleAndMutePin, false);

// When the button was pressed 1 time we toggle the mute state, this is for the Clock Multiplier only.
void myClickFunction() {
  if (inClockMultiplierMode) {
    inMutedState = !inMutedState;
  }
} 

void updateModeLeds() {
  if (inClockMultiplierMode) {
    analogWrite(modeClockMultiplierLedPin, MODE_LED_HIGH_BRIGHTNESS);
    analogWrite(modeRandomTriggerLedPin, 0);
  } else {
    analogWrite(modeClockMultiplierLedPin, 0);
    analogWrite(modeRandomTriggerLedPin, MODE_LED_HIGH_BRIGHTNESS);
  }
}

// When the button was pressed 2 times we change from Clock Multiplier to Random Trigger generator.
void myDoubleClickFunction() {
  inClockMultiplierMode = !inClockMultiplierMode;
  updateModeLeds();
}

// This function will be called when the button is released.
void LongPressStop(void *oneButton) {
    inClockMultiplierMode = !inClockMultiplierMode;
    updateModeLeds();
}

void setup() {
  debug_begin(230400); // Initialize serial communication at 9600 bits per second.
  pinMode(modeClockMultiplierLedPin, OUTPUT);
  pinMode(modeRandomTriggerLedPin, OUTPUT);
  
  // Link the myClickFunction function to be called on a click event.
  button.attachClick(myClickFunction);

  // Link the doubleclick function to be called on a doubleclick event.
  button.attachDoubleClick(myDoubleClickFunction);  

  // link functions to be called on events.
  button.attachLongPressStop(LongPressStop, &button);
  button.setLongPressIntervalMs(1000);

  // Pin settings for Clock Multiplier and Random Trigger.
  pinMode(triggerInPin, INPUT);
  pinMode(triggerInLEDPin, OUTPUT);
  pinMode(quantityPotiPin, INPUT);
  pinMode(quantityCVPin, INPUT);
  pinMode(distributionPotiPin, INPUT);
  pinMode(triggerOutLEDPin, OUTPUT);
  pinMode(triggerOutPin, OUTPUT);
}

void loop() {
  #ifdef DEBUG
    showMode();
  #endif
  static bool only_once = true;
  if (only_once) {
    only_once = false;
    updateModeLeds();
  }
  if (inClockMultiplierMode == CLOCK_MULTIPLIER) {
    clockMultiplier.tick(inMutedState);
  } else {
   randomTriggers.tick();
  }
  button.tick();
}