#include "base.h"

extern uint32_t chord_p_us[];
extern uint8_t chord_weights[];

uint8_t gen_sin(uint32_t period_us);
uint8_t gen_saw(uint32_t period_us);
uint8_t gen_triangle(uint32_t period_us);
void gen_chord();
