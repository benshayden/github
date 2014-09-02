// TODO light slow on, fast off at 6:30 on weekdays except holidays
// TODO light fast on, slow off at 8 on weeknights except holidays
// TODO RTClib doesn't handle DST! We need to handle DST!
// https://www.pjrc.com/teensy/td_libs_Wire.html
// http://www.adafruit.com/products/1065
// https://www.pjrc.com/teensy/external_power.html
// http://docs.macetech.com/doku.php/chronodot_v2.0

#include <LowPower.h>
#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <RTC_DS3231.h>

#define kuint32max 0xffffffff

// Days since 2000/1/1.
uint16_t holidays[] = {};
// uint16_t dayMillenium() const;
// uint16_t DateTime::dayMillenium() const {return date2days(yOff, m, d);}

// dst[odd] = dayMillenium() when Daylight Savings Time begins. dst[even] DST ends.
uint16_t dst[] = {};

int light = 11;

RTC_DS3231 RTC;
RTC_Millis RTCm;

int period = 10000;
int duty = 0;
int step = 100;

#define delayMilliseconds delay
void delaySeconds(int n) {
  while (n) {
    period_t p = SLEEP_1S;
    if (n >= 8) {
      n -= 8;
      p = SLEEP_8S;
    } else if (n >= 4) {
      n -= 4;
      p = SLEEP_4S;
    } else if (n >= 2) {
      n -= 2;
      p = SLEEP_2S;
    } else {
      n--;
    }
    LowPower.powerDown(p, ADC_OFF, BOD_OFF);
  }
}

void setup() {
  pinMode(light, OUTPUT);
  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  DateTime compiled = DateTime(__DATE__, __TIME__);
  RTCm.adjust(compiled);
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // Set the RTC to the date & time this sketch was compiled
    RTC.adjust(compiled);
  }
  DateTime now = RTC.now();
  if (now.unixtime() < compiled.unixtime()) {
    Serial.println("RTC is older than compile time!  Updating");
    RTC.adjust(compiled);
  }
}

void loop() {
  digitalWrite(light, HIGH);
  delayMicroseconds(duty);
  digitalWrite(light, LOW);
  unsigned long start = micros();
  if (((duty + step) > period) || ((duty + step) < 0)) {
    step *= -1;
    if (step > 0) {
      delaySeconds(1);
      DateTime now = RTC.now();
      if (now.month() == 0) {
        now = RTCm.now();
      }
      //Serial.println(now.unixtime());
      //Serial.println(now.dayMillenium());
      char buf[30];
      now.toString(buf, 30);
      //Serial.println(buf);
    }
  }
  duty += step;
  unsigned long nowus = micros();
  unsigned long working = (nowus < start) ? (nowus + (kuint32max - start)) : (nowus - start);
  delayMicroseconds(period - duty - working);
}
