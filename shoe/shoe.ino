#include <avr/pgmspace.h>

// To access keyboard_report_data directly for Teensy boards. Your board may
// differ.
#include "../usb_hid/usb_private.h"

// SETTINGS

#define BUTTON_PINS 5, 6, 7
#define LED_PINS 8, 9, 10

#define NUM_MACROS 5
#define MAX_MACRO_LEN 20

#define SHIFT_LED 0
#define CONTROL_LED 1
#define ALT_LED 2
#define GUI_LED 3
#define MODE0_LED 4
#define MODE1_LED 5

// END SETTINGS

#define ARRAYSIZE(a) ((sizeof(a) == 0) ? 0 : (sizeof(a) / sizeof(a[0])))

typedef struct { uint8_t x; uint8_t y; } uint8_pair_t;
typedef struct { uint8_t* bytes; const uint16_t size; } array8_t;
typedef struct { uint16_t* shorts; const uint16_t size; } array16_t;
typedef struct { unsigned long* longs; const uint16_t size; } arrayL_t;

const uint8_t button_pin_bytes[] = {BUTTON_PINS};
const array8_t button_pins = {(uint8_t*)button_pin_bytes, ARRAYSIZE(button_pin_bytes)};
const uint16_t num_buttons = ARRAYSIZE(button_pin_bytes) * (ARRAYSIZE(button_pin_bytes) - 1);
uint16_t button_shorts[(num_buttons + 15) / 16];
const array16_t buttons = {button_shorts, ARRAYSIZE(button_shorts)};
const uint8_t led_pin_bytes[] = {LED_PINS};
const array8_t led_pins = {(uint8_t*)led_pin_bytes, ARRAYSIZE(led_pin_bytes)};
const uint16_t num_leds = ARRAYSIZE(led_pin_bytes) * (ARRAYSIZE(led_pin_bytes) - 1);
uint16_t led_shorts[(num_leds + 15) / 16];
const array16_t leds = {led_shorts, ARRAYSIZE(led_shorts)};
#include "keycodes.h"
const uint16_t keymap_keys[] PROGMEM = {
#include "keymap"
};

uint16_t macro_keys[NUM_MACROS][MAX_MACRO_LEN];

uint8_t mode = 0;

bool stickey_next = false;
uint8_t current_button = 0;
uint8_t current_led_row = 0;

void mode_write(uint8_t pin, uint8_t mode, uint8_t state) {
  pinMode(pin, mode);
  digitalWrite(pin, state);
}

void mode_write_all(const array8_t& pins, uint8_t mode, uint8_t state) {
  for (uint16_t i = 0; i < pins.size; ++i) {
    mode_write(pins.bytes[i], mode, state);
  }
}

bool read_bit(uint8_t bit, const array16_t& shorts) {
  if ((bit / 16) >= shorts.size) return 0;
  return (shorts.shorts[bit / 16] >> (bit % 16)) & 1;
}

void write_bit(uint8_t bit, bool value, const array16_t& shorts) {
  if ((bit / 16) >= shorts.size) return;
  uint8_t shorti = bit / 16;
  uint16_t bit16 = (bit % 16);
  if (value) {
    shorts.shorts[shorti] |= (1 << bit16);
  } else {
    shorts.shorts[shorti] &= ~(1 << bit16);
  }
}

uint8_pair_t get_pair(uint8_t charlie, uint8_t num_pins) {
  uint8_pair_t result = {charlie % num_pins, charlie / num_pins};
  if (result.y >= result.x) ++result.y;
  return result;
}

uint8_t get_charlie(uint8_t x, uint8_t y, uint8_t num_pins) {
  if (y >= x) --y;
  return (y * num_pins) + x;
}

uint8_pair_t get_pin_pair(uint8_t charlie, const array8_t& pins) {
  uint8_pair_t pair = get_pair(charlie, pins.size);
  pair.x = pins.bytes[pair.x];
  pair.y = pins.bytes[pair.y];
  return pair;
}

bool read_button(uint8_t button) {
  mode_write_all(button_pins, OUTPUT, LOW);
  uint8_pair_t charlie = get_pin_pair(button, button_pins);
  mode_write(charlie.x, INPUT, LOW);
  mode_write(charlie.y, OUTPUT, HIGH);
  delayMicroseconds(1);
  return digitalRead(charlie.x);
}

void paint_led_row() {
  mode_write_all(led_pins, INPUT, LOW);
  bool any = false;
  for (uint8_t d_row = 0; d_row < led_pins.size && !any; ++d_row) {
    current_led_row = (current_led_row + 1) % led_pins.size;
    for (uint8_t x = 0; x < led_pins.size; ++x) {
      if ((x != current_led_row) &&
          read_bit(get_charlie(x, current_led_row, led_pins.size), leds)) {
        mode_write(led_pins.bytes[current_led_row], OUTPUT, HIGH);
        mode_write(led_pins.bytes[x], OUTPUT, LOW);
        any = true;
      }
    }
  }
}

bool toggle_modifier(uint8_t k, bool is_press) {
  switch (k) {
    case KEY_MODE0:
      if (is_press) {
        mode ^= 1;
      }
      break;
    case KEY_MODE1:
      if (is_press) {
        mode ^= 2;
      }
      break;
    case KEY_LEFT_CONTROL:
      keyboard_report_data[0] ^= MODIFIERKEY_LEFT_CTRL;
      break;
    case KEY_LEFT_SHIFT:
      keyboard_report_data[0] ^= MODIFIERKEY_LEFT_SHIFT;
      break;
    case KEY_LEFT_ALT:
      keyboard_report_data[0] ^= MODIFIERKEY_LEFT_ALT;
      break;
    case KEY_LEFT_GUI:
      keyboard_report_data[0] ^= MODIFIERKEY_LEFT_GUI;
      break;
    case KEY_VOLUME_UP:
      keyboard_report_data[1] ^= MEDIA_VOLUME_UP;
      break;
    case KEY_VOLUME_DOWN:
      keyboard_report_data[1] ^= MEDIA_VOLUME_DOWN;
      break;
    case KEY_MUTE:
      keyboard_report_data[1] ^= MEDIA_MUTE;
      break;
    case KEY_PAUSE:
      keyboard_report_data[1] ^= MEDIA_PAUSE;
      break;
    case KEY_NEXT:
      keyboard_report_data[1] ^= MEDIA_NEXT;
      // This keycode is reserved.
      return true;
    case KEY_PREV:
      keyboard_report_data[1] ^= MEDIA_PREV;
      // This keycode is reserved.
      return true;
    case KEY_STOP:
      keyboard_report_data[1] ^= MEDIA_STOP;
      break;
    case KEY_EJECT:
      keyboard_report_data[1] ^= MEDIA_EJECT;
      // This keycode is reserved.
      return true;
  }
  return false;
}

void play_macro(uint8_t mi);

void press(uint16_t k) {
  if ((k & MOD_CONTROL) == MOD_CONTROL) {
    press(KEY_LEFT_CONTROL);
  }
  if ((k & MOD_SHIFT) == MOD_SHIFT) {
    press(KEY_LEFT_SHIFT);
  }
  if ((k & MOD_ALT) == MOD_ALT) {
    press(KEY_LEFT_ALT);
  }
  if ((k & MOD_GUI) == MOD_GUI) {
    press(KEY_LEFT_GUI);
  }
  k &= 0xff;
  if ((KEY_MACRO0 <= k) && (k < (KEY_MACRO0 + NUM_MACROS))) {
    play_macro(k - KEY_MACRO0);
    return;
  }
  if (toggle_modifier(k, true)) {
    // k is reserved.
    return;
  }
  for (uint8_t ri = 0; ri < 6; ++ri) {
    if (keyboard_report_data[2 + ri] == 0) {
      keyboard_report_data[2 + ri] = k;
      break;
    }
  }
}

void release(uint16_t k) {
  if ((k & MOD_CONTROL) == MOD_CONTROL) {
    release(KEY_LEFT_CONTROL);
  }
  if ((k & MOD_SHIFT) == MOD_SHIFT) {
    release(KEY_LEFT_SHIFT);
  }
  if ((k & MOD_ALT) == MOD_ALT) {
    release(KEY_LEFT_ALT);
  }
  if ((k & MOD_GUI) == MOD_GUI) {
    release(KEY_LEFT_GUI);
  }
  k &= 0xff;
  if (toggle_modifier(k, false)) {
    // k is reserved.
    return;
  }
  for (uint8_t ri = 0; ri < 6; ++ri) {
    if (keyboard_report_data[2 + ri] == k) {
      keyboard_report_data[2 + ri] = 0;
      break;
    }
  }
}

void play_macro(uint8_t mi) {
  if (mi >= NUM_MACROS) return;
  for (uint8_t ki = 0; (ki < MAX_MACRO_LEN) && macro_keys[mi][ki]; ++ki) {
    press(macro_keys[mi][ki]);
    Keyboard.send_now();
    delay(3);
    release(macro_keys[mi][ki]);
    Keyboard.send_now();
    delay(3);
  }
}

void setup() {
  mode_write_all(button_pins, OUTPUT, LOW);
  mode_write_all(led_pins, INPUT, LOW);
  macro_keys[0][0] = 4;
  macro_keys[0][1] = MOD_SHIFT | 5;
  macro_keys[0][2] = 6;
  macro_keys[0][3] = MOD_SHIFT | 7;
}

void loop() {
  paint_led_row();
  current_button = (current_button + 1) % num_buttons;
  if (current_button == 0) {
    Keyboard.send_now();
  }
  bool state = read_button(current_button);
  //write_bit(current_button, state, leds);
  uint16_t k = pgm_read_word(keymap_keys + current_button + (mode * num_buttons));
  bool pressed = read_bit(current_button, buttons);
  if (state && !pressed) {
    press(k);
  } else if (pressed && !state) {
    release(k);
  }
  write_bit(current_button, state, buttons);
}
