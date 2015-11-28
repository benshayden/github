#include <avr/power.h>
#include <TinyWireM.h>

// This uses PWM on pin 4.
// You have to low-pass filter PWM.
// So you're going to have a resistor from pin 4 to your speaker
// (or a voltage follower buffer) and then a capacitor from that to ground.
// This interferes with USB communication, which also uses pin 4.
// So you need to remove the low-pass filter resistor from pin 4
// while programming the Trinket, and
// add it back as soon as the programming finishes.

// Use TinyWireM to use I2C to communicate with the
// MPR121 capacitive touch sensor.
// Connect the Trinket's pin 0 to the MPR121's SDA pin,
// and pin 2 to the SCL pin.
// So we can't use pins 0 or 2 for PWM.
// We can't use pin 1 for PWM, either, because, in order for the
// PWM frequency to be fast enough for audio frequencies,
// we would need to reconfigure Timer/Counter 0, which would break
// millis(), micros(), delay(), and delayMicroseconds().
// So we need to configure Timer/Counter 1 to be fast enough for audio PWM,
// and that timer only outputs on pin 4.

// Use this tool to calculate the resistor and capacitor values:
// http://sim.okawa-denshi.jp/en/CRlowkeisan.htm
// And don't forget that AVR GPIO pins can only supply about 20mA,
// so the resistor must be at least 5V / 20mA = 250 Ohm.

// If you want to experiment with more advanced signal processing techniques,
// here's a circuit simulator:
// http://lushprojects.com/circuitjs/circuitjs.html

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
// c='';for (var o=3;o<6;++o){for (var s=0;s<12;++s){c+='#define ' + names[s] + o+'_P_US ' + Math.round(1e6/(110*Math.pow(a,((o-3)*12)+s+3)))+'\n'}}
#define C3_P_US 7645
#define CS3_P_US 7215
#define D3_P_US 6810
#define DS3_P_US 6428
#define E3_P_US 6067
#define F3_P_US 5727
#define FS3_P_US 5405
#define G3_P_US 5102
#define GS3_P_US 4816
#define A3_P_US 4545
#define AS3_P_US 4290
#define B3_P_US 4050
#define C4_P_US 3822
#define CS4_P_US 3608
#define D4_P_US 3405
#define DS4_P_US 3214
#define E4_P_US 3034
#define F4_P_US 2863
#define FS4_P_US 2703
#define G4_P_US 2551
#define GS4_P_US 2408
#define A4_P_US 2273
#define AS4_P_US 2145
#define B4_P_US 2025
#define C5_P_US 1911
#define CS5_P_US 1804
#define D5_P_US 1703
#define DS5_P_US 1607
#define E5_P_US 1517
#define F5_P_US 1432
#define FS5_P_US 1351
#define G5_P_US 1276
#define GS5_P_US 1204
#define A5_P_US 1136
#define AS5_P_US 1073
#define B5_P_US 1012

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

uint16_t muldiv16(uint16_t a, uint16_t b, uint16_t c) {
  // return a*b/c if it fits in 16 bits
  uint16_t q = 0;
  uint16_t r = 0;
  uint16_t qn = b / c;
  uint16_t rn = b % c;
  while(a) {
    if (a & 1) {
      q += qn;
      r += rn;
      if (r >= c) {
        q++;
        r -= c;
      }
    }
    a  >>= 1;
    qn <<= 1;
    rn <<= 1;
    if (rn >= c) {
      qn++; 
      rn -= c;
    }
  }
  return q;
}

uint8_t mytri(unsigned long period_us) {
	unsigned long t = micros() % period_us;
	period_us /= 2;
	if (t < period_us) {
		return muldiv16(t, 255, period_us);
	}
	t -= period_us;
	return 255 - muldiv16(t, 255, period_us);
}

uint8_t mysq(unsigned long period_us) {
	unsigned long t = micros() % period_us;
}

void chord(unsigned long p0us, uint8_t w0,
	       unsigned long p1us, uint8_t w1,
	       unsigned long p2us, uint8_t w2,
	       unsigned long p3us, uint8_t w3,
	       unsigned long p4us, uint8_t w4,
	       unsigned long p5us, uint8_t w5,
	       unsigned long p6us, uint8_t w6,
	       unsigned long p7us, uint8_t w7,
	       unsigned long p8us, uint8_t w8,
	       unsigned long p9us, uint8_t w9) {
	delayMicroseconds(5);
	unsigned long duty = 0;
	unsigned long denominator = w0 + w1 + w2 + w3 + w4 + w5 + w6 + w7 + w8 + w9;
	if (denominator == 0) {
		analogWrite4(0);
		return;
	}
	if (w0 > 0) duty += mysin(p0us) * w0;
	if (w1 > 0) duty += mysin(p1us) * w1;
	if (w2 > 0) duty += mysin(p2us) * w2;
	if (w3 > 0) duty += mysin(p3us) * w3;
	if (w4 > 0) duty += mysin(p4us) * w4;
	if (w5 > 0) duty += mysin(p5us) * w5;
	if (w6 > 0) duty += mysin(p6us) * w6;
	if (w7 > 0) duty += mysin(p7us) * w7;
	if (w8 > 0) duty += mysin(p8us) * w8;
	if (w9 > 0) duty += mysin(p9us) * w9;
	duty /= denominator;
	analogWrite4(duty);
}

void loop() {
	chord(C4_P_US, 1,
	      D4_P_US, 0,
	      E4_P_US, 0,
	      G4_P_US, 0,
	      A4_P_US, 0,
	      C5_P_US, 1,
	      D5_P_US, 0,
	      E5_P_US, 0,
	      G5_P_US, 0,
	      A5_P_US, 0);
}
