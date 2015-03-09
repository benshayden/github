#define btn 0
#define led 1
#define spkr 2

// BOM:
// Trinket 3V: http://www.adafruit.com/products/1500
// transducer/speaker e.g. http://www.adafruit.com/products/1674
// battery: http://www.adafruit.com/products/1578
// charger: http://www.adafruit.com/products/259
// button e.g. http://www.adafruit.com/products/1119
// PN2222: http://www.adafruit.com/products/756
// 100R, 1kR, 4.7uF

// RC low-pass filter design tool
// http://sim.okawa-denshi.jp/en/CRlowkeisan.htm

// 4 minutes / 110 in ms
#define whole_ms 2182
#define half_ms (whole_ms / 2)
#define quarter_ms (half_ms / 2)
#define half_dot_ms (half_ms + quarter_ms)
#define eighth_ms (quarter_ms / 2)

// periods in microseconds
// http://www.phy.mtu.edu/~suits/notefreqs.html
// for o in xrange(5, 0, -1):
//   for a in 'B Bb A Ab G Gb F E Eb D Db C'.split():
//     print '#define '+a+str(o)+'p ('+a+str(o-1)+'p / 2)'
// def pus(fhz): return int(round(1e6/fhz))
// for i,n in zip(xrange(-10,-22,-1), 'B Bb A Ab G Gb F E Eb D Db C'.split()):f=55*(2**(i/12.0));print '#define %s0p %d /* %.02f Hz */'%(n, int(round(pus(f))),f)
#define B5p (B4p / 2)
#define Bb5p (Bb4p / 2)
#define A5p (A4p / 2)
#define Ab5p (Ab4p / 2)
#define G5p (G4p / 2)
#define Gb5p (Gb4p / 2)
#define F5p (F4p / 2)
#define E5p (E4p / 2)
#define Eb5p (Eb4p / 2)
#define D5p (D4p / 2)
#define Db5p (Db4p / 2)
#define C5p (C4p / 2)
#define B4p (B3p / 2)
#define Bb4p (Bb3p / 2)
#define A4p (A3p / 2)
#define Ab4p (Ab3p / 2)
#define G4p (G3p / 2)
#define Gb4p (Gb3p / 2)
#define F4p (F3p / 2)
#define E4p (E3p / 2)
#define Eb4p (Eb3p / 2)
#define D4p (D3p / 2)
#define Db4p (Db3p / 2)
#define C4p (C3p / 2)
#define B3p (B2p / 2)
#define Bb3p (Bb2p / 2)
#define A3p (A2p / 2)
#define Ab3p (Ab2p / 2)
#define G3p (G2p / 2)
#define Gb3p (Gb2p / 2)
#define F3p (F2p / 2)
#define E3p (E2p / 2)
#define Eb3p (Eb2p / 2)
#define D3p (D2p / 2)
#define Db3p (Db2p / 2)
#define C3p (C2p / 2)
#define B2p (B1p / 2)
#define Bb2p (Bb1p / 2)
#define A2p (A1p / 2)
#define Ab2p (Ab1p / 2)
#define G2p (G1p / 2)
#define Gb2p (Gb1p / 2)
#define F2p (F1p / 2)
#define E2p (E1p / 2)
#define Eb2p (Eb1p / 2)
#define D2p (D1p / 2)
#define Db2p (Db1p / 2)
#define C2p (C1p / 2)
#define B1p (B0p / 2)
#define Bb1p (Bb0p / 2)
#define A1p (A0p / 2)
#define Ab1p (Ab0p / 2)
#define G1p (G0p / 2)
#define Gb1p (Gb0p / 2)
#define F1p (F0p / 2)
#define E1p (E0p / 2)
#define Eb1p (Eb0p / 2)
#define D1p (D0p / 2)
#define Db1p (Db0p / 2)
#define C1p (C0p / 2)
#define B0p 32396 /* 30.87 Hz */
#define Bb0p 34323 /* 29.14 Hz */
#define A0p 36364 /* 27.50 Hz */
#define Ab0p 38526 /* 25.96 Hz */
#define G0p 40817 /* 24.50 Hz */
#define Gb0p 43244 /* 23.12 Hz */
#define F0p 45815 /* 21.83 Hz */
#define E0p 48540 /* 20.60 Hz */
#define Eb0p 51426 /* 19.45 Hz */
#define D0p 54484 /* 18.35 Hz */
#define Db0p 57724 /* 17.32 Hz */
#define C0p 61156 /* 16.35 Hz */

void tone(uint16_t period_us, int16_t total_ms) {
  uint16_t half_period_us = period_us / 2;
  uint16_t total_us = 0;
  while (total_ms > 0) {
    digitalWrite(spkr, HIGH);
    delayMicroseconds(half_period_us);
    digitalWrite(spkr, LOW);
    delayMicroseconds(half_period_us);
    total_us += period_us;
    total_ms -= (total_us / 1000);
    total_us %= 1000;
  }
}

void lullaby() {
  // http://herbalcell.com/static/sheets/legend-of-zelda-ocarina-of-time/zeldas-lullaby.pdf
  tone(B3p, half_ms);
  tone(D4p, quarter_ms);
  tone(A3p, half_ms);
  tone(G3p, eighth_ms);
  tone(A3p, eighth_ms);
  tone(B3p, half_ms);
  tone(D4p, quarter_ms);
  tone(A3p, half_dot_ms);
  tone(B3p, half_ms);
  tone(D4p, quarter_ms);
  tone(A4p, half_ms);
  tone(G4p, quarter_ms);
  tone(D4p, half_ms);
  tone(C4p, eighth_ms);
  tone(B3p, eighth_ms);
  tone(A3p, half_dot_ms);
#if 0
  tone(B3p, half_ms);
  tone(D4p, quarter_ms);
  tone(A3p, half_ms);
  tone(G3p, eighth_ms);
  tone(A3p, eighth_ms);
  tone(B3p, half_ms);
  tone(D4p, quarter_ms);
  tone(A3p, half_dot_ms);
  tone(B3p, half_ms);
  tone(D4p, quarter_ms);
  tone(A4p, half_ms);
  tone(G4p, quarter_ms);
  tone(D5p, half_dot_ms);
  delay(50);
  tone(D5p, half_ms);
  tone(C5p, eighth_ms);
  tone(B4p, eighth_ms);
  tone(C5p, eighth_ms);
  tone(B4p, eighth_ms);
  tone(G4p, half_ms);
  tone(C5p, half_ms);
  tone(B4p, eighth_ms);
  tone(A4p, eighth_ms);
  tone(B4p, eighth_ms);
  tone(A4p, eighth_ms);
  tone(E4p, half_ms);
  tone(D5p, half_ms);
  tone(C5p, eighth_ms);
  tone(B4p, eighth_ms);
  tone(C5p, eighth_ms);
  tone(B4p, eighth_ms);
  tone(G4p, quarter_ms);
  tone(C5p, quarter_ms);
  tone(F5p, whole_ms);
#endif
}

void setup() {
  pinMode(btn, INPUT);
  pinMode(led, OUTPUT);
  pinMode(spkr, OUTPUT);
}

void loop() {
  if (digitalRead(btn)) {
    digitalWrite(led, HIGH);
    lullaby();
    digitalWrite(led, LOW);
  }
}
