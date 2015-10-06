#include <avr/pgmspace.h>
#include <CapacitiveSensor.h>

// You may need to change this next line if you don't use a Teensy
// microcontroller in order to allow direct access to `keyboard_report_data`.
#include "../usb_hid/usb_private.h"

// SETTINGS

// This firmware cannot support more than 16 button pins (240 buttons). In order
// to support >240 buttons, `current_button` would need to be changed to
// uint16_t.
#define LED_PINS 4, 5, 6, 7
#define BUTTON_PINS 12, 13, 14, 15, 16, 17, 18

#define NUM_MACROS 10
#define MAX_MACRO_LEN 100

#define SHIFT_LED           0
#define CONTROL_LED         1
#define ALT_LED             2
#define GUI_LED             6
#define MODE0_LED           4
#define MODE1_LED           5
#define LOCK_NEXT_LED       3
#define RECORDING_MACRO_LED 7
#define NUM_LOCK_LED        8
#define CAPS_LOCK_LED       9
#define SCROLL_LOCK_LED     10
#define COMPOSE_LED         11
#define KANA_LED            12

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

#define MACRO_PRESS 0x8000

CapacitiveSensor capsens0(0, 1);
CapacitiveSensor capsens1(0, 2);

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
  // These keycodes are never sent to the host though they may be used
  // internally.
  return ((k < KEY_A) ||
          ((165 <= k) && (k <= 175)) ||
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
    case KEY_RIGHT_CONTROL:
      keyboard_report_data[0] ^= MODIFIERKEY_RIGHT_CTRL;
      break;
    case KEY_RIGHT_SHIFT:
      keyboard_report_data[0] ^= MODIFIERKEY_RIGHT_SHIFT;
      break;
    case KEY_RIGHT_ALT:
      keyboard_report_data[0] ^= MODIFIERKEY_RIGHT_ALT;
      break;
    case KEY_RIGHT_GUI:
      keyboard_report_data[0] ^= MODIFIERKEY_RIGHT_GUI;
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
      // Ubuntu doesn't recognize this.
      keyboard_report_data[1] ^= MEDIA_NEXT;
      break;
    case KEY_PREV:
      // Ubuntu doesn't recognize this.
      keyboard_report_data[1] ^= MEDIA_PREV;
      break;
    case KEY_STOP:
      // Ubuntu recognizes this as keycode 136 keysym 0xff69 "Cancel".
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
      lock_next = false;
      if (record_macro < NUM_MACROS) {
        // Don't actually record the key that, when replayed, would start
        // recording the macro again.
        macro_keys[record_macro][record_macro_keyi - 1] = 0;
        record_macro = 0xff;
        record_macro_keyi = 0;
      } else {
        record_macro = k - KEY_MACRO0;
        record_macro_keyi = 0;
        for (uint16_t ki = 0; ki < MAX_MACRO_LEN; ++ki) {
          macro_keys[record_macro][ki] = 0;
        }
      }
    } else {
      play_macro(k - KEY_MACRO0);
      // Not sure why playing macros sometimes leaves lock_next true.
      lock_next = false;
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
  if (k == KEY_CONTROL_1) {
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
    k = KEY_LEFT_CONTROL;
  }
  if (k == KEY_SHIFT_1) {
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
    k = KEY_LEFT_SHIFT;
  }
  if (k == KEY_ALT_1) {
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
    k = KEY_LEFT_ALT;
  }
  if (k == KEY_GUI_1) {
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
    k = KEY_LEFT_GUI;
  }
  if (k == KEY_CLEAR_MODIFIERS) {
    mode = 0;
    lock_next = false;
    keyboard_report_data[0] = 0;
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
  bool isnt_once = ((k != KEY_CONTROL_1) &&
                    (k != KEY_SHIFT_1) &&
                    (k != KEY_ALT_1) &&
                    (k != KEY_GUI_1) &&
                    (k != KEY_MODE0_1) &&
                    (k != KEY_MODE1_1));
  // don't release any once-modifier when any once-modifier is released
  if (((k & MOD_CONTROL) == MOD_CONTROL) ||
      (isnt_once &&
       (k != KEY_LEFT_CONTROL) &&
       (k != KEY_RIGHT_CONTROL) &&
       ((once_modifier & MODIFIERKEY_LEFT_CTRL) != 0))) {
    release(KEY_LEFT_CONTROL);
    once_modifier &= ~MODIFIERKEY_LEFT_CTRL;
  }
  if (((k & MOD_SHIFT) == MOD_SHIFT) ||
      (isnt_once &&
       (k != KEY_LEFT_SHIFT) &&
       (k != KEY_RIGHT_SHIFT) &&
       ((once_modifier & MODIFIERKEY_LEFT_SHIFT) != 0))) {
    release(KEY_LEFT_SHIFT);
    once_modifier &= ~MODIFIERKEY_LEFT_SHIFT;
  }
  if (((k & MOD_ALT) == MOD_ALT) ||
      (isnt_once &&
       (k != KEY_LEFT_ALT) &&
       (k != KEY_RIGHT_ALT) &&
       ((once_modifier & MODIFIERKEY_LEFT_ALT) != 0))) {
    release(KEY_LEFT_ALT);
    once_modifier &= ~MODIFIERKEY_LEFT_ALT;
  }
  if (((k & MOD_GUI) == MOD_GUI) ||
      (isnt_once &&
       (k != KEY_LEFT_GUI) &&
       (k != KEY_RIGHT_GUI) &&
       ((once_modifier & MODIFIERKEY_LEFT_GUI) != 0))) {
    release(KEY_LEFT_GUI);
    once_modifier &= ~MODIFIERKEY_LEFT_GUI;
  }
  if (isnt_once &&
      ((once_mode & 1) != 0)) {
    mode &= ~1;
    once_mode &= ~1;
  }
  if (isnt_once &&
      ((once_mode & 2) != 0)) {
    mode &= ~2;
    once_mode &= ~2;
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
  release(0);  // release all once modifiers
  uint16_t k = 0;
  bool is_press = false;
  for (uint8_t ki = 0; (ki < MAX_MACRO_LEN) && macro_keys[mi][ki]; ++ki) {
    k = macro_keys[mi][ki];
    is_press = (k & MACRO_PRESS) == MACRO_PRESS;
    k &= ~MACRO_PRESS;
    if (k) {
      if (is_press) {
        press(k);
      } else {
        release(k);
      }
      Keyboard.send_now();
      delay(5);
    }
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

  bool pressed = read_bit(current_button, buttons);
  if (state == pressed) {
    return;
  }
  write_bit(current_button, state, buttons);
  uint16_t k = 0;
  const array16_t& keymap = keymaps[mode];
  if (current_button < keymap.size) {
    k = pgm_read_word(keymap.shorts + current_button);
  }
  if (k == 0) {
    return;
  }
  if (record_macro < NUM_MACROS) {
    macro_keys[record_macro][record_macro_keyi] = k + (state ? MACRO_PRESS : 0);
    ++record_macro_keyi;
    if (record_macro_keyi >= MAX_MACRO_LEN) {
      record_macro = 0xff;
      record_macro_keyi = 0;
    }
  }
  if (state) {
    press(k);
  } else {
    release(k);
  }
  if (false) {
    long mousex = capsens0.capacitiveSensor(30);
    long mousey = capsens1.capacitiveSensor(30);
    Mouse.move(mousex, mousey, 0);
  }

  set_led(CONTROL_LED,
      ((keyboard_report_data[0] & MODIFIERKEY_LEFT_CTRL) != 0) ||
      ((keyboard_report_data[0] & MODIFIERKEY_RIGHT_CTRL) != 0));
  set_led(SHIFT_LED,
      ((keyboard_report_data[0] & MODIFIERKEY_LEFT_SHIFT) != 0) ||
      ((keyboard_report_data[0] & MODIFIERKEY_RIGHT_SHIFT) != 0));
  set_led(ALT_LED,
      ((keyboard_report_data[0] & MODIFIERKEY_LEFT_ALT) != 0) ||
      ((keyboard_report_data[0] & MODIFIERKEY_RIGHT_ALT) != 0));
  set_led(GUI_LED,
      ((keyboard_report_data[0] & MODIFIERKEY_LEFT_GUI) != 0) ||
      ((keyboard_report_data[0] & MODIFIERKEY_RIGHT_GUI) != 0));
  set_led(MODE0_LED, (mode & 1) != 0);
  set_led(MODE1_LED, (mode & 2) != 0);
  set_led(LOCK_NEXT_LED, lock_next);
  set_led(RECORDING_MACRO_LED, record_macro < NUM_MACROS);
  // keyboard_leds is sent from the host asynchronously from button presses.
  set_led(NUM_LOCK_LED, (keyboard_leds & 1) != 0);
  set_led(CAPS_LOCK_LED, (keyboard_leds & 2) != 0);
  set_led(SCROLL_LOCK_LED, (keyboard_leds & 4) != 0);
  set_led(COMPOSE_LED, (keyboard_leds & 8) != 0);
  set_led(KANA_LED, (keyboard_leds & 0x10) != 0);
}
