/*
Arduino record player speed tester using magnetic hall effect sensors

Matthew Nielsen, (C) 2016
https://github.com/xunker/arduino_record_player_speed_tester

Last updated Oct 20, 2016.

*/

/* Do you want to use real floating-point math or fake it with integer-only
   math? On the ATTiny4313, real floating-point uses an extra 2KB of flash.
   Using true floats give two decimal places of precision, where as integer
   math gives one one decimal place. Either should be sufficient for showing
   33.3 or 16.6 speeds. */
//#define USE_FLOATING_POINT

/* Enable RPM output on Serial? Note: ATTiny4313 can't use an external crystal
   and have serial output enabled at the same time. */
//#define ENABLE_SERIAL

/* Use pin interrupts to detect RPMs? By default, you will want this unless you
   are changing pin assignments. If commented out, the pin will be polled
   continuously to look for changes. This would be good option if you don't have
   a free pin that supports external interrupts. */
#define USE_INTERRUPTS

/* You can disable the numeric display complete by commenting this out. Why
   would you want to? Maybe you want serial output only, or you are testing
   with a different display or device. Disabling the display will also stop
   the SevSeg library from loading which can lower memory requirements. */
#define ENABLE_DISPLAY

/* The led indicator. If this is a platform that defines LED_BUILTIN, we use
   that. Otherwise, assume pin 13. */
#ifdef LED_BUILTIN
  #define LED_PIN LED_BUILTIN
#else
  #define LED_PIN 13
#endif

/* For input smoothing, read the sensor this many times consecutively and then
   return the average. Comment it out to used a conventional digitalRead. */
#define TIMES_TO_READ_SENSOR 5
/* Delay between consecutive reads during input smoothing, in milliseconds.
   Comment it out to remove the delay. */
// #define DELAY_BETWEEN_READS 1 // milliseconds

/* Average the number of RPM reads over this many times. The larger the number
   the more accurate a smoother the reported number will be. However, making
   this number larger also means it will take that many rotations to get an
   accurate averate and so start-up time will be slower. Also, the larger the
   number, the more mempry (RAM) will be used.

   If commented out, the current instantaneous RPM will be returned without
   being averaged over time. */
#define RPM_AVERAGE 5

/* ATTiny4313 only: Are you using an external crystal? In that case, we need
   free-up pins 4/5 (D2/D3). We relocate them to 2/3 (D0/D1), but that also
   means we can't use Serial at the same time. */
#define USE_EXTERNAL_CRYSTAL

/* SENSE_PIN is where the signal output of the hall effect sensor is read.
   Ideally, this should be a pin that supports external interrupts*/
#if defined(__AVR_ATmega328P__)
  #define SENSE_PIN 3
#endif
#if defined(__AVR_ATtinyX313__)
  // attiny4314 INT0. Express in attachInterrupt as 0.
  #define SENSE_PIN 4
#endif

/* MAXIMUM_ROTATION is the longest amount of time one revolution of the turntable should take. A value of 2 seconds (2000 milliseconds) is a good number for 33 1/3 RPM or faster
turntables.

If you are testing a very old phonograph that plays 16RPM or 8RPM records, you should adjust this number. You can calculate the maximum time one revolution should take by dividing 60 (number of second in a minute) by the RPM speed of the turntable. For a 16 2/3 RPM turntable that would be:
  60 / 16.66 == 3.6s

*/
#define MAXIMUM_ROTATION 2000 // milliseconds

/* MINIMUM_ROTATION is the minimum amount of time a single
rotation should take. This is a "sanity check" to help us ignore
noise or erronious signals on SENSE_PIN.
This should be less than the amount of time one revolution of the
turntable should take at the fastest speed you plan on testing. A value of 500 millisecond (0.5 second) is a good default and will work for turntables up to 78RPM. */
#define MINIMUM_ROTATION 500 // milliseconds

/* if CONTINUOUS_UPDATES is enabled then the average will be calculated
   regularly, every MAXIMUM_ROTATION milliseconds. If it is not enabled, it will
   only be calculated when SENSE_PIN is triggered. */
#define CONTINUOUS_UPDATES

/* --- configuration ends here -- */

#ifdef CONTINUOUS_UPDATES
  unsigned long nextUpdate = 0;
#endif

#ifdef USE_EXTERNAL_CRYSTAL
  #undef ENABLE_SERIAL
#endif

#ifndef USE_INTERRUPTS
  boolean currentSensePinState = false;
  boolean previousSensePinState = false;
#endif

volatile boolean recordPass = false;
unsigned int thisPass = 0;
unsigned int lastPass = 0;
unsigned int elapsed = 0;
#ifdef USE_FLOATING_POINT
  float rpm = 0;
#else
  unsigned int rpm = 0;
#endif

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
    3  9  (alt: D1 if using external crystal)
    2  12 (alt: D0 if using external crystal)

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
      #ifdef USE_EXTERNAL_CRYSTAL
        byte digitPins[] = {6,5,1,0};
      #else
        byte digitPins[] = {6,5,3,2};
      #endif
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
    #ifdef CONTINUOUS_UPDATES
      nextUpdate = millis() + MAXIMUM_ROTATION;
    #endif

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
          #ifdef RPM_AVERAGE
            rpm = 60000.0 / elapsed;
          #else
            rpm = getAverageRPM(60000.0 / elapsed);
          #endif
        #else
          /* Use integer math to fake floating-point. Multiply number of
           * seconds in a minute by 10 and then place the decimal point
           * before the "tens" column on the display so we have a reasonable
           * facsimile. Floating-point math uses an extra 2Kb on some tiny
           * MCUs like the ATTiny2313/ATTiny4313.
           */
          #ifdef RPM_AVERAGE
            rpm =  getAverageRPM(600000 / elapsed); // integer only
          #else
            rpm = 600000 / elapsed; // integer only
          #endif
          if (rpm < 100) { rpm = 0; }
        #endif

        #ifdef ENABLE_SERIAL
          Serial.print("RPM: ");
          Serial.println(rpm);
        #endif

        toggleLED();
        lastPass = thisPass;
      }
    }
    #if defined(USE_INTERRUPTS)
      attachSenseInterrupt();
    #endif
  } else {
    #ifdef RPM_AVERAGE
      #ifdef CONTINUOUS_UPDATES
        unsigned long currentMillis = millis();
        if (nextUpdate < currentMillis) {
          toggleLED();

          #ifdef USE_FLOATING_POINT
            rpm = getAverageRPM(0.0);
          #else
            rpm = getAverageRPM(0);
          #endif

          nextUpdate = currentMillis + MAXIMUM_ROTATION;
        }
      #endif
    #endif
  }
}

void toggleLED() {
  digitalWrite(LED_PIN, !digitalRead(LED_PIN));
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
      return HIGH;
    } else {
      return LOW;
    }
  }
#else
  boolean readSensor() {
    digitalRead(SENSE_PIN);
  }
#endif

#ifdef RPM_AVERAGE
  uint8_t averageRPMCounter = 0;

  #ifdef USE_FLOATING_POINT
//    float averageRPMCounter = 0.0;
    float averageRPMHistory[RPM_AVERAGE];
    float getAverageRPM(float currentRPM) {
      averageRPMHistory[averageRPMCounter] = currentRPM;
      averageRPMCounter++;
      if (averageRPMCounter > RPM_AVERAGE) {
        averageRPMCounter = 0;
      }

      float averageRPMTotal = 0;
      for (uint8_t i = 0; i < RPM_AVERAGE;i++) {
        averageRPMTotal += averageRPMHistory[i];
      }
      return (averageRPMTotal/RPM_AVERAGE);
    }
  #else
//    unsigned int averageRPMCounter = 0;
    unsigned int averageRPMHistory[RPM_AVERAGE];
    unsigned int getAverageRPM(unsigned int currentRPM) {
      averageRPMHistory[averageRPMCounter] = currentRPM;
      averageRPMCounter++;
      if (averageRPMCounter > RPM_AVERAGE) {
        averageRPMCounter = 0;
      }

      unsigned int averageRPMTotal = 0;
      for (uint8_t i = 0; i < RPM_AVERAGE;i++) {
        averageRPMTotal += averageRPMHistory[i];
      }
      return (averageRPMTotal/RPM_AVERAGE);
    }
  #endif
#endif
