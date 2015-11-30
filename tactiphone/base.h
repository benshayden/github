#include <stdint.h>

#if (ARDUINO >= 100)
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#define BIT_TEST(x, i) ((x) & _BV(i))
#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))
#define MAX_UINT32 0xFFFFFFFF
#define boolean bool

void PWM4_init();
uint32_t millis();
uint32_t micros();

void analogWrite4(uint8_t duty_value);
