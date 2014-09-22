// Scroll down to SETTINGS

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
#include <Time.h>
#include <Timezone.h>

#define kuint32max 0xffffffff
#define ARRAYSIZE(a) ((sizeof(a) == 0) ? 0 : (sizeof(a) / sizeof(a[0])))
TimeChangeRule EDT = {"EDT", Second, Sun, Mar, 2, -240};
TimeChangeRule EST = {"EST", First, Sun, Nov, 2, -300};
TimeChangeRule CDT = {"CDT", Second, Sun, Mar, 2, -300};
TimeChangeRule CST = {"CST", First, Sun, Nov, 2, -360};
TimeChangeRule MDT = {"MDT", Second, Sun, Mar, 2, -360};
TimeChangeRule MST = {"MST", First, Sun, Nov, 2, -420};
TimeChangeRule PDT = {"PDT", Second, Sun, Mar, 2, -420};
TimeChangeRule PST = {"PST", First, Sun, Nov, 2, -480};
#define EASTERN EDT, EST
#define CENTRAL CDT, CST
#define MOUNTAIN MDT, MST
#define PACIFIC PDT, PST
#define CIE1931(L) ((L <= 0.088) ? (L / 9.033) : pow(((L + 0.16) / 1.16), 3))
#define GAMMA(x) pow(x, 2)
#define LOGISTIC(x, s) (1.0 / (1.0 + exp((s) * (0.5 - (x)))))

// SETTINGS

Timezone myTZ(PACIFIC);

// No parens, no colons. "08" and "09" are an error; use "8" or "9".
#define WAKE 6, 15

#define PREBED 20, 0

// number of pin controlling the light. HIGH = on, LOW = off.
#define LIGHT 11

// PWM cycle time
#define PERIOD_MS 10

// How should the PWM duty cycle sweep?
// SWEEP(0.0) = 0.0; SWEEP(1.0) = 1.0;
#define SWEEP(x) GAMMA(x)

// The light clock does not turn on in the morning on these weekdays so you can
// sleep in.
// Days since 2000/1/1.
#define HOLIDAYS 

// END SETTINGS

RTC_Millis SoftRTC;
RTC_DS3231 HardRTC;

time_t RTCunixtime() {
  return HardRTC.isrunning() ? HardRTC.now().unixtime() : SoftRTC.now().unixtime();
}

static const uint16_t holidays[] PROGMEM = {HOLIDAYS};
bool isHoliday(uint32_t t) {
  t -= SECS_YR_2000;
  uint16_t day = t / SECS_PER_DAY;
  // TODO binary search
  for (uint8_t i = 0; i < ARRAYSIZE(holidays); ++i) {
    if (day == pgm_read_word(&(holidays[i]))) {
      return true;
    }
  }
  return false;
}

void delaySeconds(double s) {
  while (s > 1.0) {
    if (HardRTC.isrunning()) {
      period_t p = SLEEP_1S;
      if (s >= 8.0) {
        s -= 8.0;
        p = SLEEP_8S;
      } else if (s >= 4.0) {
        s -= 4.0;
        p = SLEEP_4S;
      } else if (s >= 2.0) {
        s -= 2.0;
        p = SLEEP_2S;
      } else {
        s -= 1.0;
      }
      LowPower.powerDown(p, ADC_OFF, BOD_OFF);
    } else {
      // LowPower doesn't increment millis().
      delay(1000);
      s -= 1.0;
    }
    Serial.print("delaySeconds ");
    Serial.print(s);
    Serial.print(" utc ");
    Serial.println(RTCunixtime());
  }
  s *= 1000.0;
  delay(floor(s));
  s -= floor(s);
  s *= 1000.0;
  delayMicroseconds(round(s));
}

double PERIOD_S = (((double) PERIOD_MS) / 1000.0);
int log_freq = floor(1000.0 / PERIOD_MS);

void transition(bool on, double total_min, double scale) {
  if (on) Serial.print("transition on");
  else Serial.print("transition off");
  Serial.println(total_min);
  const double end = on ? 1.0 : 0.0;
  double start = 1.0 - end;
  double step = (on ? 1.0 : -1.0) * PERIOD_S / (60.0 * total_min);
  int log_counter = 0;
  while (on ? (start < end) : (start > end)) {
    double duty_s = SWEEP(start) * scale * PERIOD_S;
    digitalWrite(LIGHT, HIGH);
    delaySeconds(duty_s);
    digitalWrite(LIGHT, LOW);
    delaySeconds(PERIOD_S - duty_s);
    start += step;
    ++log_counter;
    if (log_counter == log_freq) {
      log_counter = 0;
      Serial.print("start ");
      Serial.print(start);
      Serial.print(" duty_us ");
      Serial.println(round(duty_s * 1e6));
    }
  }
}

void hold(double x, time_t until_utc) {
  double duty_s = x * PERIOD_S;
  while (now() < until_utc) {
    for (int s = 0; s < 10; ++s) {
      for (int i = 0; i < log_freq; ++i) {
        digitalWrite(LIGHT, HIGH);
        delaySeconds(duty_s);
        digitalWrite(LIGHT, LOW);
        delaySeconds(PERIOD_S - duty_s);
      }
    }
  }
}

void onholdoff(double on_min, double x, double hold_min, double off_min) {
  transition(1, on_min, x);
  hold(x, now() + round(60.0 * hold_min));
  transition(0, off_min, x);
}

void setup() {
  pinMode(LIGHT, OUTPUT);
  digitalWrite(LIGHT, LOW);
  Serial.begin(9600);
  Wire.begin();
  DateTime compiled_local = DateTime(__DATE__, __TIME__);
  Serial.print("compiled ");
  Serial.print(compiled_local.unixtime());
  TimeChangeRule* tcr;
  myTZ.toLocal(compiled_local.unixtime(), &tcr);
  Serial.print(tcr->abbrev);
  DateTime compiled_utc = DateTime(compiled_local.unixtime() - (60 * tcr->offset));
  Serial.print(" ");
  Serial.print(compiled_utc.unixtime());
  Serial.println(" UTC");

  SoftRTC.adjust(compiled_utc);
  Serial.print("SoftRTC ");
  Serial.println(SoftRTC.now().unixtime());

  HardRTC.begin();
  if (!HardRTC.isrunning()) {
    Serial.println("HardRTC is NOT running!");
    HardRTC.adjust(compiled_utc);
  }
  DateTime dt = HardRTC.now();
  Serial.print("HardRTC ");
  Serial.println(dt.unixtime());
  if (dt.unixtime() < compiled_utc.unixtime()) {
    Serial.println("RTC is older than compile time!  Updating");
    HardRTC.adjust(compiled_utc);
    Serial.print("HardRTC ");
    Serial.println(HardRTC.now().unixtime());
  }
  setSyncProvider(RTCunixtime);

  double tens = 10.0 / 60.0;
  onholdoff(tens, 0.8, tens, tens);
}

bool isWeekDay(uint8_t d) {
  return (1 <= d) && (d <= 5);
}

uint16_t hm2m(uint8_t h, uint8_t m) {
  return (h * 60) + m;
}

void loop() {
  time_t utc = now();
  TimeChangeRule* tcr;
  time_t local = myTZ.toLocal(utc, &tcr);
  DateTime localdt = local;
  bool sleepin = !isWeekDay(dayOfWeek(local)) || isHoliday(local);
  uint16_t minOfDay = hm2m(localdt.hour(), localdt.minute());
  uint16_t wakeMin = hm2m(WAKE);
  uint16_t prebedMin = hm2m(PREBED);
  Serial.print("loop ");
  Serial.print(utc);
  Serial.print("UTC / ");
  Serial.print(local);
  Serial.print(tcr->abbrev);
  Serial.print("; localdt ");
  char localdts[30];
  Serial.print(localdt.toString(localdts, sizeof(localdts)));
  Serial.print("; sleepin ");
  Serial.print(sleepin);
  Serial.print("; wakeMin ");
  Serial.print(wakeMin);
  Serial.print("; prebedMin ");
  Serial.print(prebedMin);
  Serial.print("; minOfDay ");
  Serial.println(minOfDay);
  Serial.println();
#define BETWEEN(a, z) (((a) <= minOfDay) && (minOfDay <= (z)))
  if (BETWEEN(0, wakeMin - 90)) {
    delaySeconds(60 * 60);
  } else if (BETWEEN(wakeMin - 90, wakeMin - 3)) {
    if (sleepin) {
      delaySeconds(60 * 60);
    } else {
      delaySeconds(60);
    }
  } else if (BETWEEN(wakeMin - 3, wakeMin + 3)) {
    if (sleepin) {
      delaySeconds(60 * 60);
    } else {
      onholdoff(20, 0.8, 20, 5);
    }
  } else if (BETWEEN(wakeMin, prebedMin - 90)) {
    delaySeconds(60 * 60);
  } else if (BETWEEN(prebedMin - 90, prebedMin - 3)) {
    delaySeconds(60);
  } else if (BETWEEN(prebedMin - 3, prebedMin + 3)) {
    onholdoff(10, 0.9, 80, 30);
  } else if (BETWEEN(prebedMin, hm2m(24, 60))) {
    delaySeconds(60 * 60);
  } else {
    Serial.println("woops");
    delaySeconds(60);
  }
}