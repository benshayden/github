#include <Keypad.h>

// Many keyboard have a hundred keys or more,
// but they usually have even more functions.
// This keyboard only has 32 keys,
// but might still want to access all those hundred-plus functions.
// This is a compression problem.
// Many keyboards use mode keys to map multiple functions to each key,
// but, with only 32 buttons, a better compression algorithm is necessary.
// Use huffman.html to encode an arbitrary number of functions
// as sequences of button presses.
// Extremely common keys might map to a single button press.
// Most keys will require 2 or 3 buttons.
// Uncommon keys might require 4 or 5 button presses.
// The huffman state machine is advanced every time a Key on the keypad is PRESSED.
// If you press two buttons less than a millisecond apart, say |a| and |b|,
// then the state machine can't tell if you meant a-then-b or b-then-a,
// but it will always interpret the lower number first.
// The Huffman code tree is encoded in the HUFFMAN_TREE setting variable.
// Each row in this matrix represents an internal node in a 36-ary Huffman tree.
// The n-th element of a row corresponds to the n-th child of that internal node.
// If that element's value <= 0, then it is interpreted as a pointer to another internal node in the tree.
// HUFFMAN_TREE[0] is the root node.
// It is possible to create cycles. Don't.
// If the value > 0xffff, then it is a leaf node
// that will be interpreted as a function pointer, and called.
// You can define these functions to call Keyboard.press()
// or set internal variables or blink lights.
// If that element's value is between 0 and 0xffff, then it is a leaf node
// that will be interpreted as a keycode to be passed to Keyboard.press().
// If the strength of the weakest finger is less than half that of the strongest finger,
// then it might be preferable to use the strongest finger twice
// rather than the weakest finger once.
// In such a case, a variable-ary Huffman tree would increase the average sequence length,
// but also increase the comfort.

#define KEY_SHIFT KEY_LEFT_SHIFT
#define KEY_CTRL KEY_LEFT_CTRL
#define KEY_VOLDN 0 // TODO
#define KEY_VOLUP 0 // TODO

// SETTINGS:

#define DEBOUNCE_MS 10
#define HOLD_MS 200
byte ROW_PINS[] = {0, 1, 2, 3, 4, 5};
byte COLUMN_PINS[] = {16, 17, 18, 19, 20, 21};
char HUFFMAN_TREE[][sizeof(ROW_PINS) * sizeof(COLUMN_PINS)] = {
  // TODO huffman.html
};

// END SETTINGS

char KEYMAP[sizeof(ROW_PINS) * sizeof(COLUMN_PINS)] = {
  {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36}
};

#define ARRAYSIZE(a) (sizeof(a) / sizeof((a)[0]))
Keypad keypad(KEYMAP, ROW_PINS, COLUMN_PINS, sizeof(ROW_PINS), sizeof(COLUMN_PINS));
char* huffman_state = HUFFMAN[0];

char lookup(char kchar) {
  kchar = huffman_state[kchar];
  if (kchar <= 0) {
    huffman_state = HUFFMAN[-kchar];
    return 0;
  } else {
    huffman_state = HUFFMAN[0];
    return kchar;
  }
}

char current_values[LIST_MAX];
#define CALL(x) ((void(*)())(x))();

void setup() {
  for (int i = 0; i < LIST_MAX; ++i) current_values[i] = 0;
  keypad.setDebounceTime(DEBOUNCE_MS);
  keypad.setHoldTime(HOLD_MS);
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
      Keyboard.release(current_values[i]);
      current_values[i] = 0;
    } else if (k.kstate == PRESSED) {
      char kchar = lookup(k.char);
      if (!kchar) return;
      current_values[i] = kchar;
      if (kchar > 0xffff) {
        CALL(current_values[i]);
        return;
      }
      Keyboard.press(current_values[i]);
    } else if (k.kstate == HOLD) {
      if (current_values[i] > 0xffff) {
        CALL(current_values[i]);
      }
    }
  }
}
