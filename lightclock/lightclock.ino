#include <avr/pgmspace.h>
#include <TinyWireM.h>
#include <TinyRTClib.h>

#define ARRAYSIZE(a) ((sizeof(a) == 0) ? 0 : (sizeof(a) / sizeof(a[0])))

// SETTINGS:

// You have to uncomment this, compile and run it with the RTC connected once in order to adjust and start it, then comment the next line back out in order to run it.
//#define START_CLOCK

// Set to false if the RTC was last adjusted during standard (winter) time.
#define RTC_IS_DST true

// No parens, no colons. "08" and "09" are an error; use "8" or "9".
#define WAKE_ON 30
#define WAKE_START 6, 5
#define WAKE_MAX 0.7
#define PREBED_START 20, 0
#define PREBED_ON 10
#define PREBED_MAX 0.9
#define BED_OFF 30
#define PREBED_END 22, 0

// number of pin controlling the light. HIGH = on, LOW = off.
#define LIGHT 1

// The light clock does not turn on in the morning on these weekdays so you can
// sleep in.
// Days since 2000/1/1.
#define HOLIDAYS 

// END SETTINGS

#define SECS_YR_2000 946684800UL
#define SECS_PER_DAY 86400

static const uint16_t holidays[] PROGMEM = {HOLIDAYS};
bool isHoliday(uint32_t day) {
  uint16_t imin = 0, imax = ARRAYSIZE(holidays) - 1;
  uint16_t imid = 0;
  while (imax >= imin) {
    imid = imin + ((imax - imin) / 2);
    uint16_t holiday = pgm_read_word(&(holidays[imid]));
    if (holiday == day) {
      return true;
    } else if (holiday < day) {
      imin = imid + 1;
    } else {
      imax = imid - 1;
    }
  }
  return false;
}

static const uint16_t dst_days[] PROGMEM = {
// DST begins on even-indexed elements, ends on odd-indexed elements
5181, 5419,
5545, 5783,
5916, 6154,
6280, 6518,
6644, 6882,
7008, 7246,
7372, 7610,
7743, 7981,
8107, 8345,
8471, 8709,
8835, 9073,
9199, 9437,
9563, 9801,
9934, 10172,
10298, 10536,
10662, 10900,
};
bool isDST(uint32_t day) {
  for (uint8_t i = 0; i < ARRAYSIZE(dst_days); ++i) {
    if (day < pgm_read_word(&(holidays[i]))) {
      return (i % 2) == 1;
    }
  }
  return false;
}

void delaySeconds(double s) {
  while (s > 1.0) {
    delay(1000);
    s -= 1.0;
  }
  s *= 1000.0;
  delay(floor(s));
  s -= floor(s);
  s *= 1000.0;
  delayMicroseconds(round(s));
}

// dj=[1-math.sqrt(i/256.) for i in xrange(256]
// di=[j/sum(dj) for j in dj]
// assert abs(sum(di) - 1) < 1e-6
// sum_dj_min = 60.0 / sum(dj)
const double sum_dj_min = 0.698924681;

void onholdoff(double on_min, double x, double hold_min, double off_min) {
  // There are 255 steps. We'll spend some time at all of them. More at lower
  // steps, less at higher steps, but non-linear.
  on_min *= sum_dj_min;
  off_min *= sum_dj_min;
  uint8_t y = x * 256;
  uint8_t i = 1;
  for (; i < y; ++i) {
    analogWrite(LIGHT, i);
    delaySeconds(on_min * (1.0 - sqrt((i - 1) / 256.0)));
  }
  delaySeconds(hold_min * 60.0);
  for (; i > 0; --i) {
    analogWrite(LIGHT, i);
    delaySeconds(off_min * (1.0 - sqrt((i - 1) / 256.0)));
  }
  analogWrite(LIGHT, 0);
}

DateTime dt;
void setup() {
  pinMode(LIGHT, OUTPUT);
  analogWrite(LIGHT, 0);
  TinyWireM.begin();
#ifdef START_CLOCK
  // Local time when the sketch was compiled.
  RTC_DS1307::adjust(DateTime(__DATE__, __TIME__));
  analogWrite(LIGHT, 200);
#else
  double tens = 10.0 / 60.0;
  onholdoff(tens, 0.8, tens, tens);
#endif
}

bool isWeekDay(uint8_t d) {
  return (1 <= d) && (d <= 5);
}

int16_t hm2m(uint8_t h, uint8_t m) {
  return (h * 60) + m;
}

int16_t wakeMin = hm2m(WAKE_START);
int16_t prebedMin = hm2m(PREBED_START);
int16_t bedMin = hm2m(PREBED_END);
int16_t bedHoldMin = bedMin - prebedMin - PREBED_ON - BED_OFF;

void loop() {
#ifdef START_CLOCK
  delay(1000);
#else
  dt = RTC_DS1307::now();
  uint32_t t = dt.unixtime();
  uint32_t d = (t - SECS_YR_2000) / SECS_PER_DAY;
  if (isDST(d)) {
    dt = t - 60 * 60;
  }
  bool sleepin = !isWeekDay(dt.dayOfWeek()) || isHoliday(d);
  int16_t minOfDay = hm2m(dt.hour(), dt.minute());
  if (!sleepin && ((wakeMin - 1) <= minOfDay) && (minOfDay <= (wakeMin + WAKE_ON))) {
    onholdoff(WAKE_ON, WAKE_MAX, WAKE_ON - (minOfDay - wakeMin), 5);
    return;
  }
  if (((prebedMin - 1) <= minOfDay) && (minOfDay <= (prebedMin + bedHoldMin))) {
    int16_t bedHold = bedHoldMin - (minOfDay - prebedMin);
    onholdoff(PREBED_ON, PREBED_MAX, bedHold, BED_OFF);
    return;
  }
  double minutes = min(60, min(constrain(wakeMin - minOfDay, 1, 60), constrain(prebedMin - minOfDay, 1, 60)));
  if (minOfDay > prebedMin) {
    minutes = 60;
  }
  // Don't oversleep.
  delaySeconds(50.0 * minutes);
#endif
}
