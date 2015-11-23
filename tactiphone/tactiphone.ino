#include <TinyWireM.h>

#define SPEAKER 1

void setup() {
  pinMode(LIGHT, OUTPUT);
  analogWrite(SPEAKER, 0);
  TinyWireM.begin();
}

void loop() {
  analogWrite(SPEAKER, value);
}
