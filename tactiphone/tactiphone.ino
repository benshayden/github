#include <avr/power.h>
#include <TinyWireM.h>

// https://github.com/adafruit/Adafruit_MPR121_Library
#include "tinympr121.h"
// https://www.sparkfun.com/datasheets/Components/MPR121.pdf

#include "base.h"
#include "recognize.h"
#include "genwaves.h"

#define TOUCH_POLL_MS 50

Adafruit_MPR121 mpr121;
uint16_t prev_touches = 0;
uint32_t prev_touch_t = 0;

void setup()
{
	if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
	pinMode(4, OUTPUT);
	PWM4_init();
	mpr121.begin();
}

void loop()
{
	gen_chord();

	if (millis() > prev_touch_t && millis() < (prev_touch_t + TOUCH_POLL_MS))
		return;

	uint16_t touches = mpr121.touched();
	for (uint8_t i = 0; i < 12; i++)
	{
		if (!BIT_TEST(touches, i))
		{
			chord_weights[i] = 0;
			continue;
		}

		chord_weights[i] = mpr121.filteredData(i) - mpr121.baselineData(i);

		if (!BIT_TEST(prev_touches, i))
		{
			ring_touch(i);
			recognize();
		}
	}
	prev_touches = touches;
}
