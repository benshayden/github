#ifndef PWM4_init
void PWM4_init() {
  // Set up PWM on Trinket GPIO #4 (PB4, pin 3) using Timer 1
  pinMode(4, OUTPUT);
  TCCR1 = _BV (CS10); // no prescaler
  GTCCR = _BV (COM1B1) | _BV (PWM1B); // clear OC1B on compare
  OCR1B = 0; // duty cycle initialize to 0%
  OCR1C = 255; // frequency
}
#endif

#ifndef analogWrite4
void analogWrite4(uint8_t duty_value) {
  OCR1B = duty_value;
}
#endif

void setup() {
  pinMode(0, OUTPUT);
  analogWrite(0, 0);
  pinMode(1, OUTPUT);
  analogWrite(1, 0);
  PWM4_init();
  randomSeed(analogRead(1));
}

uint16_t muldiv16(uint16_t a, uint16_t b, uint16_t c) {
  // return a*b/c if it fits in 16 bits
  uint16_t q = 0;
  uint16_t r = 0;
  uint16_t qn = b / c;
  uint16_t rn = b % c;
  while(a) {
    if (a & 1) {
      q += qn;
      r += rn;
      if (r >= c) {
        q++;
        r -= c;
      }
    }
    a  >>= 1;
    qn <<= 1;
    rn <<= 1;
    if (rn >= c) {
      qn++; 
      rn -= c;
    }
  }
  return q;
}

uint16_t delaySweep(uint16_t i, uint16_t s, uint16_t m) {
  // 1 <= i < m
  // 10 <= m <= 255
  // 1 <= s < 2000
  // returns about ((s-(i*s/m))**2)*1000*3/(s*m)
  uint16_t sism = s - muldiv16(s, i, m);
  return muldiv16(muldiv16(sism, 3000, m), sism, s);
}

typedef struct {
  uint8_t value; // 0-255
  uint8_t max_value;
  uint8_t duration; // ms
  unsigned long tick;  // micros()
  uint16_t delay_us;
  bool fall;
} pulser;

pulser yellow;
pulser orange;

void pulse(pulser color, uint8_t pin) {
  unsigned long now = micros();
  if (color.tick > now) {
    // micros() overflowed
    if (color.delay_us < ((0xffffffff - color.tick) + now)) {
      return;
    }
  } else if ((color.tick + color.delay_us) < now) {
    return;
  }
  if (color.value == 0) {
    color.duration = random(750, 2000) / 2;
    color.max_value = random(10, 256);
    color.fall = false;
  }
  if (color.fall) {
    --color.value;
  } else {
    ++color.value;
    if (color.value == color.max_value) {
      color.fall = true;
    }
  }
  color.tick = now;
  color.delay_us = delaySweep(color.value, color.duration, color.max_value);
  analogWrite(pin, color.value);
}

void loop() {
  pulse(yellow, 0);
  pulse(orange, 1);
  analogWrite4(constrain(((int16_t)280 - (int16_t)yellow.value - (int16_t)orange.value), 0, 255));
}
