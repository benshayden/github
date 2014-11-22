#define YELLOW 0
#define ORANGE 1

void PWM4_init() {
  // Set up PWM on Trinket GPIO #4 (PB4, pin 3) using Timer 1
  pinMode(4, OUTPUT);
  TCCR1 = _BV (CS10); // no prescaler
  GTCCR = _BV (COM1B1) | _BV (PWM1B); // clear OC1B on compare
  OCR1B = 0; // duty cycle initialize to 0%
  OCR1C = 255; // frequency
}

void analogWrite4(uint8_t duty_value) {
  OCR1B = duty_value;
}

typedef struct {
  uint8_t value;
  uint8_t max_value;
  uint8_t duration;
} pulser;

pulser yellow;
pulser orange;

void setup() {
  pinMode(YELLOW, OUTPUT);
  analogWrite(YELLOW, 0);
  pinMode(ORANGE, OUTPUT);
  analogWrite(ORANGE, 0);
  PWM4_init();
}

void pulse(pulser color) {
  if (color.value == 0) {
    
  }
}

void loop() {
  pulse(yellow);
  pulse(orange);
  analogWrite4(constrain(((int16_t)280 - (int16_t)yellow.value - (int16_t)orange.value), 0, 255));
}
