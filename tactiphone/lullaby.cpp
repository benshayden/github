#include "lullaby.h"
// http://herbalcell.com/static/sheets/legend-of-zelda-ocarina-of-time/zeldas-lullaby.pdf
note_t lullaby[] = {
/*tone(B4p, half_ms);
  tone(D5p, quarter_ms);
  tone(A4p, half_ms);
  tone(G4p, eighth_ms);
  tone(A4p, eighth_ms);
  tone(B4p, half_ms);
  tone(D5p, quarter_ms);
  tone(A4p, half_dot_ms);

  tone(B4p, half_ms);
  tone(D5p, quarter_ms);
  tone(A5p, half_ms);
  tone(G5p, quarter_ms);
  tone(D5p, half_ms);
  tone(C5p, eighth_ms);
  tone(B4p, eighth_ms);
  tone(A4p, half_dot_ms);

  tone(B4p, half_ms);
  tone(D5p, quarter_ms);
  tone(A4p, half_ms);
  tone(G4p, eighth_ms);
  tone(A4p, eighth_ms);
  tone(B4p, half_ms);
  tone(D5p, quarter_ms);
  tone(A4p, half_dot_ms);

  tone(B4p, half_ms);
  tone(D5p, quarter_ms);
  tone(A5p, half_ms);
  tone(G5p, quarter_ms);
  tone(D6p, half_dot_ms);
  delay(50);
  tone(D6p, half_ms);
  tone(C6p, eighth_ms);
  tone(B5p, eighth_ms);
  tone(C6p, eighth_ms);
  tone(B5p, eighth_ms);
  tone(G5p, half_ms);

  tone(C6p, half_ms);
  tone(B5p, eighth_ms);
  tone(A5p, eighth_ms);
  tone(B5p, eighth_ms);
  tone(A5p, eighth_ms);
  tone(E5p, half_ms);

  tone(D5p, half_ms);
  tone(C5p, eighth_ms);
  tone(B4p, eighth_ms);
  tone(C5p, eighth_ms);
  tone(B4p, eighth_ms);
  tone(G4p, quarter_ms);
  tone(C5p, half_ms);
  tone(F5p, whole_ms);
  */
};

static uint8_t pattern[] = {2,3,1,2,3,1};

recognizer_t lullaby_recognizer = {
	pattern, ARRAYSIZE(pattern),
	lullaby, ARRAYSIZE(lullaby)
};
