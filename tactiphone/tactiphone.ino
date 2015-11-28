#include <avr/power.h>
#include <TinyWireM.h>

// This uses PWM on pin 4.
// You have to low-pass filter PWM.
// So you're going to have a resistor from pin 4 to your speaker
// (or a voltage follower buffer) and then a capacitor from that to ground.
// This interferes with USB communication.
// So you need to remove the low-pass filter resistor from pin 4
// while programming the Trinket, and
// add it back as soon as the programming finishes.

// Use this tool to calculate the resistor and capacitor values:
// http://sim.okawa-denshi.jp/en/CRlowkeisan.htm
// And don't forget that AVR GPIO pins can only supply about 20mA,
// so the resistor must be at least 5V / 20mA = 250 Ohm.

// The attiny85 datasheet is always handy:
// http://www.atmel.com/images/atmel-2586-avr-8-bit-microcontroller-attiny25-attiny45-attiny85_datasheet.pdf

// Here's an online tone generator for comparison:
// https://plasticity.szynalski.com/tone-generator.htm

// Here's a tuner for testing:
// https://jbergknoff.github.io/guitar-tuner/

// If you've never played music before, then
// the pentatonic scale is a good place to start,
// like the Arduino of music:
// https://en.wikipedia.org/wiki/Pentatonic_scale

void PWM4_init() {
  // Set up PWM on Trinket GPIO #4 (PB4, pin 3) using Timer 1
  // micros() and friends use Timer 0, so changing it would mess them up.
  TCCR1 = _BV(CS10); // no prescaler
  GTCCR = _BV(COM1B1) | _BV(PWM1B); //  clear OC1B on compare
  OCR1B = 127; // duty cycle initialize to 50%
  OCR1C = 255; // frequency
}

void analogWrite4(uint8_t duty_value) {  
  OCR1B = duty_value;  // duty may be 0 to 255 (0 to 100%)
}

void setup() {
	if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
	pinMode(4, OUTPUT);
	PWM4_init();
    TinyWireM.begin();
}

// The first quadrant of sin() as duty cycle values:
// a=[];for (var i = 0; i <20; ++i) {a.push(127+Math.round(128 * Math.sin(Math.PI*i/(20*2))))}
uint8_t SINQ1[] = {127, 137, 147, 157, 167, 176, 185, 194, 202, 210, 218, 224, 231, 236, 241, 245, 249, 251, 253, 255};

// Periods of musical tones in microseconds:
#define C4_P_US 3822
#define D4_P_US 3405
#define E4_P_US 3034
#define G4_P_US 2551
#define A4_P_US 2273
#define C5_P_US 1191
#define D5_P_US 1702
#define E5_P_US 1517
#define G5_P_US 1275
#define A5_P_US 1136
// http://www.phy.mtu.edu/~suits/notefreqs.html

uint8_t mysin(unsigned long period_us) {
	unsigned long t = micros() % period_us;
	t = (sizeof(SINQ1) * t * 4) / period_us;
	if (t < sizeof(SINQ1)) {
		// first quadrant
		return SINQ1[t];
	}
	t -= sizeof(SINQ1);
	if (t < sizeof(SINQ1)) {
		// second quadrant
		return SINQ1[sizeof(SINQ1) - 1 - t];
	}
	t -= sizeof(SINQ1);
	if (t < sizeof(SINQ1)) {
		// third quadrant
		return 255 - SINQ1[t];
	}
	return 255 - SINQ1[sizeof(SINQ1) - 1 - t];
}

void chord(unsigned long p0us, uint8_t w0,
	       unsigned long p1us, uint8_t w1,
	       unsigned long p2us, uint8_t w2) {
	unsigned long duty = 0;
	duty += mysin(p0us) * w0;
	duty += mysin(p1us) * w1;
	duty += mysin(p2us) * w2;
	duty /= (w0 + w1 + w2);
	analogWrite4(duty);
	delayMicroseconds(5);
}

void loop() {
	chord(C4_P_US, 1,
	      E4_P_US, 1,
	      G4_P_US, 1);
}
