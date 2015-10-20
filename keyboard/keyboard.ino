#include <Keypad.h>
#define KEY_MODE 1
#define KEY_MACRO 2
#define KEY_SHIFT KEY_LEFT_SHIFT
#define KEY_CTRL KEY_LEFT_CTRL
#define KEY_VOLDN 0 // TODO
#define KEY_VOLUP 0 // TODO

// SETTINGS:

#define DEBOUNCE_MS 10
#define HOLD_MS 200
byte ROW_PINS[] = {0, 1, 2, 3, 4, 5};
byte COLUMN_PINS[] = {16, 17, 18, 19, 20, 21};
char KEYMAPS[][sizeof(ROW_PINS) * sizeof(COLUMN_PINS)] = {
  // https://cdn.rawgit.com/benshayden/github/da6ea8e/keyboard/keymaps.html
  {'z', 'b', 'm', 'h', KEY_BACKSPACE, 't', 'x', 'y', 'c', 'n', ' ', 'k', 'f', 'd', 'o', 'e', 'q', 'p', 'u', 's', KEY_ENTER, KEY_SHIFT, 'v', 'g', 'l', 'i', KEY_MODE, 'j', 'w', 'r', 'a', ';'},
  {'`', '[', '\'', KEY_UP, '=', '-', '|', '/', KEY_LEFT, KEY_RIGHT, KEY_CTRL, '\\', ']', '.', KEY_DOWN, ',', KEY_VOLDN, '9', '8', '7', KEY_TAB, KEY_SHIFT, KEY_VOLUP, '6', '5', '4', KEY_MODE, KEY_MACRO, '3', '2', '1', '0'}
};

// END SETTINGS

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
char mode = 0;
Keypad keypad(KEYMAPS[mode], ROW_PINS, COLUMN_PINS, sizeof(ROW_PINS), sizeof(COLUMN_PINS));
keypad.setDebounceTime(DEBOUNCE_MS);
keypad.setHoldTime(HOLD_MS);

void setup() {
  Keyboard.begin();
}

void loop() {
  if (!keypad.getKeys())
    return;
  for (int i = 0; i < LIST_MAX; ++i) {
    const Key& k = keypad.key[i];
    if (!k.stateChanged)
      continue;
    if (k.kstate == RELEASED) {
      Keyboard.release(k.kchar);
    } else if (k.kstate == PRESSED) {
      if (k.kchar == KEY_MODE) {
        mode = (mode + 1) % ARRAYSIZE(KEYMAPS);
        keypad.begin(KEYMAPS[mode]);
      } else {
        Keyboard.press(k.kchar);
      }
    }
  }
}
