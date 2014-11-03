#include <avr/pgmspace.h>

// To access keyboard_report_data directly for Teensy boards. Your board may
// differ.
#include "../usb_hid/usb_private.h"

// SETTINGS

// See circuit.svg for how to charlie-plex the buttons in a way that is
// compatible with this firmware.
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
#define LOCK_NEXT_LED 6
#define RECORDING_MACRO_LED 7

// END SETTINGS

#define ARRAYSIZE(a) ((sizeof(a) == 0) ? 0 : (sizeof(a) / sizeof(a[0])))

// bitmasks for keyboard_report_data[1]
#define MEDIA_VOLUME_UP   0x01
#define MEDIA_VOLUME_DOWN 0x02
#define MEDIA_MUTE        0x04
#define MEDIA_PAUSE       0x08
#define MEDIA_NEXT        0x10
#define MEDIA_PREV        0x20
#define MEDIA_STOP        0x40
#define MEDIA_EJECT       0x80

typedef struct { uint8_t x; uint8_t y; } uint8_pair_t;
typedef struct { uint8_t* bytes; const uint16_t size; } array8_t;
typedef struct { uint16_t* shorts; const uint16_t size; } array16_t;

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
#include "keymap"
const uint16_t mode_0_keys[] PROGMEM = {MODE_NEITHER_KEYS};
const uint16_t mode_1_keys[] PROGMEM = {MODE_0_KEYS};
const uint16_t mode_2_keys[] PROGMEM = {MODE_1_KEYS};
const uint16_t mode_3_keys[] PROGMEM = {MODE_BOTH_KEYS};
const array16_t keymaps[] = {
  {(uint16_t*) mode_0_keys, ARRAYSIZE(mode_0_keys)},
  {(uint16_t*) mode_1_keys, ARRAYSIZE(mode_1_keys)},
  {(uint16_t*) mode_2_keys, ARRAYSIZE(mode_2_keys)},
  {(uint16_t*) mode_3_keys, ARRAYSIZE(mode_3_keys)}};

uint16_t macro_keys[NUM_MACROS][MAX_MACRO_LEN];

uint8_t mode = 0;
uint8_t once_modifier = 0;
uint8_t once_mode = 0;
bool lock_next = false;
uint8_t current_button = 0;
uint8_t current_button_pin = 0;
uint8_t current_led_row = 0;
uint8_t record_macro = 0xff;
uint8_t record_macro_keyi = 0;

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

void set_led(uint8_t led, bool state) {
  write_bit(led, state, leds);
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

bool is_reserved(uint8_t k) {
  return (((165 <= k) && (k <= 175)) ||
          (k == 222) ||
          (k == 223) ||
          ((232 <= k) && (k <= 255)));
}

void toggle_modifier(uint8_t k) {
  switch (k) {
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
      break;
    case KEY_PREV:
      keyboard_report_data[1] ^= MEDIA_PREV;
      break;
    case KEY_STOP:
      keyboard_report_data[1] ^= MEDIA_STOP;
      break;
    case KEY_EJECT:
      keyboard_report_data[1] ^= MEDIA_EJECT;
      break;
    case KEY_MODE0:
      mode ^= 1;
      break;
    case KEY_MODE1:
      mode ^= 2;
      break;
  }
}

bool is_pressed(uint8_t k) {
  for (uint8_t i = 2; i < 8; ++i) {
    if (keyboard_report_data[i] == k) return true;
  }
  return false;
}

void play_macro(uint8_t mi);
void release(uint16_t k);

void press(uint16_t k) {
  if (record_macro < NUM_MACROS) {
  }
  // handle the MSB
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
  // clear the MSB
  k &= 0xff;
  if (k == KEY_LOCK_NEXT) {
    lock_next = !lock_next;
    return;
  }
  if ((KEY_MACRO0 <= k) && (k < (KEY_MACRO0 + NUM_MACROS))) {
    if (lock_next) {
      record_macro = k - KEY_MACRO0;
      set_led(RECORDING_MACRO_LED, 1);
    } else {
      play_macro(k - KEY_MACRO0);
    }
    return;
  }
  if (k == KEY_MODE0_1) {
    if (lock_next) {
      k = KEY_MODE0_LOCK;
      lock_next = false;
    } else {
      mode ^= 1;
      once_mode |= 1;
      return;
    }
  }
  if (k == KEY_MODE0_LOCK) {
    mode ^= 1;
    return;
  }
  if (k == KEY_MODE1_1) {
    if (lock_next) {
      k = KEY_MODE1_LOCK;
      lock_next = false;
    } else {
      mode ^= 2;
      once_mode |= 2;
      return;
    }
  }
  if (k == KEY_MODE1_LOCK) {
    mode ^= 2;
    return;
  }
  if (k == KEY_CONTROL1) {
    if (lock_next) {
      k = KEY_LEFT_CONTROL_LOCK;
      lock_next = false;
    } else {
      once_modifier ^= MODIFIERKEY_LEFT_CTRL;
      if (is_pressed(KEY_LEFT_CONTROL)) {
        release(KEY_LEFT_CONTROL);
      } else {
        press(KEY_LEFT_CONTROL);
      }
      return;
    }
  }
  if (k == KEY_LEFT_CONTROL_LOCK) {
    // TODO
  }
  if (k == KEY_SHIFT1) {
    if (lock_next) {
      k = KEY_LEFT_SHIFT_LOCK;
      lock_next = false;
    } else {
      once_modifier ^= MODIFIERKEY_LEFT_SHIFT;
      if (is_pressed(KEY_LEFT_SHIFT)) {
        release(KEY_LEFT_SHIFT);
      } else {
        press(KEY_LEFT_SHIFT);
      }
      return;
    }
  }
  if (k == KEY_LEFT_SHIFT_LOCK) {
    // TODO
  }
  if (k == KEY_ALT1) {
    if (lock_next) {
      k = KEY_LEFT_ALT_LOCK;
      lock_next = false;
    } else {
      once_modifier ^= MODIFIERKEY_LEFT_ALT;
      if (is_pressed(KEY_LEFT_ALT)) {
        release(KEY_LEFT_ALT);
      } else {
        press(KEY_LEFT_ALT);
      }
      return;
    }
  }
  if (k == KEY_LEFT_ALT_LOCK) {
    // TODO
  }
  if (k == KEY_GUI1) {
    if (lock_next) {
      k = KEY_LEFT_ALT_LOCK;
      lock_next = false;
    } else {
      once_modifier ^= MODIFIERKEY_LEFT_GUI;
      if (is_pressed(KEY_LEFT_GUI)) {
        release(KEY_LEFT_GUI);
      } else {
        press(KEY_LEFT_GUI);
      }
      return;
    }
  }
  if (k == KEY_LEFT_GUI_LOCK) {
    // TODO
  }
  toggle_modifier(k);
  if (is_reserved(k)) {
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
  if (((k & MOD_CONTROL) == MOD_CONTROL) ||
      ((k != KEY_CONTROL1) &&
       (k != KEY_LEFT_CONTROL) &&
       ((once_modifier & MODIFIERKEY_LEFT_CTRL) != 0))) {
    release(KEY_LEFT_CONTROL);
    once_modifier &= ~MODIFIERKEY_LEFT_CTRL;
  }
  if (((k & MOD_SHIFT) == MOD_SHIFT) ||
      ((k != KEY_SHIFT1) &&
       (k != KEY_LEFT_SHIFT) &&
       ((once_modifier & MODIFIERKEY_LEFT_SHIFT) != 0))) {
    release(KEY_LEFT_SHIFT);
    once_modifier &= ~MODIFIERKEY_LEFT_SHIFT;
  }
  if (((k & MOD_ALT) == MOD_ALT) ||
      ((k != KEY_ALT1) &&
       (k != KEY_LEFT_ALT) &&
       ((once_modifier & MODIFIERKEY_LEFT_ALT) != 0))) {
    release(KEY_LEFT_ALT);
    once_modifier &= ~MODIFIERKEY_LEFT_ALT;
  }
  if (((k & MOD_GUI) == MOD_GUI) ||
      ((k != KEY_GUI1) &&
       (k != KEY_LEFT_GUI) &&
       ((once_modifier & MODIFIERKEY_LEFT_GUI) != 0))) {
    release(KEY_LEFT_GUI);
    once_modifier &= ~MODIFIERKEY_LEFT_GUI;
  }
  if ((k != KEY_MODE0_1) &&
      ((once_mode & 1) != 0)) {
    mode &= ~1;
    once_mode &= ~1;
    return;
  }
  if ((k != KEY_MODE1_1) &&
      ((once_mode & 2) != 0)) {
    mode &= ~2;
    once_mode &= ~2;
    return;
  }
  k &= 0xff;
  toggle_modifier(k);
  if (is_reserved(k)) {
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
  current_button_pin = button_pin_bytes[0];
}

void loop() {
  paint_led_row();

  bool state = digitalRead(current_button_pin);
  current_button = (current_button + 1) % num_buttons;
  mode_write_all(button_pins, OUTPUT, LOW);
  uint8_pair_t charlie = get_pin_pair(current_button, button_pins);
  mode_write(charlie.x, INPUT, LOW);
  mode_write(charlie.y, OUTPUT, HIGH);
  current_button_pin = charlie.x;

  if (current_button == 0) {
    Keyboard.send_now();
  }

  uint16_t k = 0;
  const array16_t& keymap = keymaps[mode];
  if (current_button < keymap.size) {
    k = pgm_read_word(keymap.shorts + current_button);
  }

  bool pressed = read_bit(current_button, buttons);
  if (state != pressed) {
    write_bit(current_button, state, buttons);
    if (k && state) {
      press(k);
    } else if (k && !state) {
      release(k);
    }
  }

  set_led(LOCK_NEXT_LED, lock_next);
  set_led(MODE0_LED, mode & 1);
  set_led(MODE1_LED, mode & 2);
  set_led(CONTROL_LED, keyboard_report_data[0] & MODIFIERKEY_LEFT_CTRL);
  set_led(SHIFT_LED, keyboard_report_data[0] & MODIFIERKEY_LEFT_SHIFT);
  set_led(ALT_LED, keyboard_report_data[0] & MODIFIERKEY_LEFT_ALT);
  set_led(GUI_LED, keyboard_report_data[0] & MODIFIERKEY_LEFT_GUI);

  delayMicroseconds(1);
}
