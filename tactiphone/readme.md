The Tactiphone is a tactile, baby-proof synthesizer.

[](TODO photo)

## Parts

 * 12 inch (30cm) long project box
 * [adafruit trinket](http://www.adafruit.com/products/1500) I2C pin 0 = SDA, 2 = SCL
 * [MPR121](https://www.adafruit.com/products/1982)
 * [copper tape](https://www.adafruit.com/products/1127)
 * [dual op-amp](http://www.adafruit.com/products/808)
 * misc resistors, capacitors, hookup wire
 * speaker
 * battery

## Circuit

Load filter.nl in [circuitjs](http://lushprojects.com/circuitjs/circuitjs.html) to see how to use the op-amps to filter the PWM.

## Notes on Programming

[Codebender sketch](https://codebender.cc/sketch:193420)

##### You need to remove the resistor on pin 4 while programming, then put it back in after programming.

[](TODO use transistor?)

The attiny85 has two timers. Timer 0 drives the PWM that outputs to pin 1. Timer 1 drives the PWM that outputs to pin 4. 
Timer 0 also drives delay(), delayMicroseconds(), millis(), and micros(). These timers default to such slow frequencies that they can't drive audio signals. Firmware must change the frequency of a timer in order for it to be fast enough to drive an audio signal. We can't change the frequency of Timer 0 because that would break delay(), millis(), etc. So we have to make Timer 1 drive a fast PWM signal. But Timer 1 outputs to pin 4, which is also used for USB communication while programming. While programming, your circuit cannot have anything connected to pin 4. While running this sketch, your circuit must have the audio filter and speaker connected to pin 4. So that's why you need to remove the resistor on pin 4 while programming, then put it back in after programming.

## References

[attiny85 datasheet](http://www.atmel.com/images/atmel-2586-avr-8-bit-microcontroller-attiny25-attiny45-attiny85_datasheet.pdf)

[mpr121 datasheet](https://www.sparkfun.com/datasheets/Components/MPR121.pdf)

[web tone generator](https://plasticity.szynalski.com/tone-generator.htm)

[web keyboard](http://www.bgfl.org/bgfl/custom/resources_ftp/client_ftp/ks2/music/piano/)

[web tuner](https://jbergknoff.github.io/guitar-tuner/)
