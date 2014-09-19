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
#define PERIOD_MS 10

// How should the PWM duty cycle sweep?
// SWEEP(0.0) = 0.0; SWEEP(1.0) = 1.0;
// http://en.wikipedia.org/wiki/Gamma_correction
#define SWEEP(x) (x / (1.0 + exp(10 * (0.5 - x))))

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
  delayMicroseconds(floor(s));
}

double PERIOD_S = (((double) PERIOD_MS) / 1000.0);
int log_freq = floor(1000.0 / PERIOD_MS);

// Synchronous! Blocks for total_min minutes!
void fade(bool on, double total_min) {
  if (on) Serial.print("fade on");
  else Serial.print("fade off");
  Serial.println(total_min);
  const double end = on ? 1.0 : 0.0;
  double start = 1.0 - end;
  double step = (on ? 1.0 : -1.0) * PERIOD_S / (60.0 * total_min);
  int log_counter = 0;
  while (on ? (start < end) : (start > end)) {
    double duty_s = SWEEP(start) * PERIOD_S;
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
      Serial.print(" duty_s ");
      Serial.println(duty_s);
    }
  }
  if (on) {
    digitalWrite(LIGHT, HIGH);
  }
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

  fade(1, 1);
  fade(0, 1);
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
  Serial.print("sleepin ");
  Serial.println(sleepin);

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
  Serial.print(minutes);
  Serial.println(" minutes until 6:30");
  if (CLOCK_READS(7, 00)) {
    if (sleepin) {
      delaySeconds(60);
      return;
    } else {
      fade(0, 1);
      return;
    }
  }
  Serial.print(minutes);
  Serial.println(" minutes until 7:00");
  if (CLOCK_READS(19, 00)) {
    fade(1, 10);
    return;
  }
  Serial.print(minutes);
  Serial.println(" minutes until 19:00");
  if (CLOCK_READS(21, 15)) {
    fade(0, 30);
    return;
  }
  Serial.print(minutes);
  Serial.println(" minutes until 21:15");

  // Delay a bit less than the number of minutes until the next event since
  // this timer is less accurate than the RTC.
  delaySeconds(minutes * 55);
}
