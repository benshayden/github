// Search for SETTINGS

#include <avr/pgmspace.h>
#include <TinyWireM.h>
#include <TinyRTClib.h>
// ATTINYs require pull-up resistors on SDA and SCL

#define ARRAYSIZE(a) ((sizeof(a) == 0) ? 0 : (sizeof(a) / sizeof(a[0])))

// Local time when the sketch was compiled.
DateTime dt;

int16_t hm2m(uint8_t h, uint8_t m) {
  return (h * 60) + m;
}

class OnHoldOffEvent {
 public:
  OnHoldOffEvent(uint8_t startHour, uint8_t startMin,
                 uint8_t endHour, uint8_t endMin,
                 uint16_t onMin, uint16_t offMin,
                 uint8_t maxDuty, boolean exceptSleepin) {
    startMin_ = hm2m(startHour, startMin);
    int16_t endMinOfDay = hm2m(endHour, endMin);
    lastStartMin_ = endMinOfDay - onMin - offMin;
    onSec_ = onMin * 60;
    offSec_ = offMin * 60;
    maxDuty_ = maxDuty;
    exceptSleepin_ = exceptSleepin;
  }
  
  boolean process(int16_t nowMin, boolean sleepin) const;

 private:
  int16_t startMin_;
  int16_t lastStartMin_;
  int16_t onSec_;
  int16_t offSec_;
  uint8_t maxDuty_;
  boolean exceptSleepin_;
};

// SETTINGS:

OnHoldOffEvent wake(6, 5, 7, 30, 30, 5, 150, true);
OnHoldOffEvent bed(20, 0, 22, 0, 10, 30, 250, false);

// Uncomment to make a test event that lasts for a few minutes starting every day
// 1 minute after the sketch was compiled.
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

#ifdef TEST
OnHoldOffEvent test(0, 0, 0, 0, 1, 1, 255, false);
#endif

#define SECS_YR_2000 946684800UL
#define SECS_PER_DAY 86400
#define SECS_PER_HOUR 3600

static const uint16_t holidays[] PROGMEM = {HOLIDAYS};
boolean isHoliday(uint32_t day) {
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

static const uint16_t dst_days[] PROGMEM = {
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
boolean isDST(uint32_t day) {
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
void blinkN(uint32_t n) {
  while (n) {
    if ((n % 10) == 0) {
      blink(10, BLINKN_MS, 1);
    } else {
      blink(255, BLINKN_MS, n % 10);
    }
    n /= 10;
    delay(1000);
  }
}

uint16_t sweep(uint32_t i, uint32_t s, uint32_t m) {
  // 1 <= i < m
  // 1 <= m <= 255
  // 1 < s < 60 * 60
  // sum(sweep(i,s,m) for i in range(1,m)) == s
  uint32_t q = (s - ((i * s) / m));
  // 0 < q < s
  q *= q;
  if (q > (0xffffffff / 3000)) {
    q /= max(m, s);
    if (q > (0xffffffff / 3000)) {
      q /= min(m, s);
      q *= 3000;
    } else {
      q *= 3000;
      q /= min(m, s);
    }
  } else {
    q *= 3000;
    q /= s;
    q /= m;
  }
  return q;
}

void delaySeconds(uint32_t s) {
  while (s) {
    delay(1000);
    --s;
  }
}

void onholdoff(uint32_t ons, uint8_t max_duty, uint16_t holds, uint32_t offs) {
  // We'll spend some time at each of the max_duty steps. More at lower
  // steps, less at higher steps, but non-linear so that it looks linear.
  uint8_t i = 1;
  for (; i < max_duty; ++i) {
    analogWrite(LIGHT, i);
    delay(sweep(i, ons, max_duty));
  }
  delaySeconds(holds);
  for (; i > 0; --i) {
    analogWrite(LIGHT, i);
    delay(sweep(i, offs, max_duty));
  }
  analogWrite(LIGHT, 0);
}

boolean OnHoldOffEvent::process(int16_t nowMin, boolean sleepin) const {
  if (exceptSleepin_ && sleepin) return false;
  if ((startMin_ <= nowMin) && (nowMin <= lastStartMin_)) {
    onholdoff(onSec_, maxDuty_, (lastStartMin_ - nowMin) * 60, offSec_);
    return true;
  }
  return false;
}

void setup() {
  pinMode(LIGHT, OUTPUT);
  analogWrite(LIGHT, 0);
  TinyWireM.begin();
  dt = DateTime(__DATE__, __TIME__);
#ifdef TEST
  test = OnHoldOffEvent(dt.hour(), dt.minute() + 1, dt.hour(), dt.minute() + 5, 1, 1, 255, false);
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
  boolean sleepin = !isWeekDay(dt.dayOfWeek()) || isHoliday(d);
  int16_t nowMin = hm2m(dt.hour(), dt.minute());
#define PROCESS(e) if (e.process(nowMin, sleepin)) return
#ifdef TEST
  PROCESS(test);
#endif
  PROCESS(wake);
  PROCESS(bed);
  // If none of the OnHoldOffEvents returned true, then wait for the next minute.
  delaySeconds(55);
}
