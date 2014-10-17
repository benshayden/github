#include <avr/pgmspace.h>
#include <TinyWireM.h>
#include <TinyRTClib.h>

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

// Uncomment to make a test event that runs for a few seconds
// shortly after the sketch was compiled.
//#define TEST

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

// Convert (hour, minute) to minute of day.
#define HM2M(h, m) ((((uint16_t) (h)) * 60) + ((uint16_t) (m)))

uint16_t test_start = 0;
#define TEST_MAX_DUTY 255
#define TEST_ON_SEC 10
#define TEST_HOLD_SEC 3
#define TEST_OFF_SEC 10

#define WAKE_START HM2M(WAKE_START_HOUR, WAKE_START_MIN)
#define WAKE_END (HM2M(WAKE_END_HOUR, WAKE_END_MIN) - WAKE_OFF_MIN)
#define WAKE_ON_SEC (60 * WAKE_ON_MIN)
#define WAKE_OFF_SEC (60 * WAKE_OFF_MIN)

#define BED_START HM2M(BED_START_HOUR, BED_START_MIN)
#define BED_END (HM2M(BED_END_HOUR, BED_END_MIN) - BED_OFF_MIN)
#define BED_ON_SEC (60 * BED_ON_MIN)
#define BED_OFF_SEC (60 * BED_OFF_MIN)

#define ARRAYSIZE(a) ((sizeof(a) == 0) ? 0 : (sizeof(a) / sizeof(a[0])))

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
  uint16_t sism = s - muldiv16(s, i, m);
  uint16_t q = muldiv16(muldiv16(sism, sism, s), 3, m);
  if (q > 30) {
    delaySeconds(q);
  } else {
    q = muldiv16(muldiv16(sism, 3000, m), sism, s);
    delaySeconds(q / 1000);
    delay(q % 1000);
  }
}

boolean isOn = false;

void fade(uint16_t totals, uint8_t max_duty) {
  // We'll spend some time at each of the max_duty steps. More at lower
  // steps, less at higher steps, but non-linear so that it looks linear.
  uint8_t i = isOn ? max_duty : 1;
  while (isOn ? (i > 0) : (i < max_duty)) {
    analogWrite(LIGHT, i);
    delaySweep(i, totals, max_duty);
    if (isOn) --i;
    else ++i;
  }
  analogWrite(LIGHT, i);
  isOn = !isOn;
}

DateTime dt;

void setup() {
  pinMode(LIGHT, OUTPUT);
  analogWrite(LIGHT, 0);
  TinyWireM.begin();
#if defined(START_CLOCK)
  dt = DateTime(__DATE__, __TIME__);
  RTC_DS1307::adjust(dt);
#endif
#if defined(TEST)
  dt = RTC_DS1307::now();
  test_start = HM2M(dt.hour(), dt.minute() + 1);
#endif
}

boolean isWeekDay(uint8_t d) {
  return (1 <= d) && (d <= 5);
}

void loop() {
  dt = RTC_DS1307::now();
  uint16_t d = dt.day2000();
  uint16_t h = dt.hour();
  if (RTC_IS_DST && !isDST(d)) {
    // subtract an hour
    if (h == 0) {
      h += 23;
      d -= 1;
    } else {
      --h;
    }
  } else if (!RTC_IS_DST && isDST(d)) {
    // add an hour
    if (h == 23) {
      h = 0;
      d += 1;
    } else {
      ++h;
    }
  }
  uint16_t nowMin = HM2M(h, dt.minute());

#ifdef TEST
  if (!isOn && test_start && (test_start <= nowMin)) {
    fade(TEST_ON_SEC, TEST_MAX_DUTY);
    delaySeconds(TEST_HOLD_SEC);
    return;
  }
  if (isOn && test_start && (test_start < nowMin)) {
    fade(TEST_OFF_SEC, TEST_MAX_DUTY);
    test_start = 0;
    return;
  }
#endif
  if (!isOn && isWeekDay((d + 6) % 7) && !isHoliday(d) &&
      (WAKE_START <= nowMin) && (nowMin < WAKE_END)) {
    fade(WAKE_ON_SEC, WAKE_MAX_DUTY);
  } else if (isOn && (WAKE_END <= nowMin)) {
    fade(WAKE_OFF_SEC, WAKE_MAX_DUTY);
  } else if (!isOn && (BED_START <= nowMin) && (nowMin < BED_END)) {
    fade(BED_ON_SEC, BED_MAX_DUTY);
  } else if (isOn && (BED_END <= nowMin)) {
    fade(BED_OFF_SEC, BED_MAX_DUTY);
  } else {
    delaySeconds(57);
  }
}
