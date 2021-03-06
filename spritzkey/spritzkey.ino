#include <SPI.h>
#include <Wire.h>
#include "SD.h"
#include "Adafruit_TinyUSB.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "LowPower.h"
#include "list.h"

// Configuration variables depend on your board and display.
#define CHIP_SELECT 4
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5
#define BATTERY_PIN A7
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

// https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
#include "Helvetica8pt7b.h"
#define MENU_FONT &Helvetica8pt7b
#include "Helvetica10pt7b.h"
#define SPRITZ_FONT &Helvetica10pt7b

// Platform
#define INACTIVITY_MS 5000
#define MS_PER_MIN 60000
#define WPM_FILENAME "~WPM.TXT"
#define PROGRESS_FILENAME "~PROGRES.TXT"
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
uint8_t word_chars = 0;
uint16_t reticle_x = 0;

// Application UI Model
enum class Screen {
  MENU,
  WPM,
  READING,
  SPRITZ,
} screen = Screen::MENU;
List<Book*> books;
uint32_t reading_filename_index = 0;
float wpm = 300.0;
unsigned int activity_timestamp = 0;
unsigned int seeking_timestamp = 0;
SdFile bookf;

void setup() {
  setup_buttons();
  setup_display();
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  setup_msc();
  setup_wpm();
  setup_books();
  setup_progress();

  // This is the x-position of the reticle, where the user's eye will be
  // looking. render_spritz() may put about 2.5 letters to the left of the
  // reticle.
  reticle_x = getCharWidth('a');
  INFO2("width a ", reticle_x);
  reticle_x += (reticle_x / 2) + getCharWidth('A');
  INFO2("reticle_x ", reticle_x);
  INFO2("battery_volts ", battery_volts());
  INFO2("free_memory ", free_memory());
  delay(2 * MS_PER_MIN / wpm);
  render();
  activity_timestamp = millis();
}

void setup_buttons() {
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
}

uint16_t getCharWidth(char c) {
  char t = c;
  return getTextWidth(&t);
}
uint16_t getTextWidth(const char* c) {
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(c, 0, 0, &x1, &y1, &w, &h);
  return w;
}

void setup_cursor() {
  // NB setFont() adjusts cursor_y when switching between the default font and a
  // custom font, so be consistent about whether this is called before or after
  // setFont().
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds("A", 0, 0, &x1, &y1, &w, &h);
  display.setCursor(0, (display.height() / 2) + (h / 2) - y1 - 4);
}

void setup_display() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.dim(true);
  display.setRotation(2);
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

  if (PRESSED(BUTTON_A) && PRESSED(BUTTON_C)) {
    Serial.begin(115200);
    while (!Serial) delay(10);
    digitalWrite(LED_BUILTIN, HIGH);
  }

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
  INFO2("wpm ", wpm);
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
  while (analogRead(BATTERY_PIN) < 10) delay(1);
  return analogRead(BATTERY_PIN) * 2 * 3.3 / 1024;
}

uint8_t battery_percent() {
#define MIN_VOLTS 3.4
#define MAX_VOLTS 4.22
#define NORMALIZE(x, fromMin, fromMax, toMin, toMax) ((((toMax) - (toMin)) * ((x) - (fromMin)) / ((fromMax) - (fromMin))) + (toMin))
  return round(NORMALIZE(constrain(battery_volts(), MIN_VOLTS, MAX_VOLTS), MIN_VOLTS, MAX_VOLTS, 0.0, 100.0));
}

// Application UI View
void render() {
  display.clearDisplay();
  display.setRotation(2);
  display.setFont();
  display.setCursor(0, 0);
  switch (screen) {
    case Screen::MENU:
      render_menu();
      break;
    case Screen::WPM:
      render_wpm();
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
    case Screen::WPM:
      controller_wpm();
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

  // TODO Does a library mess with these pins or something?
  setup_buttons();

  delay(loop_ms());

  if (!Serial && ((millis() - activity_timestamp) > INACTIVITY_MS)) {
    sleepUntilB();
  }
}

void sleepUntilB() {
  display.clearDisplay();
  display.display();
  attachInterrupt(BUTTON_B, handleInterruptB, LOW);
  LowPower.standby();
  detachInterrupt(BUTTON_B);
  render();
  while (PRESSED(BUTTON_B)) delay(10);
  activity_timestamp = millis();
}

void handleInterruptB(void) {
}

#define CHAR_ADJUST 5.0
#define AVG_WORD_LENGTH 5.0
unsigned int loop_ms() {
  float ms = 200.0;
  if (screen == Screen::SPRITZ) {
    ms = MS_PER_MIN / wpm;

    // Hold the ends of long sentences a bit longer.
    if (sentence_end && (sentence_words > 2)) ms *= 2.0;

    // Hold long words longer than shorter words.
    // https://www.desmos.com/calculator/m0befj6uhr
    ms *= (max(2, word_chars) + CHAR_ADJUST) / (AVG_WORD_LENGTH + CHAR_ADJUST);
  } else if (screen == Screen::READING && seeking_timestamp != 0) {
    float dms = millis() - seeking_timestamp;
    // Double speed about every 2s while seeking through sentences.
    ms *= exp(-dms / 2900);
  }
  return round(ms);
}

// Evaluates to 1 or -1 depending on which of buttons A or C is pressed.
#define BUTTON_SIGN (PRESSED(BUTTON_A) ? -1 : 1)

void render_menu() {
  bookf.close();
  uint32_t num_options = books.length() + 1;
  for (uint32_t di = 0; di < min(num_options, 4); ++di) {
    uint32_t x = (reading_filename_index - (reading_filename_index % 4) + di + num_options - 1) % num_options;
    display.print(((x == reading_filename_index) || (num_options == 1)) ? ">" : " ");
    if (x == books.length()) {
      display.print(":wpm: ");
      display.println((int)round(wpm));
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
      screen = Screen::WPM;
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
    reading_filename_index = (reading_filename_index + num_options - BUTTON_SIGN) % num_options;
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
    screen = Screen::MENU;
    render();

    SdFile wpmf;
    if (!wpmf.open(root, WPM_FILENAME, O_WRITE | O_CREAT)) {
      INFO1("unable to open wpm file for writing");
      return;
    }
    wpmf.println((unsigned int)wpm);
    wpmf.flush();
    wpmf.close();
    return;
  }

  int dwpm = 10 * BUTTON_SIGN;
  if (((wpm <= 20) && (dwpm < 0)) ||
      ((wpm >= MS_PER_MIN) && (dwpm > 0))) {
    return;
  }

  wpm += dwpm;
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

  display.print((int)floor(battery_percent()));
  display.println("%");
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

    if (screen == Screen::SPRITZ) {
      flash_reticle();
    }

    const Book* book = books.get(reading_filename_index);
    bookf.seekSet(book->position);

    render();
    activity_timestamp = millis();
    return;
  }

  if (!PRESSED(BUTTON_A) && !PRESSED(BUTTON_C)) {
    seeking_timestamp = 0;
    if (prev_button_a || prev_button_c) {
      // Finished skipping forward/backward, so save the new progress position.
      write_progress();
    }
    return;
  }

  activity_timestamp = millis();
  if (seeking_timestamp == 0) seeking_timestamp = millis();

  // Skip forward/backward a sentence.
  int16_t c = bookf.read();
  if (BUTTON_SIGN < 0) {
    for (int i = 0; i < 3; ++i) {
      while ((c >= 0) && is_punctuation(c)) {
        bookf.seekSet(max(2, bookf.curPosition()) - 2);
        if (bookf.curPosition() > 0) break;
        c = bookf.read();
      }
      while ((c >= 0) && !is_punctuation(c)) {
        bookf.seekSet(max(2, bookf.curPosition()) - 2);
        if (bookf.curPosition() > 0) break;
        c = bookf.read();
      }
    }
  } else {
    while ((c >= 0) && is_punctuation(c)) {
      c = bookf.read();
    }
    while ((c >= 0) && !is_punctuation(c)) {
      c = bookf.read();
    }
  }

  Book* book = books.get(reading_filename_index);
  book->position = bookf.curPosition();
  render();
}

#define RETICLE_PX 4
void render_reticle() {
  display.drawFastVLine(reticle_x, 0, RETICLE_PX, SSD1306_WHITE);
  display.drawFastVLine(reticle_x, display.height() - RETICLE_PX, RETICLE_PX, SSD1306_WHITE);
}
void flash_reticle() {
  display.clearDisplay();
  display.setRotation(2);
  display.drawFastVLine(reticle_x, 0, display.height(), SSD1306_WHITE);
  display.drawFastHLine(0, display.height() / 2, display.width(), SSD1306_WHITE);
  display.display();
  delay(round(MS_PER_MIN / wpm));
}
void render_spritz() {
  render_reticle();
  setup_cursor();
  display.setFont(SPRITZ_FONT);

  if (sentence_end) sentence_words = 0;
  sentence_end = false;
  ++sentence_words;
  word_chars = 0;

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

  // Read the next word into a buffer before displaying it so that it can be
  // positioned under the reticle.
  char buf[20];

  // Display this word, and remember if this is the last word in the sentence.
  while ((c >= 0) && !is_whitespace(c) && (word_chars < ARRAYSIZE(buf))) {
    if (is_punctuation(c)) sentence_end = true;
    buf[word_chars++] = c;

    // Split hyphenated words, but display the hyphen unlike whitespace.
    if (c == '-') break;

    c = bookf.read();
  }
  buf[word_chars] = 0;

  // Position the word so that the first, second, or third letter is in the
  // reticle, where the user is already looking, depending on the length of the
  // word.
  int16_t left = reticle_x;
  if (word_chars >= 5) {
    left -= (getCharWidth(buf[0]) + getCharWidth(buf[1]) + (getCharWidth(buf[2]) / 2));
  } else if (word_chars >= 3) {
    left -= (getCharWidth(buf[0]) + (getCharWidth(buf[1]) / 2));
  } else  {
    left -= (getCharWidth(buf[0]) / 2);
  }

  // reticle_x is based on a typical char width, but some chars may be wider,
  // which cause left to go negative.
  if (left < 0) left = 0;

  if (display.width() < (getTextWidth(buf) + left)) {
    // Shorten the word so it will fit on the screen, and display the rest of
    // the word in the next frame. A better hyphenation algorithm might consider
    // sounds or word roots.
    INFO2((getTextWidth(buf) + left), buf);
    while (display.width() < (getTextWidth(buf) + left)) {
      buf[--word_chars] = 0;
      bookf.seekSet(bookf.curPosition() - 1);
    }
    buf[--word_chars] = '-';
    bookf.seekSet(bookf.curPosition() - 1);
  }

  display.setCursor(left, display.getCursorY());
  display.print(buf);

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

void msc_flush_cb(void) {
}

#ifdef __arm__
extern "C" char* sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif  // __arm__
int free_memory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}
