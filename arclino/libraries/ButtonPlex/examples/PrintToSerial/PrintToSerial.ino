#include "ButtonPlex.h"

uint8_t pins[] = {3, 4, 5};

void setup() {
  ButtonPlex.setup(pins, sizeof(pins));
}

void loop() {
  uint16_t button = ButtonPlex.read();
  if (button) {
    Serial.print(button);
    Serial.print(" ");
    Serial.println(ButtonPlex.is_rebounce());
  }
  delay(1);
}
