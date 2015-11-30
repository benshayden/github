#include "notes.h"
typedef struct {
	uint8_t* pattern;
	uint8_t pattern_length;
	note_t* song;
	uint8_t song_length;
} recognizer_t;

void ring_touch(uint8_t i);
void recognize();
