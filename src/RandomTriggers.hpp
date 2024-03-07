#ifndef _RANDOM_TRIGGERS
#define _RANDOM_TRIGGERS

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
  
    RANDOM TRIGGERS (Rev1)
    This is the code for an Eurorack synth module.
    It's built and tested on an ATMega 328p.
    
    Find out more on: https://www.bummbummgarage.com/modules/random-triggers/

    Creative Commons yeah! License: CC BY-NC 4.0
    
    ----------------------------------------------------------------------

    J.S. Bouten 2024-02-01
    - converted code into a C++ class
 */

class RandomTriggers {

  private:

    // General
    const bool internalClock = true; // Refuses the triggers and starts the internal clock. Meant for developing / debugging.
    const int internalClockBpm = 480; // The speed for the internal clock.


    // Trigger IN
    bool triggerIn = false; // Indicator that the current HIGH state has already been detected.
    int triggerInLEDPin;
    int triggerInPin;


    // Pattern
    int patternLength;  // Variable sequence length from 8 to 128
    int patternDensity;
    int densityPotiPin;
    int lengthPotiPin;
    String pattern; // The current pattern stored in zeros (0 = no trigger) and ones (1 = trigger).
    unsigned long calculation; // Timestamp of the latest calculation.
    unsigned int calcIndication = 200; // Time in milliseconds that the LEDs are lit to indicate the recent calculation.


    // Trigger OUT
    int patternPosition = 1; // Starts at 1 and ends at patternLength.
    const int triggerLength = 25; // In milliseconds.
    int triggerOutLEDPin;
    unsigned long triggerOutHigh; // Timestamp of the latest trigger out.
    int triggerOutPin;
    const int TRIGGER_OUT_LED_LOW_BRIGHTNESS = 0;
    const int TRIGGER_OUT_LED_HIGH_BRIGHTNESS = 50;

    void init() {
      // Ensuring non-repeating randomness in random().
      // See https://www.arduino.cc/reference/en/language/functions/random-numbers/randomseed/
      randomSeed(analogRead(0));
    }

    // Read the trigger.
    boolean getTriggerIn(){
      boolean t = LOW;
      if ( !internalClock ) { // the external clock
        t = digitalRead(triggerInPin);
      } else { // the internal clock (when developing or debugging).
        int cycle = 1000 / ( internalClockBpm / 60 );
        for (int i = 0; i <= triggerLength; i++) { // Run the clock.
          if ( ( millis() - i ) % cycle == 0 ) {
            t = HIGH;
          }
        }
      }
      return t;
    }

    // Read the poti and return the trigger pattern length.
    int getLength(){
      // Get the sequence length as a number between 1 and 6.
      int v  = map( analogRead(lengthPotiPin), 0, 1024, 1, 6 );
      int l;
      
      switch (v) {
        case 1:
          l = 4;
          break;
        case 2:
          l = 8;
          break;
        case 3:
          l = 16;
          break;
        case 4:
          l = 32;
          break;
        case 5:
          l = 64;
          break;
        case 6:
          l = 128;
          break;
        default:
          l = 32;          
      }
      return l;
    }

    // Read the poti and return the trigger pattern density [0%...100%].
    int getDensity() {
      // Get the density as a number between 0 and 100 [%].
      int d  = map( analogRead(densityPotiPin), 0, 1024, 0, 100);
      return d;
    }

    // Calculate the pattern based on the current config.
    String calculatePattern( int l , int d ){
      String p; // The pattern to be stored.

      // Iteratively create the pattern.
      for (int i = 0; i < l; i++) {
        // Random binary with propability
        // As seen on: https://forum.arduino.cc/t/increasing-probability-with-a-number/358459/6
        if ( random( 100 ) < d ) {
          // will be executed d % of time
          p = p + "1";
        } else {
          // will be executed in ( 100 – d ) % of the time
          p = p + "0";
        }
      }      
      return p;
    }

    public:

      RandomTriggers() {}

      RandomTriggers( int _triggerInLEDPin, 
                      int _triggerInPin, 
                      int _densitiyPotiPin, 
                      int _lengthPotiPin, 
                      int _triggerOutLEDPin, 
                      int _triggerOutPin):
                      triggerInLEDPin(_triggerInLEDPin),
                      triggerInPin(_triggerInPin), 
                      densityPotiPin(_densitiyPotiPin),
                      lengthPotiPin(_lengthPotiPin),
                      triggerOutLEDPin(_triggerOutLEDPin),
                      triggerOutPin(_triggerOutPin) {
          init();
        }

      void tick() {
        // --------------------- CALCULATE PATTERN --------------------
        int l = getLength();  // The length of the pattern (sequence).
        int d = getDensity(); // The density of the pulses in the pattern.
        bool c = false;       // Indicator to re-calculate.

        // Did the pattern config change?
        // Length
        if ( l != patternLength ) {         
          debug_print2("Pattern length: %d\n", l);

          // Set the new length globally.
          patternLength = l;

          // Ping the calculation.
          c = true;
        }

        // Did the pattern config change?
        // Density
        if ( d != patternDensity ) {          
          debug_print2("Pattern density: %d%\n", d);
          // Set the new density globally.
          patternDensity = d;
          // Ping the calculation.
          c = true;
        }

        // Re-calculate pattern and reset position if needed .
        if ( c == true ) {
          String p = calculatePattern( patternLength , patternDensity );

          // Set the global pattern accordingly.
          pattern = p;

          // Store the timestamp of the calculation.
          calculation = millis();
            
          debug_print2("Pattern: %s\n", p.c_str());

          // Reset global pattern position.
          patternPosition = 1;         
        }

        // Light both LEDs when the pattern has been calculated.
        if ( millis() <= ( calculation + calcIndication ) ) {
          digitalWrite(triggerInLEDPin, HIGH);
          analogWrite(triggerOutLEDPin, TRIGGER_OUT_LED_HIGH_BRIGHTNESS);
          
        }

        // ------------------------ MATCH TRIGGERS ------------------------
        if ( getTriggerIn() ) {
          // Light up the trigger in LED.
          digitalWrite(triggerInLEDPin, HIGH);

          // Is this truely the beginning of this trigger high?
          if ( !triggerIn ) { // Check against the latest stored state.
            // Log the current trigger in high phase.
            triggerIn = true;

            debug_print("Trigger IN\n");

            // Do we have a trigger on the current pattern position?
            if ( pattern.substring( ( patternPosition - 1 ) ).startsWith( "1" ) ) { // YES !
              // Log the trigger out.
              triggerOutHigh = millis();
              debug_print2("Trigger OUT: patternPosition: %d\n", patternPosition);
            }
          
            // Advance pattern position.
            if ( patternPosition < patternLength ) {
              patternPosition++;
            } else {
              patternPosition = 1;
            }            
          }
        } else {
          // Mute the trigger in LED (if there is no calc. to be indicated).
          if ( millis() > ( calculation + ( triggerLength * 4 ) ) ) {
            digitalWrite(triggerInLEDPin, LOW);
          }
          // Log the current trigger in low phase.
          triggerIn = false;
        }

        // ----------------------- SEND TRIGGERS ------------------------

        // Send the trigger out and light the LED as long it's time.
        if ( millis() <= ( triggerOutHigh + triggerLength ) ) {
          analogWrite(triggerOutLEDPin, TRIGGER_OUT_LED_HIGH_BRIGHTNESS);          
          digitalWrite(triggerOutPin, LOW);
        } else {
          // Mute the light (when no calc. is to be indicated).
          if ( millis() > ( calculation + calcIndication ) ) {
            //digitalWrite(triggerOutLEDPin, LOW);
            analogWrite( triggerOutLEDPin, TRIGGER_OUT_LED_LOW_BRIGHTNESS );
            digitalWrite(triggerOutPin, HIGH);
          } 
        }
      }
};
#endif