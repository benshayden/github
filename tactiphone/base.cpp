#include "base.h"

void PWM4_init()
{
	// Set up PWM on Trinket GPIO #4 (PB4, pin 3) using Timer 1
	// micros() and friends use Timer 0, so changing it would mess them up.
	TCCR1 = _BV(CS10); // no prescaler
	GTCCR = _BV(COM1B1) | _BV(PWM1B); //  clear OC1B on compare
	OCR1B = 127; // duty cycle initialize to 50%
	OCR1C = 255; // frequency
}

void analogWrite4(uint8_t duty_value)
{
	OCR1B = duty_value;  // duty may be 0 to 255 (0 to 100%)
}
