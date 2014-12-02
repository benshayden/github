#ifndef __ARDUINO_H_
#define __ARDUINO_H_

#define random stdlib_random

#include <execinfo.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#undef random

#define ARRAYSIZE(x) ((sizeof(x) == 0) ? 0 : (sizeof(x) / sizeof(x[0])))
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define SERIAL  0x0
#define DISPLAY 0x1
#define LSBFIRST 0
#define MSBFIRST 1
#define CHANGE 1
#define FALLING 2
#define RISING 3
#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define abs(x) ((x)>0?(x):-(x))
#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define round(x)     ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
long map(long x, long in_min, long in_max, long out_min, long out_max);
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))
#define interrupts() sei()
#define noInterrupts() cli()
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))
#define LOW 0
#define HIGH 1
#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))
unsigned long millis();
unsigned long micros();
void pinMode(int pin, int mode);
#define DEFAULT 0
#define INTERNAL 1
#define INTERNAL1V1 2
#define INTERNAL2V56 3
#define EXTERNAL 4
void analogReference(int ref);
void analogReadResolution(int res);
void analogWriteResolution(int res);
void tone(int pin, int freq, int dur=0);
void noTone(int pin);
void shiftOut(int datapin, int clockpin, int bitorder, int value);
int shiftIn(int datapin, int clockpin, int bitorder);
long pulseIn(int pin, int value, int timeout=0);
#define A0 0
#define A1 1
#define A2 2
#define A3 3
#define A4 4
#define A5 5
int analogRead(int pin);
void analogWrite(int pin, int value);
void digitalWrite(int pin, bool value);
bool digitalRead(int pin);
void delay(int ms);
void delayMicroseconds(long us);
int random(long bound, long upper=0);
void randomSeed(int s);
class KeyboardClass {
 public:
  static void set_modifier(uint8_t mod);
  static void set_key1(uint8_t key);
  static void send_now();
};
extern KeyboardClass Keyboard;
class MouseClass {
 public:
  static void move(int8_t dx, int8_t dy);
  static void scroll(int8_t ds);
  static void set_buttons(bool left, bool middle, bool right);
};
extern MouseClass Mouse;
#define DEC 10
#define HEX 16
#define OCT 8
#define BIN 2
class Print {
 public:
  static size_t print(const char*);
  static size_t print(char);
  static size_t print(int, int = DEC);
  static size_t print(double, int = 2);
  static size_t println(const char*);
  static size_t println(char);
  static size_t println(int, int = DEC);
  static size_t println(double, int = 2);
  static size_t println(void);
};
class Serial_ : public Print {
 public:
  static void begin(uint16_t baud_count);
  static void end();
  static int available();
  static void accept();
  static int peek();
  static int read();
  static void flush();
  static size_t write(uint8_t);
  static size_t write(const char*);
  static size_t write(const char*, size_t);
  static size_t print(const char*);
  static size_t print(char);
  static size_t print(int, int = DEC);
  static size_t print(double, int = 2);
  static size_t println(const char*);
  static size_t println(char);
  static size_t println(int, int = DEC);
  static size_t println(double, int = 2);
  static size_t println(void);
  static bool _bool();
  operator bool() {return _bool();}
};
extern Serial_ Serial;

#endif  // __ARDUINO_H_
