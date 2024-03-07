
/* --------------------------------------------------------------------------
    $$$$$$$\  $$\   $$\ $$\      $$\ $$\      $$\
    $$  __$$\ $$ |  $$ |$$$\    $$$ |$$$\    $$$ |
    $$ |  $$ |$$ |  $$ |$$$$\  $$$$ |$$$$\  $$$$ |
    $$$$$$$\ |$$ |  $$ |$$\$$\$$ $$ |$$\$$\$$ $$ |
    $$  __$$\ $$ |  $$ |$$ \$$$  $$ |$$ \$$$  $$ |
    $$ |  $$ |$$ |  $$ |$$ |\$  /$$ |$$ |\$  /$$ |
    $$$$$$$  |\$$$$$$  |$$ | \_/ $$ |$$ | \_/ $$ |
    \_______/  \______/ \__|     \__|\__|     \__|

    $$$$$$$\  $$\   $$\ $$\      $$\ $$\      $$\
    $$  __$$\ $$ |  $$ |$$$\    $$$ |$$$\    $$$ |
    $$ |  $$ |$$ |  $$ |$$$$\  $$$$ |$$$$\  $$$$ |
    $$$$$$$\ |$$ |  $$ |$$\$$\$$ $$ |$$\$$\$$ $$ |
    $$  __$$\ $$ |  $$ |$$ \$$$  $$ |$$ \$$$  $$ |
    $$ |  $$ |$$ |  $$ |$$ |\$  /$$ |$$ |\$  /$$ |
    $$$$$$$  |\$$$$$$  |$$ | \_/ $$ |$$ | \_/ $$ |
    \_______/  \______/ \__|     \__|\__|     \__|

     $$$$$$\   $$$$$$\  $$$$$$$\   $$$$$$\   $$$$$$\  $$$$$$$$\
    $$  __$$\ $$  __$$\ $$  __$$\ $$  __$$\ $$  __$$\ $$  _____|
    $$ /  \__|$$ /  $$ |$$ |  $$ |$$ /  $$ |$$ /  \__|$$ |
    $$ |$$$$\ $$$$$$$$ |$$$$$$$  |$$$$$$$$ |$$ |$$$$\ $$$$$\
    $$ |\_$$ |$$  __$$ |$$  __$$< $$  __$$ |$$ |\_$$ |$$  __|
    $$ |  $$ |$$ |  $$ |$$ |  $$ |$$ |  $$ |$$ |  $$ |$$ |
    \$$$$$$  |$$ |  $$ |$$ |  $$ |$$ |  $$ |\$$$$$$  |$$$$$$$$\
     \______/ \__|  \__|\__|  \__|\__|  \__| \______/ \________|

    CLOCK MULTIPLIER (Rev2)
    This is the code for an Eurorack synth module.
    It's built and tested on an ATMega 328p.

    Find out more on: https://www.bummbummgarage.com/modules/clock-multiplier/

    Creative Commons yeah! License: CC BY-NC 4.0

    ---------------------------------------------------------------------- */

/*  
 *  J.S. Bouten 2023-08-21
 *  - debug changed into compiler #ifdef
 *  - unsigned long warnings resolved
 *  - replaced Serial.print by printf ( from libprintf ).
 *  - moved setup() and loop() to bottom of text
 *  - adapted for using a transistor as output instead of an opamp
 */

#include <LibPrintf.h>

#define USE_TRANSISTOR // When using a transistor as output instead of an opamp.

// General
//#define DEBUG // Enables the Serial print in several functions. Slows down the frontend.
const bool internalClock = false; // Refuses the triggers and starts the internal clock. Meant for developing / debugging.
const int internalClockBpm = 70; // The speed for the internal clock.
const int pushButtonDelay = 50; // The time the button will be insensitive after last change.


// Trigger IN
int triggerInPin = A5;
int triggerInLEDPin = 3;

long triggerInHigh; // Timestamp of the latest trigger high.
long triggerInLow; // Timestamp of the latest trigger low.

// Cycles
long cycleStart = 0; // Timestamp of when the last cycle began.
int cycleTime = 0; // The absolute time span one cycle has in the given settings.


// Quantity
const int quantityPotiPin = A3;
const int quantityCVPin = A4;
const int maxQuantity = 8; // The maximum amount of possible triggers out per cycle.
int currentQuantity; // The current amount of triggers per cycle.


// Distribution
const int distributionPotiPin = A2;
const int maxDistribution = 11; // The amount of different distribution patterns.
int currentDistribution;


// Mute
const int mutePin = 2;
int mutePinState; // 0 or 1.
unsigned long mutePinChangelog; // The timestamp of the last change on this button.
bool mute = false; // The global mute state.


// Trigger OUT
int triggerOutLEDPin = 5;
int triggerOutLedMuted = 5; // The amount of brightness when muted.
int triggerOutLEDBrightness = 0; // The brightness, which varies in different cases.
int triggerOutPin = 6;
const int triggerLength = 25; // In milliseconds.
int triggerOut = LOW; // The state of the trigger out.

// Read the trigger.
boolean getTriggerIn() {
  #ifdef DEBUG
    static int cnt = 0;
  #endif  
  boolean t = LOW;
  if ( !internalClock ) { // the external clock
    t = digitalRead(triggerInPin);
  } else { // the internal clock.
    int cycle = 1000 / ( internalClockBpm / 60 );
    for (int i = 0; i <= triggerLength; i++) { // Run the clock.
      if ( ( millis() - i ) % cycle == 0 ) {
        t = HIGH;
      }
    }
  }
  #ifdef DEBUG
    printf("%d ", t);
    if ((cnt % 40) == 0) { 
      printf("\n");
    }
    cnt++;
  #endif
  return t;
}


// Read the CV inputs for the quantity.
int getQuantity() {
  int basicQuantity = analogRead(quantityPotiPin); // The basic quantity given by the poti
  int cvQuantity = analogRead(quantityCVPin); // The quantity given by the control voltage IN.
  // Map both values summed up.
  int q = map( ( basicQuantity + cvQuantity ), 0, 1024, 1, ( maxQuantity + 1 ) );
  // Return the value with the defined maximum.
  int r;
  if ( q > maxQuantity ) {
    r = maxQuantity;
  } else {
    r = q;
  }
  return r;
}


// Read the CV input (poti) for the distribution.
int getDistribution() {
  int d;
  d = map( analogRead(distributionPotiPin), 0, 1024, 1, ( maxDistribution + 1 ) );
  return d;
}

void setup() {
  //#ifdef DEBUG
    Serial.begin(115200); // Initialize serial communication at 115200 bits per second.
  //#endif
  // Pin settings
  pinMode(triggerInPin, INPUT);
  pinMode(triggerInLEDPin, OUTPUT);
  pinMode(quantityPotiPin, INPUT);
  pinMode(quantityCVPin, INPUT);
  pinMode(distributionPotiPin, INPUT);
  pinMode(mutePin, INPUT);
  pinMode(triggerOutLEDPin, OUTPUT);
  pinMode(triggerOutPin, OUTPUT);
}


/* ##########################################################################
   LOOP
   ##########################################################################
*/

void loop() {
  main_loop();
}

void test_loop() {
    digitalWrite( triggerOutLEDPin, 0);
    digitalWrite( triggerOutPin, 0);
    delay(100);
    digitalWrite( triggerOutLEDPin, 1);
    digitalWrite(triggerOutPin, 1);
    delay(100);
}

void main_loop() {
  // ------------------------ TRIGGER IN ------------------------
  // Get the trigger IN state and the cycle time.
  if ( getTriggerIn() ) {
    // Light up the input LED.
    digitalWrite(triggerInLEDPin, HIGH);

    // Is it the beginning of a new cycle?
    if ( triggerInLow > triggerInHigh ) { // Yes
      // Log the latest cycle time.
      cycleTime = millis() - cycleStart;

      // Log the new cycle start timestamp.
      cycleStart = millis();

      // Debug: Log the settings on cycle start.
      #ifdef DEBUG
        printf("cycleStart, cycleTime: %ld %d\n", cycleStart, cycleTime);
      #endif
    }

    // Log the timestamp of this trigger high.
    triggerInHigh = millis();

  } else {
    // Mute the input LED.
    digitalWrite(triggerInLEDPin, LOW);

    // Log the timestamp of this trigger low.
    triggerInLow = millis();
  }

  // ------------------------ QUANTITY ------------------------
  int q = getQuantity();
  if ( currentQuantity != q ) {
    currentQuantity = q;
    int cvQuantity = analogRead(quantityCVPin);
    digitalWrite(triggerInLEDPin, HIGH); // A little flash to indicate the change.
    #ifdef DEBUG
      printf("currentQuantity: %d\n", currentQuantity);
      printf("cvQuantity: %d\n", cvQuantity);
    #endif
  }


  // ------------------------ DISTRIBUTION ------------------------
  int d = getDistribution();
  if ( currentDistribution != d ) {
    currentDistribution = d;
    digitalWrite(triggerInLEDPin, HIGH); // A little flash to indicate the change.
    #ifdef DEBUG
      printf("currentDistribution: %d\n", currentDistribution);
    #endif
  }


  // ------------------------ MUTE ------------------------
  // Checking and setting the status of mute or unmuted.
  bool p;
  p = digitalRead(mutePin); // Read the pin.
  if ( ( p != mutePinState ) && ( millis() > ( mutePinChangelog + pushButtonDelay ) ) ) { // Did the pin recently change?
    mutePinState = p;
    mutePinChangelog = millis();
    if (p == 1) { // The button is pushed.
      // toggle the global mute state.
      if (mute) {
        mute = false;
      } else {
        mute = true;
      }
      #ifdef DEBUG
        printf("mute: %s\n", mute ? "ON" : "OFF");
      #endif
      digitalWrite(triggerInLEDPin, HIGH); // A little flash to indicate the change.
    }
  }

  // ------------------------ TRIGGER OUT ------------------------
  // Calculate the time slices for the triggers OUT.
  // This happens all the time because the settings can change anytime.
  // We will calculate the exact timestamps when the single hits should follow based on the current settings.
  // And then act on'em, meaning LED and the trigger.

  for (int i = 0; i < currentQuantity; i++ ) {

    // 1. Calculate the decimals from 0 to 1 for our hit quantity.
    double d = (double) i * 1 / currentQuantity;

    // 2. Calculate the resulting factor depending on the set ease function.
    // Based on: https://easings.net/
    double f;

    switch (currentDistribution) {
      case 1:
        f = d * d * d * d * d; // easeInQuint
        break;
      case 2:
        f = d * d * d * d; // easeInQuart
        break;
      case 3:
        f = d * d * d; // easeInCubic
        break;
      case 4:
        f = d * d; // easeInQuad
        break;
      case 5:
        f = 1 - cos((d * PI) / 2); // easeInSine
        break;
      case 6:
        f = d; // Linear
        break;
      case 7:
        f = sin((d * PI) / 2); // easeOutSine
        break;
      case 8:
        f = 1 - (1 - d) * (1 - d); // easeOutQuad
        break;
      case 9:
        f = 1 - pow(1 - d, 3); // easeOutCubic
        break;
      case 10:
        f = 1 - pow(1 - d, 4); // easeOutQuart
        break;
      case 11:
        f = 1 - pow(1 - d, 5); // easeOutQuint
        break;
      default:
        f = d; // Linear
        break;
    }

    // 3. Calculate the resulting absolute timestamp.
    unsigned long t = (long) cycleStart + ( cycleTime * f );

    // 4. Check if we're in the time span to trigger and blink.
    if ( ( millis() >= t ) && ( millis() < ( t + triggerLength ) ) ) {
      if (!mute) {
        triggerOutLEDBrightness = 255;
        triggerOut = HIGH;
      } else {
        triggerOutLEDBrightness = triggerOutLedMuted;
        triggerOut = LOW;
      }
      i = currentQuantity; // If so, stop the right away to keep the trigger HIGH.
    } else {
      triggerOutLEDBrightness = 0;
      triggerOut = LOW;
    }
  }

  // Your time, Outputs!
  analogWrite( triggerOutLEDPin, triggerOutLEDBrightness ); // LED
  #ifdef USE_TRANSISTOR
    digitalWrite( triggerOutPin, !triggerOut ); // Trigger OUT using an inverted version of the signal.
  #else
    digitalWrite( triggerOutPin, triggerOut );  // Trigger OUT.
  #endif
  
}
