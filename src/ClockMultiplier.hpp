#ifndef _CLOCK_MULTIPLIER
#define _CLOCK_MULTIPLIER

//#define DEBUG

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
 * 
 * J.S. Bouten 2024-02-01
 *  - converted code into a C++ class
 */

class ClockMultiplier {

  private:
    // General
    const bool internalClock = false; // Refuses the triggers and starts the internal clock. Meant for developing / debugging.
    const int internalClockBpm = 70; // The speed for the internal clock.
    const int pushButtonDelay = 50; // The time the button will be insensitive after last change.


    // Trigger IN
    int triggerInPin;
    int triggerInLEDPin;

    long triggerInHigh; // Timestamp of the latest trigger high.
    long triggerInLow; // Timestamp of the latest trigger low.

    // Cycles
    long cycleStart = 0; // Timestamp of when the last cycle began.
    int cycleTime = 0; // The absolute time span one cycle has in the given settings.


    // Quantity
    int quantityPotiPin;
    int quantityCVPin;
    const int maxQuantity = 8; // The maximum amount of possible triggers out per cycle.
    int currentQuantity; // The current amount of triggers per cycle.


    // Distribution
    int distributionPotiPin;
    const int maxDistribution = 11; // The amount of different distribution patterns.
    int currentDistribution;


    // Mute
    int mutePinState; // 0 or 1.
    unsigned long mutePinChangelog; // The timestamp of the last change on this button.

    // Trigger OUT
    int triggerOutLEDPin;
    const int TRIGGER_OUT_LED_MUTED_BRIGHTNESS = 1;      // The amount of brightness when muted.
    const int TRIGGER_OUT_LED_NOT_MUTED_BRIGHTNESS = 50; // The amount of brightness when not muted.
    const int TRIGGER_IN_LED_HIGH_BRIGHTNESS = 200;       // This led is red and needs some more umph.
    int triggerOutLEDBrightness = 0; // The brightness, which varies in different cases.
    int triggerOutPin;
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
      debug_print2("%d ", t);
      #ifdef DEBUG
        if ((cnt % 40) == 0) { 
          debug_printf("\n");
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

  public:

    ClockMultiplier() {}

    ClockMultiplier(int _triggerInPin, 
                    int _triggerInLEDPin, 
                    int _quantityPotiPin, 
                    int _quantityCVPin, 
                    int _distributionPotiPin, 
                    int _triggerOutLEDPin, 
                    int _triggerOutPin):
                    triggerInPin(_triggerInPin), 
                    triggerInLEDPin(_triggerInLEDPin),
                    quantityPotiPin(_quantityPotiPin),
                    quantityCVPin(_quantityCVPin),
                    distributionPotiPin(_distributionPotiPin),
                    triggerOutLEDPin(_triggerOutLEDPin),
                    triggerOutPin(_triggerOutPin) {}

    void tick(bool inMutedState) {
      // ------------------------ TRIGGER IN ------------------------
      // Get the trigger IN state and the cycle time.
      if ( getTriggerIn() ) {
        // Light up the input LED.
        analogWrite(triggerInLEDPin, TRIGGER_IN_LED_HIGH_BRIGHTNESS);

        // Is it the beginning of a new cycle?
        if ( triggerInLow > triggerInHigh ) { // Yes
          // Log the latest cycle time.
          cycleTime = millis() - cycleStart;

          // Log the new cycle start timestamp.
          cycleStart = millis();

          // Debug: Log the settings on cycle start.
          debug_print3("cycleStart, cycleTime: %ld %d\n", cycleStart, cycleTime);
        }
        // Log the timestamp of this trigger high.
        triggerInHigh = millis();
      } else {
        // Mute the input LED.
        analogWrite(triggerInLEDPin, 0);
        // Log the timestamp of this trigger low.
        triggerInLow = millis();
      }

      // ------------------------ QUANTITY ------------------------
      int q = getQuantity();
      if ( currentQuantity != q ) {
        currentQuantity = q;
        analogWrite(triggerInLEDPin, TRIGGER_IN_LED_HIGH_BRIGHTNESS); // A little flash to indicate the change.
        debug_print2("currentQuantity: %d\n", currentQuantity);
        debug_print2("cvQuantity: %d\n", cvQuantity);
      }

      // ------------------------ DISTRIBUTION ------------------------
      int d = getDistribution();
      if ( currentDistribution != d ) {
        currentDistribution = d;
        analogWrite(triggerInLEDPin, TRIGGER_IN_LED_HIGH_BRIGHTNESS); // A little flash to indicate the change.
        debug_print2("currentDistribution: %d\n", currentDistribution);
      }

      // ------------------------ MUTE ------------------------
      // Checking and setting the status of mute or unmuted.
      if ( ( inMutedState != mutePinState ) && ( millis() > ( mutePinChangelog + pushButtonDelay ) ) ) { // Did the pin recently change?
        mutePinState = inMutedState;
        mutePinChangelog = millis();
        #ifdef DEBUG
          printf("inMutedState: %s\n", inMutedState ? "ON" : "OFF");
        #endif
        analogWrite(triggerInLEDPin, TRIGGER_IN_LED_HIGH_BRIGHTNESS); // A little flash to indicate the change.
      }

      // ------------------------ TRIGGER OUT ------------------------
      // Calculate the time slices for the triggers OUT.
      // This happens all the time because the settings can change anytime.
      // We will calculate the exact timestamps when the single hits should follow based on the current settings.
      // And then act on'em, meaning LED and trigger output.
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
        unsigned long timestamp = (long) cycleStart + ( cycleTime * f );

        // 4. Check if we're in the time span to trigger and blink.
        if ( ( millis() >= timestamp ) && ( millis() < ( timestamp + triggerLength ) ) ) {
          if (!inMutedState) {
            triggerOutLEDBrightness = TRIGGER_OUT_LED_NOT_MUTED_BRIGHTNESS;
            triggerOut = HIGH;
          } else {
            triggerOutLEDBrightness = TRIGGER_OUT_LED_MUTED_BRIGHTNESS;
            triggerOut = LOW;
          }
          i = currentQuantity; // If so, stop right away to keep the trigger HIGH.
        } else {
          triggerOutLEDBrightness = 0;
          triggerOut = LOW;
        }
      }

      // Your time, Outputs!
      analogWrite( triggerOutLEDPin, triggerOutLEDBrightness );
      digitalWrite( triggerOutPin, !triggerOut ); // Trigger OUT using an inverted version of the signal.     
  }
};
#endif