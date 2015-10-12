#include <Keypad.h>

#define DEBOUNCE_MS 10
#define HOLD_MS 200
const char ROW_PINS[] = {0, 1, 2, 3, 4, 5};
const char COLUMN_PINS[] = {16, 17, 18, 19, 20, 21};
const char KEYMAP[][sizeof(ROW_PINS)][sizeof(COLUMN_PINS)] = {
#define KEY_MODE 1
  { // 26(a-z) + 1(;) + 5(space+backspace+enter+shift+mode)
    {},
  },
  { // 10(0-9) + 10(`-=[]\',./) + 4(arrows) + 3(shift+ctrl+mode) + 4(tab+volup+voldn+macro) + 1()
    {},
  },
};

// END SETTINGS

char mode = 0;
Keypad KEYPAD(makeKeymap(KEYMAP[mode]), ROW_PINS, COLUMN_PINS, sizeof(ROW_PINS), sizeof(COLUMN_PINS));
KEYPAD.setDebounceTime(DEBOUNCE_MS);
KEYPAD.setHoldTime(HOLD_MS);

void setup() {
  Keyboard.begin();
}

void loop() {
  KEYPAD.getKeys();
}

void onKeyChange(char k) {
  // KEYPAD.begin(makeKeymap(KEYMAP[mode]));
  if (KEYPAD.isPressed(k)) {
    Key e = KEYPAD.key[KEYPAD.findInList(k)];
    // e.kstate != HOLD
    Keyboard.press(k);
  } else {
    Keyboard.release(k);
  }
}
KEYPAD.addEventListener(onKeyChange);
