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

// SETTINGS

Timezone myTZ(PACIFIC);

// number of pin controlling the light. HIGH = on, LOW = off.
#define LIGHT 11

// PWM cycle time
#define PERIOD_MS 20

// How should the PWM duty cycle sweep?
// SWEEP(0.0) = 0.0; SWEEP(1.0) = 1.0;
//#define SWEEP(x) x
// http://en.wikipedia.org/wiki/Gamma_correction
//#define SWEEP(x) pow(x, 2)
//#define SWEEP(x) sigmoid(x)
#define SWEEP(x) pow(x, 2) * sigmoid(x)

// Decrease to make sigmoid() more gradual, increase to make dark darker and
// light lighter.
#define SIGMOID_SCALE 7

// Uncomment to use arduino's crappy clock instead of an RTC.
#define FAKE_CLOCK

// The light clock does not turn on in the morning on these weekdays so you can
// sleep in.
// Days since 2000/1/1.
#define HOLIDAYS 

// Uncomment to fade on then off over two minutes when the program starts.
#define START_FADE

// END SETTINGS


#ifdef FAKE_CLOCK
RTC_Millis RTC;
#else
RTC_DS3231 RTC;
#endif

time_t RTCunixtime() {
  return RTC.now().unixtime();
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

void delaySeconds(unsigned long n) {
  while (n) {
#ifdef FAKE_CLOCK
    // LowPower doesn't increment millis() correctly.
    delay(1000);
    --n;
#else
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
      --n;
    }
    LowPower.powerDown(p, ADC_OFF, BOD_OFF);
#endif
    Serial.print("delaySeconds "); Serial.print(n); Serial.print(" utc "); Serial.println(RTCunixtime());
  }
}

void delayManyMicroseconds(unsigned long us) {
  while (us > 4000) {
    us -= 2000;
    delay(2);
  }
  delayMicroseconds(us);
}

void superDelay(double s) {
  // TODO LowPower loop
  // TODO delay() loop
  // TODO delayMicroseconds() loop
}

double sigmoid(double x) {
  return 1.0 / (1.0 + exp(SIGMOID_SCALE * (0.5 - x)));
}

#define PERIOD_US (1000 * PERIOD_MS)

// Synchronous! Blocks for total_min minutes!
void fade(bool on, double total_min) {
  if (on) Serial.print("fade on");
  else Serial.print("fade off");
  Serial.println(total_min);
  const double end = on ? 1.0 : 0.0;
  double start = 1.0 - end;
  double step = (on ? 1.0 : -1.0) * PERIOD_MS / (1000.0 * 60.0 * total_min);
  int c = 0;
  while (on ? (start < end) : (start > end)) {
    long duty_us = SWEEP(start) * PERIOD_US;
    digitalWrite(LIGHT, HIGH);
    delayManyMicroseconds(duty_us);
    digitalWrite(LIGHT, LOW);
    delayManyMicroseconds(PERIOD_US - duty_us);
    start += step;
    ++c;
    if (c == 50) {
      c = 0;
      Serial.print("start ");
      Serial.print(start);
      Serial.print(" duty_us ");
      Serial.println(duty_us);
    }
  }
  if (on) {
    digitalWrite(LIGHT, HIGH);
  }
}

#ifdef FAKE_CLOCK
RTC_Millis RTC;
#else
RTC_DS3231 RTC;
#endif

time_t RTCunixtime() {
  return RTC.now().unixtime();
}

void setup() {
  pinMode(LIGHT, OUTPUT);
  digitalWrite(LIGHT, LOW);
  Serial.begin(9600);
  Wire.begin();
  DateTime compiled = DateTime(__DATE__, __TIME__);
  compiled = DateTime(compiled.unixtime() + (7 * 60 * 60));
#ifdef FAKE_CLOCK
  RTC.adjust(compiled);
#else
  RTC.begin();
  if (!RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    RTC.adjust(compiled);
  }
  DateTime dt = RTC.now();
  if (dt.unixtime() < compiled.unixtime()) {
    Serial.println("RTC is older than compile time!  Updating");
    RTC.adjust(compiled);
  }
#endif
  setSyncProvider(RTCunixtime);

#ifdef START_FADE
  fade(1, 1);
  fade(0, 1);
#endif
}

bool isWeekDay(uint8_t d) {
  return (d >= 1) && (d <= 5);
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
  time_t utc = now();
  Serial.print("utc ");
  Serial.println(utc);
  TimeChangeRule* tcr;
  time_t local = myTZ.toLocal(utc, &tcr);
  Serial.print(tcr->abbrev);
  Serial.print(" ");
  Serial.println(local);
  DateTime localdt = local;
  Serial.print("localdt ");
  char localdts[30];
  Serial.println(localdt.toString(localdts, sizeof(localdts)));
  bool sleepin = !isWeekDay(dayOfWeek(local)) || isHoliday(local);
  Serial.print("sleepin "); Serial.println(sleepin);

  uint16_t minutes = 24 * 60;
#define CLOCK_READS(h, m) (0 == (minutes = min(minutes, minutesUntil(localdt, (h), (m)))))
  if (CLOCK_READS(6, 30)) {
    if (sleepin) {
      delaySeconds(60);
      return;
    } else {
      fade(1, 20);
      return;
    }
  }
  Serial.print(minutes); Serial.println(" minutes until 6:30");
  if (CLOCK_READS(7, 00)) {
    if (sleepin) {
      delaySeconds(60);
      return;
    } else {
      fade(0, 1);
      return;
    }
  }
  Serial.print(minutes); Serial.println(" minutes until 7:00");
  if (CLOCK_READS(19, 00)) {
    fade(1, 10);
    return;
  }
  Serial.print(minutes); Serial.println(" minutes until 19:00");
  if (CLOCK_READS(21, 15)) {
    fade(0, 30);
    return;
  }
  Serial.print(minutes); Serial.println(" minutes until 21:15");

  // Delay a bit less than the number of minutes until the next event since
  // this timer is less accurate than the RTC.
  delaySeconds(minutes * 55);
}
