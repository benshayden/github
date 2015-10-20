#include <Keypad.h>

#define DEBOUNCE_MS 10
#define HOLD_MS 200
const char ROW_PINS[] = {0, 1, 2, 3, 4, 5};
const char COLUMN_PINS[] = {16, 17, 18, 19, 20, 21};
const char KEYMAPS[][sizeof(ROW_PINS) * sizeof(COLUMN_PINS)] = {
#define KEY_MODE 1
  { // 26(a-z) + 1(;) + 5(space+backspace+enter+shift+mode)
    '',
  },
  { // 10(0-9) + 10(`-=[]\',./) + 4(arrows) + 3(shift+ctrl+mode) + 4(tab+volup+voldn+macro) + 1()
    '',
  },
};

// END SETTINGS

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
char mode = 0;
Keypad KEYPAD(KEYMAPS[mode], ROW_PINS, COLUMN_PINS, sizeof(ROW_PINS), sizeof(COLUMN_PINS));
KEYPAD.setDebounceTime(DEBOUNCE_MS);
KEYPAD.setHoldTime(HOLD_MS);

void setup() {
  Keyboard.begin();
}

void loop() {
  if (!KEYPAD.getKeys())
    return;
  for (int i = 0; i < LIST_MAX; ++i) {
    const Key& k = KEYPAD.key[i];
    if (!k.stateChanged)
      continue;
    if (k.kstate == RELEASED) {
      Keyboard.release(k.kchar);
    } else if (k.kstate == PRESSED) {
      if (k.kchar == KEY_MODE) {
        mode = (mode + 1) % ARRAYSIZE(KEYMAPS);
        KEYPAD.begin(KEYMAPS[mode]);
      } else {
        Keyboard.press(k.kchar);
      }
    }
  }
}
