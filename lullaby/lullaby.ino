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

// 4 / (110 / minute) in ms
#define whole_ms 2182
#define half_ms (whole_ms / 2)
#define quarter_ms (half_ms / 2)
#define half_dot_ms (half_ms + quarter_ms)
#define eighth_ms (quarter_ms / 2)

// Musical note periods in microseconds
// http://www.phy.mtu.edu/~suits/notefreqs.html
// for i in xrange(7*12-1,-1,-1):f=440*(2**((i-57)/12.));print '#define %s%dp %d /* %.02f Hz */'%('B Bb A Ab G Gb F E Eb D Db C'.split()[(11-i)%12],i/12,round(1e6/f),f)
#define B6p 506 /* 1975.53 Hz */
#define Bb6p 536 /* 1864.66 Hz */
#define A6p 568 /* 1760.00 Hz */
#define Ab6p 602 /* 1661.22 Hz */
#define G6p 638 /* 1567.98 Hz */
#define Gb6p 676 /* 1479.98 Hz */
#define F6p 716 /* 1396.91 Hz */
#define E6p 758 /* 1318.51 Hz */
#define Eb6p 804 /* 1244.51 Hz */
#define D6p 851 /* 1174.66 Hz */
#define Db6p 902 /* 1108.73 Hz */
#define C6p 956 /* 1046.50 Hz */
#define B5p 1012 /* 987.77 Hz */
#define Bb5p 1073 /* 932.33 Hz */
#define A5p 1136 /* 880.00 Hz */
#define Ab5p 1204 /* 830.61 Hz */
#define G5p 1276 /* 783.99 Hz */
#define Gb5p 1351 /* 739.99 Hz */
#define F5p 1432 /* 698.46 Hz */
#define E5p 1517 /* 659.26 Hz */
#define Eb5p 1607 /* 622.25 Hz */
#define D5p 1703 /* 587.33 Hz */
#define Db5p 1804 /* 554.37 Hz */
#define C5p 1911 /* 523.25 Hz */
#define B4p 2025 /* 493.88 Hz */
#define Bb4p 2145 /* 466.16 Hz */
#define A4p 2273 /* 440.00 Hz */
#define Ab4p 2408 /* 415.30 Hz */
#define G4p 2551 /* 392.00 Hz */
#define Gb4p 2703 /* 369.99 Hz */
#define F4p 2863 /* 349.23 Hz */
#define E4p 3034 /* 329.63 Hz */
#define Eb4p 3214 /* 311.13 Hz */
#define D4p 3405 /* 293.66 Hz */
#define Db4p 3608 /* 277.18 Hz */
#define C4p 3822 /* 261.63 Hz */
#define B3p 4050 /* 246.94 Hz */
#define Bb3p 4290 /* 233.08 Hz */
#define A3p 4545 /* 220.00 Hz */
#define Ab3p 4816 /* 207.65 Hz */
#define G3p 5102 /* 196.00 Hz */
#define Gb3p 5405 /* 185.00 Hz */
#define F3p 5727 /* 174.61 Hz */
#define E3p 6067 /* 164.81 Hz */
#define Eb3p 6428 /* 155.56 Hz */
#define D3p 6810 /* 146.83 Hz */
#define Db3p 7215 /* 138.59 Hz */
#define C3p 7645 /* 130.81 Hz */
#define B2p 8099 /* 123.47 Hz */
#define Bb2p 8581 /* 116.54 Hz */
#define A2p 9091 /* 110.00 Hz */
#define Ab2p 9631 /* 103.83 Hz */
#define G2p 10204 /* 98.00 Hz */
#define Gb2p 10811 /* 92.50 Hz */
#define F2p 11454 /* 87.31 Hz */
#define E2p 12135 /* 82.41 Hz */
#define Eb2p 12856 /* 77.78 Hz */
#define D2p 13621 /* 73.42 Hz */
#define Db2p 14431 /* 69.30 Hz */
#define C2p 15289 /* 65.41 Hz */
#define B1p 16198 /* 61.74 Hz */
#define Bb1p 17161 /* 58.27 Hz */
#define A1p 18182 /* 55.00 Hz */
#define Ab1p 19263 /* 51.91 Hz */
#define G1p 20408 /* 49.00 Hz */
#define Gb1p 21622 /* 46.25 Hz */
#define F1p 22908 /* 43.65 Hz */
#define E1p 24270 /* 41.20 Hz */
#define Eb1p 25713 /* 38.89 Hz */
#define D1p 27242 /* 36.71 Hz */
#define Db1p 28862 /* 34.65 Hz */
#define C1p 30578 /* 32.70 Hz */
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
  tone(B2p, half_ms);
  tone(D3p, quarter_ms);
  tone(A2p, half_ms);
  tone(G2p, eighth_ms);
  tone(A2p, eighth_ms);
  tone(B2p, half_ms);
  tone(D3p, quarter_ms);
  tone(A2p, half_dot_ms);

  tone(B2p, half_ms);
  tone(D3p, quarter_ms);
  tone(A3p, half_ms);
  tone(G3p, quarter_ms);
  tone(D3p, half_ms);
  tone(C3p, eighth_ms);
  tone(B2p, eighth_ms);
  tone(A2p, half_dot_ms);

  tone(B2p, half_ms);
  tone(D3p, quarter_ms);
  tone(A2p, half_ms);
  tone(G2p, eighth_ms);
  tone(A2p, eighth_ms);
  tone(B2p, half_ms);
  tone(D3p, quarter_ms);
  tone(A2p, half_dot_ms);

  tone(B2p, half_ms);
  tone(D3p, quarter_ms);
  tone(A3p, half_ms);
  tone(G3p, quarter_ms);
  tone(D4p, half_dot_ms);
  delay(50);
  tone(D4p, half_ms);
  tone(C4p, eighth_ms);
  tone(B3p, eighth_ms);
  tone(C4p, eighth_ms);
  tone(B3p, eighth_ms);
  tone(G3p, half_ms);

  tone(C4p, half_ms);
  tone(B3p, eighth_ms);
  tone(A3p, eighth_ms);
  tone(B3p, eighth_ms);
  tone(A3p, eighth_ms);
  tone(E3p, half_ms);

  tone(D4p, half_ms);
  tone(C4p, eighth_ms);
  tone(B3p, eighth_ms);
  tone(C4p, eighth_ms);
  tone(B3p, eighth_ms);
  tone(G3p, quarter_ms);
  tone(C4p, quarter_ms);
  tone(F4p, whole_ms);
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
