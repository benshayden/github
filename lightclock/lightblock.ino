//#include <RTClib.h>
// light slow on, fast off at 6:30 on weekdays except holidays
// light fast on, slow off at 8 on weeknights except holidays
#include <LowPower.h>
char holidays[][3] = {};
int light = 6;
void setup() {
  pinMode(light, OUTPUT);
}
int period = 10000;
int duty = 0;
int step = 100;
void loop() {
  digitalWrite(light, HIGH);
  delayMicroseconds(duty);
  digitalWrite(light, LOW);
  delayMicroseconds(period - duty);
  if (((duty + step) > period) || ((duty + step) < 0)) {
    step *= -1;
  }
  duty += step;
}
