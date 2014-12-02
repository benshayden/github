#include "ButtonPlex.h"

uint16_t ButtonPlexClass::debounce_ms = 10;
uint16_t ButtonPlexClass::first_rebounce_ms = 300;
uint16_t ButtonPlexClass::rebounce_ms = 150;
uint16_t ButtonPlexClass::scan_ms = 2;
bool ButtonPlexClass::internal_resistors = true;
uint8_t* ButtonPlexClass::pins_ = NULL;
uint8_t ButtonPlexClass::count_ = 0;
uint8_t ButtonPlexClass::pin_i_ = 0;
uint8_t ButtonPlexClass::connected_ = 0;
unsigned long ButtonPlexClass::started_ = 0;
unsigned long ButtonPlexClass::edge_detected_ = 0;
unsigned long ButtonPlexClass::returned_ = 0;
ButtonPlexClass ButtonPlex;

ButtonPlexClass::ButtonPlexClass(){}

void ButtonPlexClass::setup(uint8_t* pins, uint8_t count) {
  pins_ = pins;
  count_ = count;
  connected_ = count_;
  started_ = millis();
  for (uint8_t i = 0; i < count_; ++i) {
    if (internal_resistors) {
      pinMode(pins_[i], INPUT_PULLUP);
    } else {
      pinMode(pins_[i], OUTPUT);
      digitalWrite(pins_[i], HIGH);
    }
  }
  pinMode(pins_[pin_i_], OUTPUT);
  digitalWrite(pins_[pin_i_], LOW);
}

uint16_t ButtonPlexClass::read(Change* change) {
  Change unused;
  if (change == NULL) {
    change = &unused;
  }
  unsigned long now = millis();
  uint8_t current_connected = scan();
  bool current_state = (current_connected != count_);
  if (current_connected != connected_) {
    // Start debouncing
    edge_detected_ = now;
    *change = (connected_ != count_) ? SAME_PRESSED : SAME_RELEASED;
    connected_ = current_connected;
    return 0;
  }
  // The connected pin is the same as it was last time. But how long has it been
  // the same?
  bool released = ((connected_ == count_) &&
                   (now >= (edge_detected_ + debounce_ms)));
  if ((now >= (started_ + scan_ms)) &&
      ((edge_detected_ == 0) || released)) {
    // We've finished scanning pin_i_ and either no buttons were ever connected,
    // or, if they were, they've debounced disconnecting.
    next(now);
    *change = released ? RELEASED : SAME_RELEASED;
    return 0;
  }
  if ((connected_ == count_) ||
      (now < (edge_detected_ + debounce_ms)) ||
      ((returned_ != 0) &&
       ((now < (edge_detected_ + debounce_ms + first_rebounce_ms)) ||
        (now < (returned_ + rebounce_ms))))) {
    // Wait for debounce (on or off) or rebounce.
    *change = (connected_ != count_) ? SAME_PRESSED : SAME_RELEASED;
    return 0;
  }
  *change = (returned_ == 0) ? PRESSED : SAME_PRESSED;
  returned_ = now;
  // which(1, 0) returns 0; add 1 so that returning 0 always means that some
  // number of buttons other than 1 is pressed.
  return 1 + which(connected_, pin_i_);
}

uint8_t ButtonPlexClass::scan() {
  uint8_t connected = count_;

  for (uint8_t i = 0; i < count_; ++i) {
    if ((i == pin_i_) || digitalRead(pins_[i]))
      continue;
    // A button is connecting pins_[pin_i_] to pins_[i] so that pins_[pin_i_]
    // pulls down pins_[i].
    if (connected != count_) {
      // Too many buttons pressed.
      return count_;
    }
    connected = i;
    // Keep looping through all the other pins to see if another button is
    // pressed.
  }
  return connected;
}

void ButtonPlexClass::next(unsigned long now) {
  if (internal_resistors) {
    pinMode(pins_[pin_i_], INPUT_PULLUP);
  } else {
    pinMode(pins_[pin_i_], OUTPUT);
    digitalWrite(pins_[pin_i_], HIGH);
  }
  pin_i_ = (1 + pin_i_) % count_;
  pinMode(pins_[pin_i_], OUTPUT);
  digitalWrite(pins_[pin_i_], LOW);
  connected_ = count_;
  started_ = now;
  edge_detected_ = 0;
  returned_ = 0;
}

uint16_t ButtonPlexClass::which(uint16_t cathode, uint16_t anode) {
  // The `if(i==pin_i_)continue` in scan() means that cathode can never equal
  // anode. Without this conditional decrement, which() would skip blocks of
  // return values.
  if (cathode > anode)
    --cathode;
  return (cathode * (uint16_t)count_) + anode;
}
