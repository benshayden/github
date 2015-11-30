#include "lullaby.h"
#include "genwaves.h"

uint8_t pattern0[] = {};
note_t song0[] = {};
recognizer_t recognizers[] = {
	lullaby_recognizer,
};

uint8_t touch_ring[] = {0, 0, 0, 0, 0, 0, 0, 0};
int8_t touch_ring_i = 0;

void ring_touch(uint8_t i)
{
	touch_ring[touch_ring_i] = i;
	touch_ring_i = (touch_ring_i + 1) % ARRAYSIZE(touch_ring);
}

uint8_t get_touch_ring(uint8_t i)
{
	return touch_ring[(touch_ring_i - i) % ARRAYSIZE(touch_ring)];
}

boolean recognize_pattern(uint8_t* pattern, uint8_t length)
{
	for (uint8_t i = 0; i < length; ++i)
	{
		if (get_touch_ring(i) != pattern[i]) return false;
	}
	return true;
}

void play_note(uint32_t period_us, uint32_t ms)
{
	uint32_t now_ms = millis();
	if (MAX_UINT32 - ms > now_ms) {
		uint32_t flip_ms = MAX_UINT32 - now_ms;
		play_note(period_us, flip_ms);
		play_note(period_us, ms - flip_ms);
		return;
	}
	uint32_t end_ms = now_ms + ms;
	while (millis() < end_ms)
	{
		analogWrite4(gen_sin(period_us));
	}
}

void recognize()
{
	for (uint8_t i = 0; i < ARRAYSIZE(recognizers); ++i)
	{
		if (recognize_pattern(recognizers[i].pattern, recognizers[i].pattern_length))
		{
			for (uint8_t n = 0; n < recognizers[i].song_length; ++n) {
				play_note(recognizers[i].song[i].period_us, recognizers[i].song[i].ms);
			}
			return;
		}	
	}
}
