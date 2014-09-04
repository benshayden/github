// https://www.pjrc.com/teensy/td_libs_Wire.html
// http://www.adafruit.com/products/1065
// https://www.pjrc.com/teensy/external_power.html
// http://docs.macetech.com/doku.php/chronodot_v2.0

#include <avr/pgmspace.h>
#include <LowPower.h>
#include <Wire.h>
#include <SPI.h>
#include <RTClib.h>
#include <RTC_DS3231.h>

#define LIGHT 11
#define PERIOD_MS 20
#define TRANSFER_LINEAR
//#define TRANSFER_GAMMA
//#define TRANSFER_SIGMOID
//#define TRANSFER_GAMMA_SIGMOID
#define GAMMA 2
#define SIGMOID_SCALE 7
//#define FAKE_CLOCK

#define kuint32max 0xffffffff
#define ARRAYSIZE(a) ((sizeof(a) == 0) ? 0 : (sizeof(a) / sizeof(a[0])))

// Days since 2000/1/1.
static const uint16_t holidays[] PROGMEM = {};
// uint16_t dayMillenium() const;
// uint16_t DateTime::dayMillenium() const {return date2days(yOff, m, d);}

bool isHoliday(uint16_t day) {
  // TODO binary search
  for (uint8_t i = 0; i < ARRAYSIZE(holidays); ++i) {
    if (day == pgm_read_word(holidays + i)) {
      return true;
    }
  }
  return false;
}

// dst[even] = dayMillenium() when Daylight Savings Time begins. dst[odd] DST ends.
uint16_t dst[] PROGMEM = {};

bool isDST(uint16_t day) {
  if (day < pgm_read_word(dst)) return false;
  for (uint8_t i = 1; i < ARRAYSIZE(dst); ++i) {
    if ((pgm_read_word(dst + i - 1) < day) && (day < pgm_read_word(dst + i))) {
      return (i % 2);
    }
  }
  return false;
}

#define delayMilliseconds delay
void delaySeconds(unsigned long n) {
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

double sigmoid(double x) {
  return 1.0 / (1.0 + exp(SIGMOID_SCALE * (0.5 - x)));
}

#define PERIOD_US (1000 * PERIOD_MS)

void fade(bool on, double total_min) {
  double total_s = total_min * 60.0;
  double start_pct = on ? 0.0 : 100.0;
  double end_pct = on ? 100.0 : 0.0;
  double duty_us = start_pct * PERIOD_MS * 10;
  int start_us = start_pct * PERIOD_MS * 10;
  int end_us = end_pct * PERIOD_MS * 10;
  double range_us = abs(end_us - start_us);
  double range_pct = end_pct - start_pct;
#if defined(TRANSFER_LINEAR)
  double step_us = (range_pct * PERIOD_MS * PERIOD_MS) / total_s;
#elif defined(TRANSFER_GAMMA) || defined(TRANSFER_SIGMOID) || defined(TRANSFER_GAMMA_SIGMOID)
  double step_pct = range_pct * PERIOD_MS / (1000.0 * total_s);
#endif
  while (on ? (duty_us < end_us) : (duty_us > end_us)) {
    digitalWrite(LIGHT, HIGH);
    delayMicroseconds((int) duty_us);
    digitalWrite(LIGHT, LOW);
    delayMicroseconds((int) (PERIOD_US - duty_us));
#if defined(TRANSFER_LINEAR)
    duty_us += step_us;
#elif defined(TRANSFER_GAMMA)
    start_pct += step_pct;
    duty_us = pow(start_pct, GAMMA) * PERIOD_US;
#elif defined(TRANSFER_SIGMOID)
    start_pct += step_pct;
    duty_us = sigmoid(start_pct) * PERIOD_US
#elif defined(TRANSFER_GAMMA_SIGMOID)
    start_pct += step_pct;
    duty_us = pow(start_pct, GAMMA) * sigmoid(start_pct) * PERIOD_US;
#endif
  }
  if (on) {
    digitalWrite(LIGHT, HIGH);
  }
}

#ifdef FAKE_CLOCK
RTC_Millis RTCm;
#else
RTC_DS3231 RTC;
#endif

void setup() {
  pinMode(LIGHT, OUTPUT);
  digitalWrite(LIGHT, LOW);
  Serial.begin(9600);
  Wire.begin();
  RTC.begin();
  DateTime compiled = DateTime(__DATE__, __TIME__);
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

bool isWeekDay(uint8_t dayOfWeek) {
  return (dayOfWeek >= 1) && (dayOfWeek <= 5);
}

uint16_t minutesUntil(const DateTime& t, int8_t h, int8_t m) {
  h = (h - t.hour()) % 24;
  m = m - t.minute();
  if (m < 0) {
    m += 60;
    h -= 1;
  }
  h %= 24;
  return (h * 60) + m;
}

void loop() {
  DateTime now = RTC.now();
  Serial.print(now.unixtime());
  Serial.print(" ");
  Serial.print(now.dayMillenium());
  Serial.print(" ");
  Serial.println(isDST(now.dayMillenium()) ? "DST" : "notDST");
  if (isDST(now.dayMillenium())) {
    now = DateTime(now.unixtime() + (60 * 60));
    Serial.println(now.unixtime());
  }

  uint16_t minutes = 24 * 60;
  if (isWeekDay(now.dayOfWeek()) &&
      !isHoliday(now.dayMillenium())) {
    minutes = min(minutes, minutesUntil(now, 6, 30));
    if (minutes == 0) {
      fade(1, 20);
    } else {
      minutes = min(minutes, minutesUntil(now, 7, 0));
      if (minutes == 0) {
        fade(0, 1);
      }
    }
  } else {
    minutes = min(minutes, minutesUntil(now, 19, 0));
    if (minutes == 0) {
      fade(1, 10);
    } else {
      minutes = min(minutes, minutesUntil(now, 21, 15));
      if (minutes == 0) {
        fade(0, 30);
      }
    }
  }

  Serial.print("minutes ");
  Serial.println(minutes);
  if (minutes == 1) {
    delaySeconds(30);
  } else if (minutes > 0) {
    // Sleep a bit less than the number of minutes until the next event since
    // the sleep timer is less accurate than the RTC.
    delaySeconds(minutes * 55);
  }
}
