## Arduino-based record player speed tester using hall effect sensors.

https://github.com/xunker/arduino_record_player_speed_tester

Uses hall effect sensor for detecting turntable speed. Place a magnet on the
turntable near the outside edge and turn it on, then hold the tester in a place
where the magnet will move under the hall sensor. The RPMs of the turntable will
be shown on the LED display.

It uses a 3461AS 4-digit display for RPM output. RPMs can be calculated using
real floating point math or integer-only math. Using true floats gives two
decimal places of precision, where as integer math gives one one decimal place.
Either should be sufficient for showing 33.3 or 16.6 speeds. Using integer
math instead of float will free about 2KB of flash on the ATTiny4313.

Code is written for the U18-524 Latching Hall Effect sensor, but a standard
linear/momentary sensor should also work. If using a latching sensor, two
magnets are needed on the turntable: one to engage and one to disengage the
sensor. NOTE: the U18-524 does not work for me on 3.3v VCC, but does work with
3.5v. If you use that sensor you will need to use a power supply that gives 3.5v
or more. USB 5v, 3xAAA batteries, or 3.7v Li-Ion have all worked for me.

It can use interrupts or polling for tracking rotations, depending on your
preference and memory requirements.

### Wiring

#### 341AS Display

When looking at the front of the display, with decimal points at the bottom, the
pins are numbered as:

```
12......7
+-------+
|8.8.8.8|
+-------+
1.......6
```
#### Trinket Pro (ATMega328p)

```
D13   LED Indicator with ~470ohm resistor inline

D3    Hall Sensor signal output

Arduino Pin   341AS Pin
D8            6           Needs ~470ohm resistor inline
D6            6           Needs ~470ohm resistor inline
D5            9           Needs ~470ohm resistor inline
D4            12          Needs ~470ohm resistor inline
D9            11
D10           5
D11           7
D12           1
A0            2
A1            3
A2            4
A3            10
```

#### ATTiny4313

```
D13   LED Indicator with ~470ohm resistor inline

D4    Hall Sensor signal output

Arduino Pin   341AS Pin
D6            6           Needs ~470ohm resistor inline
D5            8           Needs ~470ohm resistor inline
D3            9          (alt: D1 if using external crystal) Needs ~470ohm resistor inline
D2            12         (alt: D0 if using external crystal) Needs ~470ohm resistor inline
D7            11
D10           5
D9            7
D15           1
D14           2
D12           3
D11           4
D8            10
```

### Note about speed and clock accuracy

Officially, the speed of the internal oscillator in an AVR can be off by up to
10%, depending on voltage and temperature. In personal experience, I have never
seen one that off by more than 2%.

To address this, most professionally-produced Arduino (and compatible) devices
integrate an external "crystal" that will make sure the clock is 99.9% accurate.

If you are building this device from scratch and want *complete* accuracy you
will want to either include a quartz crystal or ceramic resonator in your
design. If you use an external crystal or ceramic resonator, you will need to
uncomment the `#define USE_EXTERNAL_CRYSTAL` line in the source.

If you don't want to go to that trouble, or don't have the available
pins, you can tune the internal oscillator for more accuracy. A web search for
a tool called "TinyTuner" will tell you how to do it.

Using an uncalibrated ATTiny4313, I found the reading to be within 1%
of the same ATTiny4313 using a 16mHz external crystal. Your mileage may vary.

### Hardware known to work

* Adafruit Pro Trinket (328P-based)
* ATTiny4313 (2313 will *not* work, there is not enough memory.
* Arduino Micro (32U4-based, although the current source code does not include the correct settings for the 32U4 yet)

#### Changes

v1.0 Oct 19, 2016
  * Initial

#### License

Distributed under GNU GPLv3

#### Author, etc

Created by Matthew Nielsen, (c) 2016
