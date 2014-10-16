// Search for SETTINGS

#include <avr/pgmspace.h>
#include <TinyWireM.h>
#include <TinyRTClib.h>
// ATTINYs require pull-up resistors on SDA and SCL

#define ARRAYSIZE(a) ((sizeof(a) == 0) ? 0 : (sizeof(a) / sizeof(a[0])))

// SETTINGS:

#define WAKE_START_HOUR 6
#define WAKE_START_MIN 5
#define WAKE_END_HOUR 7
#define WAKE_END_MIN 30
#define WAKE_ON_MIN 30
#define WAKE_OFF_MIN 5
#define WAKE_MAX_DUTY 150

#define BED_START_HOUR 20
#define BED_START_MIN 0
#define BED_END_HOUR 22
#define BED_END_MIN 0
#define BED_ON_MIN 10
#define BED_OFF_MIN 30
#define BED_MAX_DUTY 255

// Uncomment to make a test event that lasts for a few minutes starting every day
// 1 minute after the sketch was compiled.
#define TEST

// You have to uncomment this, compile and run it with the RTC connected once in
// order to adjust and start it, then comment the next line back out and
// recompile in order to run the sketch.
//#define START_CLOCK

// Set to false if the RTC was last adjusted during standard (winter) time.
#define RTC_IS_DST true

// number of pin controlling the light. HIGH = on, LOW = off.
#define LIGHT 1

// The light clock does not turn on in the morning on these weekdays so you can
// sleep in.
// Days since 2000/1/1.
#define HOLIDAYS 

// END SETTINGS

int16_t test_start = 0;
#define TEST_MAX_DUTY 255
#define TEST_ON_SEC 30
#define TEST_OFF_SEC 30

#define WAKE_START ((60 * WAKE_START_HOUR) + WAKE_START_MIN)
#define WAKE_END ((60 * WAKE_END_HOUR) + WAKE_END_MIN)
#define WAKE_LAST_START (WAKE_END - WAKE_ON_MIN - WAKE_OFF_MIN)
#define WAKE_ON_SEC (60 * WAKE_ON_MIN)
#define WAKE_OFF_SEC (60 * WAKE_OFF_MIN)

#define BED_START ((60 * BED_START_HOUR) + BED_START_MIN)
#define BED_END ((60 * BED_END_HOUR) + BED_END_MIN)
#define BED_LAST_START (BED_END - BED_ON_MIN - BED_OFF_MIN)
#define BED_ON_SEC (60 * BED_ON_MIN)
#define BED_OFF_SEC (60 * BED_OFF_MIN)

#define SECS_YR_2000 946684800UL
#define SECS_PER_DAY 86400
#define SECS_PER_HOUR 3600

uint16_t holidays[] PROGMEM = {HOLIDAYS};
boolean isHoliday(uint16_t day) {
  if (ARRAYSIZE(holidays) == 0) return false;
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

uint16_t dst_days[] PROGMEM = {
  // DST begins on even-indexed elements, ends on odd-indexed elements
  // elements are days since 2000 Jan 1.
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
boolean isDST(uint16_t day) {
  for (uint8_t i = 0; i < ARRAYSIZE(dst_days); ++i) {
    if (day < pgm_read_word(&(dst_days[i]))) {
      return (i % 2) == 1;
    }
  }
  return false;
}

void blink(uint8_t level, uint16_t ms, uint8_t times) {
  while (times) {
    analogWrite(LIGHT, level);
    delay(ms);
    analogWrite(LIGHT, 0);
    delay(ms);
    --times;
  }
}

#define BLINKN_MS 300
void blinkN(uint16_t n) {
  while (n) {
    if ((n % 10) == 0) {
      blink(5, BLINKN_MS, 1);
    } else {
      blink(255, BLINKN_MS, n % 10);
    }
    n /= 10;
    delay(1000);
  }
}

void delaySeconds(uint16_t s) {
  while (s) {
    delay(1000);
    --s;
  }
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

void delaySweep(uint16_t i, uint16_t s, uint16_t m) {
  // 1 <= i < m
  // 1 <= m <= 255
  // 1 <= s < 60 * 60
  // Delay about this many ms:
  // ((s-(i*s/m))**2)*1000*3/(s*m)
  // If that's more than 30000, then just delaySeconds() instead.
  // Order of operations affects underflow/overflow.
  uint16_t ism = muldiv16(i, s, m);
  uint16_t sism = s - ism;
  uint16_t q = muldiv16(muldiv16(sism, sism, s), 3, m);
  if (q > 30) {
    delaySeconds(q);
  } else {
    q = muldiv16(muldiv16(sism, 3000, m), sism, s);
    delaySeconds(q / 1000);
    delay(q % 1000);
  }
}

void onholdoff(uint16_t ons, uint8_t max_duty, uint16_t holds, uint16_t offs) {
  // We'll spend some time at each of the max_duty steps. More at lower
  // steps, less at higher steps, but non-linear so that it looks linear.
  uint8_t i = 1;
  for (; i < max_duty; ++i) {
    analogWrite(LIGHT, i);
    delaySweep(i, ons, max_duty);
  }
  delaySeconds(holds);
  for (; i > 0; --i) {
    analogWrite(LIGHT, i);
    delaySweep(i, offs, max_duty);
  }
  analogWrite(LIGHT, 0);
}

boolean process(uint16_t nowMin, uint16_t startMin, uint16_t lastStartMin, uint16_t onSec, uint8_t maxDuty, uint16_t offSec) {
  if ((startMin <= nowMin) && (nowMin <= lastStartMin)) {
    onholdoff(onSec, maxDuty, (lastStartMin - nowMin) * 60, offSec);
    return true;
  }
  return false;
}

DateTime dt;

void setup() {
  pinMode(LIGHT, OUTPUT);
  analogWrite(LIGHT, 0);
  TinyWireM.begin();
  dt = DateTime(__DATE__, __TIME__);
#ifdef TEST
  test_start = (60 * ((uint16_t) dt.hour())) + dt.minute() + 1;
#endif
#ifdef START_CLOCK
  RTC_DS1307::adjust(dt);
#endif
}

boolean isWeekDay(uint8_t d) {
  return (1 <= d) && (d <= 5);
}

void loop() {
  dt = RTC_DS1307::now();
  uint32_t t = dt.unixtime();
  uint32_t d = (t - SECS_YR_2000) / SECS_PER_DAY;
  t += RTC_IS_DST ? (isDST(d) ? 0 : -SECS_PER_HOUR) : (isDST(d) ? SECS_PER_HOUR : 0);
  dt = t;
  uint16_t nowMin = (60 * ((uint16_t) dt.hour())) + dt.minute();
#ifdef TEST
  if (test_start && process(nowMin, test_start, test_start + 1, TEST_ON_SEC, TEST_MAX_DUTY, TEST_OFF_SEC)) {
    test_start = 0;
    return;
  }
#endif
  if (isWeekDay(dt.dayOfWeek()) && !isHoliday(d) &&
      process(nowMin, WAKE_START, WAKE_LAST_START, WAKE_ON_SEC, WAKE_MAX_DUTY, WAKE_OFF_SEC)) return;
  if (process(nowMin, BED_START, BED_LAST_START, BED_ON_SEC, BED_MAX_DUTY, BED_OFF_SEC)) return;
  // If none of the OnHoldOffEvents returned true, then wait for the next minute.
  delaySeconds(55);
}
