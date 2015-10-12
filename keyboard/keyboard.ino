#include <Keypad.h>

#define DEBOUNCE_MS 10
#define HOLD_MS 200
const char ROW_PINS[] = {0, 1, 2, 3, 4, 5};
const char COLUMN_PINS[] = {16, 17, 18, 19, 20, 21};
const char KEYMAP[][sizeof(ROW_PINS)][sizeof(COLUMN_PINS)] = {
  { // 26(a-z) + 1(;) + 5(space+backspace+shift+ctrl+mode)
    {},
  },
  { // 10(0-9) + 10(`-=[]\',./) + 4(arrows) + 8(shift+alt+ctrl+gui+mode+volup+voldn)
    {},
  },
};

// END SETTINGS

// bitmasks for keyboard_report_data[1]
#define MEDIA_VOLUME_UP   0x01
#define MEDIA_VOLUME_DOWN 0x02
#define MEDIA_MUTE        0x04
#define MEDIA_PAUSE       0x08
#define MEDIA_NEXT        0x10
#define MEDIA_PREV        0x20
#define MEDIA_STOP        0x40
#define MEDIA_EJECT       0x80

bool is_reserved(char k) {
  // These keycodes are never sent to the host though they may be used
  // internally.
  return ((k < KEY_A) ||
          ((165 <= k) && (k <= 175)) ||
          (k == 222) ||
          (k == 223) ||
          ((232 <= k) && (k <= 255)));
}

Keypad KEYPAD(makeKeymap(KEYMAP[0]), ROW_PINS, COLUMN_PINS, sizeof(ROW_PINS), sizeof(COLUMN_PINS));
KEYPAD.setDebounceTime(DEBOUNCE_MS);
KEYPAD.setHoldTime(HOLD_MS);
void onKeyChange(char k);
KEYPAD.addEventListener(onKeyChange);

void setup() {
  Keyboard.begin();
}

void loop() {
  KEYPAD.getKeys();
}

void onKeyChange(char k) {
  // KEYPAD.begin(makeKeymap(KEYMAP[1]));
  if (KEYPAD.isPressed(k)) {
    if (!is_reserved(k)) {
      Keyboard.press(k);
    }
  } else {
    Keyboard.release(k);
  }
}
