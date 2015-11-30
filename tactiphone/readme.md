[](TODO describe tactiphone)

[adafruit trinket](http://www.adafruit.com/products/1500) I2C pin 0 = SDA, 2 = SCL

[MPR121](https://www.adafruit.com/products/1982)

[copper tape](https://www.adafruit.com/products/1127)

2 op amps

misc resistors, capacitors

speaker

battery

Load filter.nl in [circuitjs](http://lushprojects.com/circuitjs/circuitjs.html) to see how to use the op-amps to filter the PWM.


This uses PWM on pin 4.
You have to low-pass filter PWM.
So you're going to have a resistor from pin 4 to your speaker
(or a voltage follower buffer) and then a capacitor from that to ground.
This interferes with USB communication, which also uses pin 4.
So you need to remove the low-pass filter resistor from pin 4
while programming the Trinket, and
add it back as soon as the programming finishes.

Use TinyWireM to use I2C to communicate with the
MPR121 capacitive touch sensor.
Connect the Trinket's pin 0 to the MPR121's SDA pin,
and pin 2 to the SCL pin.
So we can't use pins 0 or 2 for PWM.
We can't use pin 1 for PWM, either, because, in order for the
PWM frequency to be fast enough for audio frequencies,
we would need to reconfigure Timer/Counter 0, which would break
millis(), micros(), delay(), and delayMicroseconds().
So we need to configure Timer/Counter 1 to be fast enough for audio PWM,
and that timer only outputs on pin 4.

[attiny85 datasheet](http://www.atmel.com/images/atmel-2586-avr-8-bit-microcontroller-attiny25-attiny45-attiny85_datasheet.pdf)

[web tone generator](https://plasticity.szynalski.com/tone-generator.htm)

[web keyboard](http://www.bgfl.org/bgfl/custom/resources_ftp/client_ftp/ks2/music/piano/)

[web tuner](https://jbergknoff.github.io/guitar-tuner/)
