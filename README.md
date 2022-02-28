Automatic sex-toy inflater deflater
===================================

This hardware and software project is a device that will automatically inflate and deflate an inflatable sex toy, at the control of a single button.


Warning
-------

The current design lack basic safeties like an overpressure valve, Use at your own risk!


User manual
-----------

Power on the device. Keep the button pressed and the toy will start to inflate.

Release the button once a confortable insertion inflation is reached, and insert the toy in your favorite hole.

Short press the button and the toy will deflate, inflate again up to the point you've set previously, pause, deflate again and loop forever on this cycle.

To change the inflation duration, long press the button while the toy is inflating, and release when reaching the desired inflation length. This will change the duration of the cyle.

At any point short-press the button to immediately deflate and restart the cycle.

Double short press the button to deflate and stop.

![Demo](medias/autopump.gif?raw=true "Demo")

Bill of materials
-----------------

- Arduino (Arduino Uno for instance)
- 12V power supply
- 12V->5V voltage regulator
- Relay board with at least two relays
- Small air pump that works at 5V
- Small electronically controlled air valve, 12V, preferably normally open.
- Air tubing and a T connector.
- Jumper wires, male-male and male-female
- A push button
- A longer two-wires cable for the control button.
- (maybe) 1mF+ capacitor

You can find all this on many hobby electronics website, for instance robot-maker.com. Note that the 5V electronic valve from this site won't work, as it leaks under too much pressure.


Wiring
------

- Button goes to GND and PIN 4
- voltage regulator IN goes to Vin and GND
- Pump goes to voltage regulator out through relay 1
- Valve goes to Vin/GND through relay 2
- Relay control 1/2 goes to PINs 2/3.

If the arduino reboots when the solenoid valve triggers, add a 1mF+ capacitor
somewhere near it.

![Wiring](medias/wiring.png?raw=true "Wiring")
