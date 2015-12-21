The Tactiphone is a tactile, baby-proof synthesizer.

[](TODO photo)

It is entirely contained in a plastic project box, so there are no dangerous wires or sharp edges, and it resists spills. With a wireless charger, you could seal it so that it is entirely water-proof and hard to accidentally open.

It uses capacitive sensing to detect when notes are touched. It can be tuned to be sensitive to smaller fingers. It's quiet when touched with a fingertip and loud when touched with more of the finger. You can paint or glue tactile materials to the outside of the box without interfering with the sensors.

## Parts

 * 12 inch (30cm) long project box
 * [adafruit trinket](http://www.adafruit.com/products/1500) I2C pin 0 = SDA, 2 = SCL
 * [MPR121](https://www.adafruit.com/products/1982)
 * [copper tape](https://www.adafruit.com/products/1127)
 * [dual op-amp](http://www.adafruit.com/products/808)
 * misc resistors, capacitors, hookup wire
 * speaker
 * battery
 * optional wireless charger [transmitter](http://www.adafruit.com/products/2162) and [receiver](http://www.adafruit.com/products/1901)

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

[analog hbridge](http://lushprojects.com/circuitjs/circuitjs.html?cct=$+13+0.000005+10.200277308269968+50+5+50%0AR+256+240+192+240+0+1+40+5+0+0+0.5%0Aa+320+224+416+224+0+5+0+1000000%0Aa+320+352+416+352+0+5+0+1000000%0Aw+256+240+320+240+0%0Ag+320+368+288+368+0%0Aw+320+208+320+176+0%0Aw+320+176+416+176+0%0Aw+416+176+416+224+0%0Aw+320+336+320+304+0%0Aw+416+304+416+352+0%0Ar+320+304+416+304+0+10000%0Ar+320+336+256+336+0+10000%0Ad+256+336+256+240+1+0.805904783%0Ar+464+272+560+272+0+100%0At+576+224+624+224+0+1+-0.002724272683175417+-0.0000912116059143444+100%0At+560+336+608+336+0+1+0.35932476151360815+0.3619561164563783+100%0Ag+608+352+608+384+0%0Ag+624+240+624+256+0%0Aw+416+224+576+224+0%0Aw+560+272+624+208+0%0Aw+464+272+608+320+0%0Ad+416+224+464+272+1+0.805904783%0Aw+416+304+560+336+0%0Ad+416+304+560+272+1+0.805904783%0Ao+0+64+0+551+10+0.003125+0+-1%0Ao+13+64+0+551+5+0.1+1+-1%0A)
