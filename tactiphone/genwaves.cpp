#include "genwaves.h"
#include "notes.h"

// The first quadrant of sin() as duty cycle values:
// a=[];for (var i = 0; i <20; ++i) {a.push(127+Math.round(128 * Math.sin(Math.PI*i/(20*2))))}
uint8_t SINQ1[] = {127, 137, 147, 157, 167, 176, 185, 194, 202, 210, 218, 224, 231, 236, 241, 245, 249, 251, 253, 255};

uint8_t gen_sin(uint32_t period_us)
{
	uint32_t t = micros() % period_us;
	t = (ARRAYSIZE(SINQ1) * t * 4) / period_us;
	if (t < ARRAYSIZE(SINQ1))
	{
		// first quadrant
		return SINQ1[t];
	}
	t -= ARRAYSIZE(SINQ1);
	if (t < ARRAYSIZE(SINQ1))
	{
		// second quadrant
		return SINQ1[ARRAYSIZE(SINQ1) - 1 - t];
	}
	t -= ARRAYSIZE(SINQ1);
	if (t < ARRAYSIZE(SINQ1))
	{
		// third quadrant
		return 255 - SINQ1[t];
	}
	return 255 - SINQ1[ARRAYSIZE(SINQ1) - 1 - t];
}

uint16_t muldiv16(uint16_t a, uint16_t b, uint16_t c)
{
	// return a*b/c if it fits in 16 bits
	uint16_t q = 0;
	uint16_t r = 0;
	uint16_t qn = b / c;
	uint16_t rn = b % c;
	while(a)
	{
		if (a & 1)
		{
			q += qn;
			r += rn;
			if (r >= c)
			{
				q++;
				r -= c;
			}
		}
		a  >>= 1;
		qn <<= 1;
		rn <<= 1;
		if (rn >= c)
		{
			qn++;
			rn -= c;
		}
	}
	return q;
}

uint8_t gen_saw(uint32_t period_us)
{
	uint32_t t = micros() % period_us;
	return muldiv16(t, 255, period_us);
}

uint8_t gen_triangle(uint32_t period_us)
{
	uint32_t t = micros() % period_us;
	period_us /= 2;
	if (t < period_us)
	{
		return muldiv16(t, 255, period_us);
	}
	t -= period_us;
	return 255 - muldiv16(t, 255, period_us);
}

// If you've never played music before, then
// the pentatonic scale is a good place to start,
// like the Arduino of music:
// https://en.wikipedia.org/wiki/Pentatonic_scale
uint32_t chord_p_us[] = {C4p, D4p, E4p, G4p, A4p, C5p, D5p, E5p, G5p, A5p};
uint8_t chord_weights[] = {1, 0, 1, 1, 0, 0, 0, 0, 0, 0};

void gen_chord()
{
	delayMicroseconds(5);
	uint32_t duty = 0;
	uint32_t denominator = 0;
	for (uint8_t i = 0; i < ARRAYSIZE(chord_weights); ++i)
	{
		if (chord_weights[i] == 0) continue;
		denominator += chord_weights[i];
		duty += gen_sin(chord_p_us[i]) * chord_weights[i];
	}
	if (denominator == 0)
	{
		analogWrite4(0);
		return;
	}
	duty /= denominator;
	analogWrite4(duty);
}
