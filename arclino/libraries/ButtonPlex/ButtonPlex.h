#ifndef BUTTONPLEX_BUTTONPLEX_H_
#define BUTTONPLEX_BUTTONPLEX_H_

#include "Arduino.h"

// Manages reading and debouncing a Charlieplexed array of buttons.
// INPUT_PULLUP is used so no resistors are necessary, just diodes:
// http://upload.wikimedia.org/wikipedia/commons/8/83/Diode_pinout_en_fr.svg
class ButtonPlexClass {
 public:
  static uint16_t debounce_ms;
  static uint16_t first_rebounce_ms;
  static uint16_t rebounce_ms;
  static uint16_t scan_ms;

  // Whether to use INPUT_PULLUP instead of OUTPUT/HIGH. Default: true.
  static bool internal_resistors;

  ButtonPlexClass();

  // Example:
  // uint8_t button_plex_pins[] = {10, 11, 12, 13};
  // void setup(){
  //   ButtonPlex.setup(button_plex_pins, sizeof(button_plex_pins));
  // }
  static void setup(uint8_t* pins, uint8_t count);

  enum Change {
    // A button was just pressed. read() returns its non-zero index.
    PRESSED,
    // The button is still pressed. read() may return 0 if it's waiting for a
    // rebounce. Set first_rebounce_ms and rebounce_ms = 0 to make read() always
    // return the button code while SAME_PRESSED (except when debouncing).
    SAME_PRESSED,
    // A button was just released. read() returns 0.
    RELEASED,
    // There still aren't any buttons pressed.
    SAME_RELEASED,
  };

  // Returns 0 if zero or multiple buttons are pressed.
  // When a button has been pressed for at least debounce_ms, return that
  // button's number, in the range [1, count*count-count].
  // As long as the button stays down, returns its number once every
  // rebounce_ms, and 0 in between rebounces.
  // Does not delay, and should be called as frequently as possible.
  static uint16_t read(Change* change = NULL);

 private:
  static uint8_t* pins_;
  static uint8_t count_;

  // The pin that is currently configured OUTPUT LOW.
  static uint8_t pin_i_;

  // The pin that is currently configured INPUT_PULLUP, but is pulled low
  // because it is connected to pins_[pin_i_].
  static uint8_t connected_;

  // The last time that next() reconfigured pins.
  static unsigned long started_;

  // When a rising/falling edge was detected on connected_. If (connected_ ==
  // count_), then a pin was disconnected; if connected_ < count_, then a pin
  // was connected.
  static unsigned long edge_detected_;

  // The last time that read() returned non-zero. For rebouncing.
  static unsigned long returned_;


  // Read all the pins_ except pins_[pin_i_] to see if any of them are connected
  // to pins_[pin_i_].
  static uint8_t scan();

  // Reconfigure the pins_ to start scanning the next block of buttons.
  static void next(unsigned long now);

  // Map a pin pair to a button index.
  // For example, for count_ = 3:
  // cat an index=which+1
  // 1   0  1
  // 0   1  2
  // 0   2  3
  // 2   0  4
  // 2   1  5
  // 1   2  6
  static uint16_t which(uint16_t cathode, uint16_t anode);
};
extern ButtonPlexClass ButtonPlex;

#endif  // BUTTONPLEX_BUTTONPLEX_H_
