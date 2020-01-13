#include <SPI.h>
#include <Wire.h>
#include "SD.h"
#include "Adafruit_TinyUSB.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_SleepyDog.h>
#include "list.h"

// Configuration variables depend on your board and display.
#define CHIP_SELECT 4
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

// https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
#include "Helvetica10pt7b.h"
#define SPRITZ_FONT &Helvetica10pt7b

// Platform
#define INACTIVITY_MS 5000
#define MS_PER_MIN 60000
#define WPM_FILENAME "~WPM.TXT"
#define PROGRESS_FILENAME "~PROGRES.TXT"
#define HAND_FILENAME "~LEFT.TXT"
#define PRESSED(btn) (!digitalRead(btn))
#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))
Adafruit_USBD_MSC usb_msc;
Sd2Card card;
SdVolume volume;
SdFile root;
#define INFO_(x) if (Serial) Serial.print(x)
#define POSTINFO if (Serial) Serial.println()
#define PREINFO INFO_(__FILE__); INFO_(":"); INFO_(__LINE__); INFO_("> ")
#define INFO1(x) PREINFO; INFO_(x); POSTINFO
#define INFO2(x, y) PREINFO; INFO_(x); INFO_(y); POSTINFO
#define INFO3(x, y, z) PREINFO; INFO_(x); INFO_(y); INFO_(z); POSTINFO
#define INFO4(w, x, y, z) PREINFO; INFO_(w); INFO_(x); INFO_(y); INFO_(z); POSTINFO
class Book {
 public:
  Book(String t, String fn) : title(t), filename(fn), position(0) {}
  String title;
  String filename;
  uint32_t position;
  friend bool operator> (const Book& a, const Book& b) {
    return a.title > b.title;
  }
  friend bool operator< (const Book& a, const Book& b) {
    return a.title < b.title;
  }
  friend bool operator== (const Book& a, const Book& b) {
    return a.title == b.title;
  }
  friend bool operator!= (const Book& a, const Book& b) {
    return a.title != b.title;
  }
};
#define is_punctuation(c) (c == '.' || c == '!' || c == '?')
#define is_nl(c) (c == '\n')
#define is_whitespace(c) ((c == ' ') || is_nl(c) || (c == '\t'))
bool sentence_end = false;
uint32_t sentence_words = 0;

// Application UI Model
enum class Screen {
  MENU,
  SETTINGS,
  WPM,
  HAND,
  READING,
  SPRITZ,
} screen = Screen::MENU;
List<Book*> books;
uint32_t reading_filename_index = 0;
bool left_hand = false;
uint32_t wpm = 300;
unsigned int activity_timestamp = 0;
SdFile bookf;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  setup_buttons();
  setup_msc();
  setup_wpm();
  setup_hand();
  setup_display();
  setup_books();
  setup_progress();
  INFO1(battery_volts());
  delay(1000);
  render();
  activity_timestamp = millis();
}

void setup_buttons() {
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
}

void setup_cursor() {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds("A", 0, 0, &x1, &y1, &w, &h);
  display.setCursor(0, 16 + (h / 2) - y1);
}

void setup_display() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.dim(true);
  display.setRotation(left_hand ? 0 : 2);
  display.setTextColor(SSD1306_WHITE);
  setup_cursor();
  display.setFont(SPRITZ_FONT);
  display.println("SpritzKey");
  display.display();
}

void setup_msc() {
  usb_msc.setID("bshayden", "Spritzkey", "0.1");
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
  usb_msc.setUnitReady(false);
  usb_msc.begin();

  Serial.begin(115200);
  unsigned long start = millis();
  while (!Serial && (3000 > (millis() - start)));

  if (!card.init(SPI_HALF_SPEED, CHIP_SELECT)) {
    INFO1("Unable to init card");
    // Is a card inserted? Is the wiring correct? Is CHIP_SELECT correct?
    while (1) delay(1);
  }

  if (!volume.init(card)) {
    Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
    while (1) delay(1);
  }

  uint32_t block_count = volume.blocksPerCluster()*volume.clusterCount();
  usb_msc.setCapacity(block_count, 512);
  usb_msc.setUnitReady(true);

  root.openRoot(volume);
}

void setup_wpm() {
  SdFile wpmf;
  if (!wpmf.open(root, WPM_FILENAME, O_READ)) {
    INFO1("no wpm file");
    return;
  }
  char buf[6];
  int bytes_read = wpmf.read(buf, ARRAYSIZE(buf));
  if (bytes_read <= 0) {
    INFO1("wpm file empty");
    return;
  }
  String s = String(buf);
  wpmf.close();
  long si = s.toInt();
  if (si) {
    wpm = si;
  } else {
    INFO1("toInt failed");
  }
  INFO2(wpm, " wpm");
}

void setup_hand() {
  SdFile handf;
  left_hand = !!handf.open(root, HAND_FILENAME, O_READ);
  INFO1(left_hand ? "left hand" : "right hand");
  if (left_hand) handf.close();
}

void setup_books() {
  uint8_t offset[] = {1, 3, 5, 7, 9, 14, 16, 18, 20, 22, 24, 28, 30};
  char name[13];
  char lfn[131];
  bool haveLong = false;
  dir_t dir;
  uint8_t i;
  uint8_t lfnIn = 130;
  uint8_t ndir;
  uint8_t sum;
  uint8_t test;

  root.rewind();
  while (root.read(&dir, 32) == 32) {
    if (DIR_IS_LONG_NAME(&dir)) {
      if (!haveLong) {
        if ((dir.name[0] & 0XE0) != 0X40) continue;
        ndir = dir.name[0] & 0X1F;
        test = dir.creationTimeTenths;
        haveLong = true;
        lfnIn = 130;
        lfn[lfnIn] = 0;
      } else if (dir.name[0] != --ndir || test != dir.creationTimeTenths) {
        haveLong = false;
        continue;
      }
      char *p = (char*)&dir;
      if (lfnIn > 0) {
        lfnIn -= 13;
        for (i = 0; i < 13; i++) {
          lfn[lfnIn + i] = p[offset[i]];
        }
      }
    } else if (DIR_IS_FILE_OR_SUBDIR(&dir)
      && dir.name[0] != DIR_NAME_DELETED
      && dir.name[0] != DIR_NAME_FREE) {
      if (haveLong) {
        for (sum = i = 0; i < 11; i++) {
           sum = (((sum & 1) << 7) | ((sum & 0xfe) >> 1)) + dir.name[i];
        }
        if (sum != test || ndir != 1) haveLong = false;
      }
      SdFile::dirName(dir, name);
      String str;
      if (haveLong) {
        str = lfn + lfnIn;
      } else if (dir.reservedNT) {
        str = name;
        str.toLowerCase();
      }
      if (str.length() > 0 && str.charAt(0) != '.' && str.charAt(0) != '~') {
        books.prepend(new Book(str, name));
      }
      haveLong = false;
    } else {
      if (dir.name[0] == DIR_NAME_FREE) break;
      haveLong = false;
    }
  }
  books.sort();
  reading_filename_index = 0;
}

void setup_progress() {
  SdFile pf;
  if (!pf.open(root, PROGRESS_FILENAME, O_READ)) {
    write_progress();
    return;
  }
  char line[300];
  uint16_t linei = 0;
  int16_t c = pf.read();
  while (c >= 0) {
    if (is_nl(c)) {
      set_progress(line, linei);
      linei = 0;
      memset(line, 0, ARRAYSIZE(line));
    } else {
      line[linei++] = (char)c;
      if (linei >= ARRAYSIZE(line)) linei = 0;
    }
    c = pf.read();
  }
  if (linei) set_progress(line, linei);
}

void set_progress(const char* line, uint16_t size) {
  for (uint16_t booki = 0; booki < books.length(); ++booki) {
    Book* book = books.get(booki);
    for (uint8_t ci = 0; ci < size && ci < book->title.length(); ++ci) {
      if (line[ci] != book->title.charAt(ci)) {
        book = nullptr;
        break;
      }
    }
    if (book == nullptr) continue;
    String offs(line + book->title.length() + 1);
    book->position = offs.toInt();
  }
  // Find book with title, set its progress
}

void write_progress() {
  SdFile pf;
  if (!pf.open(root, PROGRESS_FILENAME, O_WRITE | O_CREAT)) {
    INFO1("unable to open progress file for writing");
    return;
  }
  for (uint32_t i = 0; i < books.length(); ++i) {
    const Book* book = books.get(i);
    pf.print(book->title);
    pf.print(":");
    pf.print(book->position);
    pf.println();
  }
  pf.flush();
  pf.close();
}

float battery_volts() {
  return analogRead(A7) * 2 * 3.3 / 1024;
}

// Application UI View
void render() {
  display.clearDisplay();
  display.setRotation(left_hand ? 0 : 2);
  display.setFont();
  display.setCursor(0, 0);
  switch (screen) {
    case Screen::MENU:
      render_menu();
      break;
    case Screen::SETTINGS:
      render_settings();
      break;
    case Screen::WPM:
      render_wpm();
      break;
    case Screen::HAND:
      render_hand();
      break;
    case Screen::READING:
      render_reading();
      break;
    case Screen::SPRITZ:
      render_spritz();
      break;
  }
  display.display();
}

// Application UI Controller
bool prev_button_a = false;
bool prev_button_b = false;
bool prev_button_c = false;
void loop() {
  switch (screen) {
    case Screen::MENU:
      controller_menu();
      break;
    case Screen::SETTINGS:
      controller_settings();
      break;
    case Screen::WPM:
      controller_wpm();
      break;
    case Screen::HAND:
      controller_hand();
      break;
    case Screen::READING:
      controller_reading();
      break;
    case Screen::SPRITZ:
      controller_spritz();
      break;
  }

  prev_button_a = PRESSED(BUTTON_A);
  prev_button_b = PRESSED(BUTTON_B);
  prev_button_c = PRESSED(BUTTON_C);

  delay(loop_ms());

  if (!Serial && ((millis() - activity_timestamp) > INACTIVITY_MS)) {
    display.clearDisplay();
    display.display();
    while (!PRESSED(BUTTON_A) && !PRESSED(BUTTON_B) && !PRESSED(BUTTON_C)) {
      Watchdog.sleep(128);
    }
  }
}

unsigned int loop_ms() {
  if (screen == Screen::SPRITZ) {
    uint32_t ms = MS_PER_MIN / wpm;
    return (sentence_end && (sentence_words > 1)) ? (ms * 2) : ms;
  }
  return 200;
}

// Evaluates to 1 or -1 depending on left_hand and which of buttons A or C is
// pressed.
#define BTN_HAND_SGN ((PRESSED(BUTTON_A) ? 1 : -1) * (left_hand ? 1 : -1))

void render_menu() {
  bookf.close();
  uint32_t num_options = books.length() + 1;
  for (uint32_t di = 0; di < min(num_options, 4); ++di) {
    uint32_t x = (reading_filename_index - (reading_filename_index % 4) + di + num_options - 1) % num_options;
    display.print(((x == reading_filename_index) || (num_options == 1)) ? ">" : " ");
    if (x == books.length()) {
      display.println("|Settings|");
    } else {
      display.println(books.get(x)->title);
    }
  }
}
void controller_menu() {
  if (!PRESSED(BUTTON_A) && !PRESSED(BUTTON_B) && !PRESSED(BUTTON_C)) return;
  activity_timestamp = millis();
  if (PRESSED(BUTTON_B) && !prev_button_b) {
    if (reading_filename_index == books.length()) {
      screen = Screen::SETTINGS;
    } else {
      const Book* book = books.get(reading_filename_index);
      if (!bookf.open(root, book->filename.c_str(), O_READ)) {
        INFO1("Unable to open");
        return;
      }
      screen = Screen::READING;
      if (book->position < bookf.fileSize()) {
        bookf.seekSet(book->position);
      }
    }
  } else {
    uint32_t num_options = books.length() + 1;
    reading_filename_index = (reading_filename_index + num_options - BTN_HAND_SGN) % num_options;
  }
  render();
}

struct Settings {
  Screen screen;
  const char* label;
} SETTINGS[] = {{Screen::MENU, "back"}, {Screen::WPM, "words per minute"}, {Screen::HAND, "hand"}};
uint32_t settings_index = 0;
void render_settings() {
  for (uint32_t i = 0; i < ARRAYSIZE(SETTINGS); ++i) {
    display.print((settings_index == i) ? ">" : " ");
    display.println(SETTINGS[i].label);
  }
}
void controller_settings() {
  if (!PRESSED(BUTTON_A) && !PRESSED(BUTTON_B) && !PRESSED(BUTTON_C)) return;
  activity_timestamp = millis();
  if (PRESSED(BUTTON_B) && !prev_button_b) {
    screen = SETTINGS[settings_index].screen;
  } else {
    settings_index = (settings_index + ARRAYSIZE(SETTINGS) - BTN_HAND_SGN) % ARRAYSIZE(SETTINGS);
  }
  render();
}

void render_wpm() {
  display.println("words per minute");
  display.println(wpm);
}
void controller_wpm() {
  if (!PRESSED(BUTTON_A) && !PRESSED(BUTTON_B) && !PRESSED(BUTTON_C)) return;
  activity_timestamp = millis();

  if (PRESSED(BUTTON_B) && !prev_button_b) {
    screen = Screen::SETTINGS;
    render();

    SdFile wpmf;
    if (!wpmf.open(root, WPM_FILENAME, O_WRITE | O_CREAT)) {
      INFO1("unable to open wpm file for writing");
      return;
    }
    wpmf.println(wpm);
    wpmf.flush();
    wpmf.close();
    return;
  }

  int dwpm = 10 * BTN_HAND_SGN;
  if (((wpm <= 20) && (dwpm < 0)) ||
      ((wpm >= MS_PER_MIN) && (dwpm > 0))) {
    return;
  }

  wpm += dwpm;
  render();
}

void render_hand() {
  display.println(left_hand ? "Left" : "Right");
}
void controller_hand() {
  if (!PRESSED(BUTTON_A) && !PRESSED(BUTTON_B) && !PRESSED(BUTTON_C)) return;

  activity_timestamp = millis();

  if (PRESSED(BUTTON_B) && !prev_button_b) {
    screen = Screen::SETTINGS;
    if (left_hand) {
      SdFile handf;
      if (!handf.open(root, HAND_FILENAME, O_WRITE | O_CREAT)) {
        INFO1("unable to open hand file for writing");
      } else {
        handf.close();
      }
    } else {
      if (!SdFile::remove(&root, HAND_FILENAME)) {
        INFO1("unable to remove hand file");
      }
    }
  } else if ((PRESSED(BUTTON_A) && !prev_button_a) || (PRESSED(BUTTON_C) && !prev_button_c)) {
    left_hand = !left_hand;
  }
  render();
}

void render_reading() {
  // Display title and progress.
  const Book* book = books.get(reading_filename_index);
  display.println(book->title); // TODO scroll

  double frac = bookf.curPosition();
  frac /= bookf.fileSize();
  frac *= 100;
  uint8_t pct = floor(frac);
  display.print(pct);
  display.print(".");
  for (int i = 0; i < 4; ++i) {
    frac = (frac - pct) * 10;
    pct = floor(frac);
    display.print(pct);
  }
  display.println("%");

  frac = battery_volts();
  pct = floor(frac);
  display.print(pct);
  display.print(".");

  frac -= pct;
  frac *= 10;
  pct = floor(frac);
  display.print(pct);
  display.println("V");
}
void controller_reading() {
  if (PRESSED(BUTTON_B)) {
    // Wait a bit to see if this is a short click (back to menu) or a long press
    // (spritz).
    unsigned long start = millis();
    while (((millis() - start) < 400) && PRESSED(BUTTON_B)) {
      delay(10);
    }
    screen = PRESSED(BUTTON_B) ? Screen::SPRITZ : Screen::MENU;
    const Book* book = books.get(reading_filename_index);
    bookf.seekSet(book->position);
    render();
    activity_timestamp = millis();
    return;
  }

  if (!PRESSED(BUTTON_A) && !PRESSED(BUTTON_C)) {
    if (prev_button_a || prev_button_c) {
      // Finished skipping forward/backward, so save the new progress position.
      write_progress();
    }
    return;
  }

  activity_timestamp = millis();

  // Skip forward/backward a sentence.
  int dsi = BTN_HAND_SGN;
  int16_t c = bookf.read();
  if (dsi < 0) {
    while ((c >= 0) && !is_punctuation(c)) {
      bookf.seekSet(max(2, bookf.curPosition()) - 2);
      if (bookf.curPosition() == 0) break;
      c = bookf.read();
    }
    while ((c >= 0) && is_punctuation(c)) {
      bookf.seekSet(max(2, bookf.curPosition()) - 2);
      if (bookf.curPosition() == 0) break;
      c = bookf.read();
    }
    while ((c >= 0) && !is_punctuation(c)) {
      bookf.seekSet(max(2, bookf.curPosition()) - 2);
      if (bookf.curPosition() == 0) break;
      c = bookf.read();
    }
  } else {
    while ((c >= 0) && !is_punctuation(c)) {
      c = bookf.read();
    }
  }

  Book* book = books.get(reading_filename_index);
  book->position = bookf.curPosition();
  render();
}

void render_spritz() {
  setup_cursor();
  display.setFont(SPRITZ_FONT);

  if (sentence_end) sentence_words = 0;
  sentence_end = false;
  ++sentence_words;

  int16_t c = bookf.read();
  // Skip to the start of the next word.
  while (c >= 0 && (is_whitespace(c) || is_punctuation(c))) {
    c = bookf.read();
  }

  if (c < 0) {
    // end of file
    screen = Screen::READING;
    render();
    return;
  }

  // Display this word, and remember if this is the last word in the sentence.
  while (c >= 0 && !is_whitespace(c)) {
    if (is_punctuation(c)) sentence_end = true;
    display.print((char)c);
    c = bookf.read();
  }

  if (sentence_end) {
    // Finished a sentence, so update book->position.
    Book* book = books.get(reading_filename_index);
    book->position = bookf.curPosition();
  }
}
void controller_spritz() {
  activity_timestamp = millis();
  if (!PRESSED(BUTTON_B)) {
    screen = Screen::READING;
    render();
    write_progress();
    return;
  }

  if (!prev_button_b) {
    const Book* book = books.get(reading_filename_index);
    bookf.seekSet(book->position);
  }

  // Continue spritzing
  render();
}

int32_t msc_read_cb(uint32_t lba, void* buffer, uint32_t bufsize) {
  (void) bufsize;
  return card.readBlock(lba, (uint8_t*) buffer) ? 512 : -1;
}

int32_t msc_write_cb(uint32_t lba, uint8_t* buffer, uint32_t bufsize) {
  (void) bufsize;
  int32_t bytes_written = card.writeBlock(lba, buffer) ? 512 : -1;
  // TODO after timeout: setup_books();
  return bytes_written;
}

void msc_flush_cb (void) {
}

#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__
int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}
