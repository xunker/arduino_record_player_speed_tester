#if defined(__AVR_ATmega328P__)
  #define SENSE_PIN 3
#endif
#if defined(__AVR_ATtinyX313__)
  // attiny4314 INT0 on PD2/Arduino D3. Express in attachInterrupt as 0.
  #define SENSE_PIN 4
#endif

/* For input smoothing, read the sensor this many times consecutively and then
   return the average. Comment it out to used a conventional digitalRead. */
#define TIMES_TO_READ_SENSOR 5
/* Delay between consecutive reads during input smoothing, in milliseconds.
   Comment it out to remove the delay. */
#define DELAY_BETWEEN_READS 1

//#define USE_FLOATING_POINT
//#define ENABLE_SERIAL
#define ENABLE_DISPLAY
#define USE_INTERRUPTS

#ifndef USE_INTERRUPTS
  boolean currentSensePinState = false;
  boolean previousSensePinState = false;
#endif

#define LED_PIN 13

volatile boolean recordPass = false;
unsigned int thisPass = 0;
unsigned int lastPass = 0;
unsigned int elapsed = 0;
#ifdef USE_FLOATING_POINT
  float rpm = 0;
#else
  unsigned int rpm = 0;
#endif

// How long should the built-in LED stay one when a rotation is detected?
#define LED_ON_TIME 500 // milliseconds

// Sanity check; ignore rotations faster than this to filter sensor noise.
#define MINIMUM_ROTATION 500 // milliseconds

#ifdef ENABLE_DISPLAY
  // requires the older "2014" version of library, NOT the most current one on Github.
  #include "SevSeg.h"
  SevSeg sevseg; //Initiate a seven segment controller object
#endif

void setup() {
  
  #ifdef ENABLE_SERIAL
    Serial.begin(9600);
    //while (!Serial) {
    //  ; // wait for serial port to connect. Needed for native USB
    //}
    Serial.println("Serial enabled.");
    delay(1000);
    Serial.println("Ready.");
  #endif
  
  
  pinMode(LED_PIN, OUTPUT);
//  pinMode(SENSE_PIN, INPUT_PULLUP);
  pinMode(SENSE_PIN, INPUT_PULLUP);

  const byte numDigits = 4;
  
  /* 
   12......7
   +-------+ 
   |8.8.8.8|
   +-------+
   1.......6

    11 (ELEVEN)
   1  9
   0  9
    55
   1  4
   1  4
    22  33


    Display pin mapping for Trinket Pro

    Arduino   Display
    Driven pins -- need resistors
    8  6
    6  8
    5  9
    4  12

    undriven -- no resistor needed
    9  11
    10 5
    11 7
    12 1
    A0 2
    A1 3
    A2 4
    A3 10

    
    Display pin mapping for attiny4313

    Arduino   Display
    Driven pins -- need resistors
    6  6
    5  8
    3  9
    2  12

    undriven -- no resistor needed
    7  11
    10 5
    9  7
    15 1
    14 2
    12 3
    11 4
    8  10
   
  */
  #ifdef ENABLE_DISPLAY
  
    #if defined(__AVR_ATmega328P__)
      byte digitPins[] = {4, 5, 6, 8};
      byte segmentPins[] = {9, 11, A2, A0, 12, A3, 10, A1};
    #endif
    #if defined(__AVR_ATtinyX313__)
      byte digitPins[] = {6,5,3,2};
      byte segmentPins[] = {7, 9, 11, 14, 15, 8, 10, 12}; 
    #endif
    
    sevseg.begin(COMMON_CATHODE, numDigits, digitPins, segmentPins);
//  sevseg.setBrightness(90);
  #endif

  #if defined(USE_INTERRUPTS)
    attachSenseInterrupt();
  #endif
}

void loop() {
  #ifdef ENABLE_DISPLAY
    #ifdef USE_FLOATING_POINT
      sevseg.setNumber(rpm, 2);
    #else
      sevseg.setNumber(rpm, 1);
    #endif

    sevseg.refreshDisplay(); // Must run repeatedly to keep display stable
  #endif

  #ifndef USE_INTERRUPTS
    if (previousSensePinState != (currentSensePinState = readSensor())) {
      recordPass = true;
      previousSensePinState = currentSensePinState;
    }
  #endif

  if (recordPass) {
    recordPass = false;
    if (readSensor()) {
      thisPass = millis();
      elapsed = thisPass - lastPass;
      
      #if defined(ENABLE_SERIAL)
        Serial.println(thisPass);
        Serial.println(elapsed);
      #endif

      if (elapsed > MINIMUM_ROTATION) {

        #ifdef USE_FLOATING_POINT
          // 60,000 milliseconds in 1 minute
          rpm = 60000.0 / elapsed;
        #else  
          /* Use integer math to fake floating-point. Multiply number of
           * seconds in a minute by 10 and then place the decimal point 
           * before the "tens" column on the display so we have a reasonable
           * facsimile. Floating-point math uses an extra 2Kb on some tiny
           * MCUs like the ATTiny2313/ATTiny4313.
           */
          rpm = 600000 / elapsed; // integer only
          if (rpm < 100) { rpm = 0; }
        #endif

        #ifdef ENABLE_SERIAL
          Serial.print("RPM: ");
          Serial.println(rpm);
        #endif
          
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        lastPass = thisPass;
      }
    }
    #if defined(USE_INTERRUPTS)
      attachSenseInterrupt();
    #endif
  }
}

#if defined(USE_INTERRUPTS)
  void triggerRecordPass() { //ISR
    #if defined(__AVR_ATmega328P__)
      detachInterrupt(digitalPinToInterrupt(SENSE_PIN));
    #endif
    #if defined(__AVR_ATtinyX313__)
      detachInterrupt(0);
    #endif
    recordPass = true;
  }

  void attachSenseInterrupt() {
    #if defined(__AVR_ATmega328P__)
      attachInterrupt(digitalPinToInterrupt(SENSE_PIN), triggerRecordPass, CHANGE);
    #endif
    #if defined(__AVR_ATtinyX313__)
      attachInterrupt(0, triggerRecordPass, CHANGE);
    #endif
  }
#endif

#ifdef TIMES_TO_READ_SENSOR
  uint8_t smoothDigitalReadTotal = 0;
  boolean readSensor() {
    smoothDigitalReadTotal = 0;
    for(uint8_t i = 0; i < TIMES_TO_READ_SENSOR; i++) {
      smoothDigitalReadTotal += digitalRead(SENSE_PIN);
      #ifdef DELAY_BETWEEN_READS
        if (i<(TIMES_TO_READ_SENSOR-1)) { delay(DELAY_BETWEEN_READS); }
      #endif
    }

    if (smoothDigitalReadTotal >= (TIMES_TO_READ_SENSOR/2)) {
      // digitalWrite(LED_PIN, HIGH);
      return HIGH;
    } else {
      // digitalWrite(LED_PIN, LOW);
      return LOW;
    }
  }
#else
  boolean readSensor() {
    digitalRead(SENSE_PIN);
  }
#endif

