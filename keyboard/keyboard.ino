#include <Keypad.h>

const char ROW_PINS[] = {0, 1, 2, 3, 4, 5};
const char COLUMN_PINS[] = {16, 17, 18, 19, 20, 21};
#define ROWS sizeof(ROW_PINS)
#define COLUMNS sizeof(COLUMN_PINS)
const char KEYMAP[][ROWS][COLUMNS] = {
  { // 26(a-z) + 1(;) + 5(space+backspace+shift+ctrl+mode)
    {},
  },
  { // 10(0-9) + 10(`-=[]\',./) + 4(arrows) + 8(shift+alt+ctrl+gui+mode+volup+voldn+macro)
    {},
  },
};

Keypad KEYPADS[] = {
  Keypad(makeKeymap(KEYMAP[0]), ROW_PINS, COLUMN_PINS, ROWS, COLUMNS),
  Keypad(makeKeymap(KEYMAP[1]), ROW_PINS, COLUMN_PINS, ROWS, COLUMNS),
};

// bitmasks for keyboard_report_data[1]
#define MEDIA_VOLUME_UP   0x01
#define MEDIA_VOLUME_DOWN 0x02
#define MEDIA_MUTE        0x04
#define MEDIA_PAUSE       0x08
#define MEDIA_NEXT        0x10
#define MEDIA_PREV        0x20
#define MEDIA_STOP        0x40
#define MEDIA_EJECT       0x80
#define MACRO_PRESS     0x8000

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
}
