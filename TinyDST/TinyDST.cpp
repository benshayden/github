#include <avr/pgmspace.h>

uint16_t dst_days[] PROGMEM = {
  // DST begins on even-indexed elements, ends on odd-indexed elements
  // elements are days since 2000 Jan 1.
  5181, 5419,
  5545, 5783,
  5916, 6154,
  6280, 6518,
  6644, 6882,
  7008, 7246,
  7372, 7610,
  7743, 7981,
  8107, 8345,
  8471, 8709,
  8835, 9073,
  9199, 9437,
  9563, 9801,
  9934, 10172,
  10298, 10536,
  10662, 10900,
};

boolean isDST(uint16_t day) {
  for (uint8_t i = 0; i < ARRAYSIZE(dst_days); ++i) {
    if (day < pgm_read_word(&(dst_days[i]))) {
      return (i % 2) == 1;
    }
  }
  return false;
}
