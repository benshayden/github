// Spritzkey is an e-reader that displays one word at a time on a tiny OLED
// display. It reads books from an SD card. It enumerates as a flash storage
// drive when plugged into a USB host so you can copy text files containing
// books to its SD card.

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
  Book(String t, String fn) : title(t), filename(fn), num_sentences(0), sentence(0) {}
  String title;
  String filename;
  uint32_t num_sentences;
  uint32_t sentence;
  uint32_t remaining_mins() const {
    // TODO count words after current sentence
    return 0;
  }
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
class Sentence {
 public:
  Sentence(uint32_t o = 0, uint8_t w = 0) : offset(o), word_count(w) {}
  uint32_t offset;
  uint8_t word_count;
};

// https://learn.adafruit.com/adafruit-gfx-graphics-library/using-fonts
#include "FreeSerif7pt7b.h"
#define SPRITZ_FONT &FreeSerif7pt7b

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
List<Sentence*> sentences;
List<String> words;
bool left_hand = false;
uint32_t wpm = 300;
unsigned int activity_timestamp = 0;

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

void setup_display() {
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setRotation(left_hand ? 0 : 2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 5);
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
  SdFile f;
  if (!f.open(root, PROGRESS_FILENAME, O_READ)) {
    write_progress();
    return;
  }
  char buf[1 << 12];
  INFO1("TODO read progress file");
}

void write_progress() {
  SdFile f;
  if (!f.open(root, PROGRESS_FILENAME, O_WRITE | O_CREAT)) {
    INFO1("unable to open progress file for writing");
    return;
  }
  for (uint32_t i = 0; i < books.length(); ++i) {
    const Book* book = books.get(i);
    f.print(book->filename);
    f.print(":");
    f.print(book->sentence);
    f.print("/");
    f.print(book->num_sentences);
    f.println();
  }
  f.flush();
  f.close();
}

#define BOOK_BUF_SIZE (1 << 12)
#define sentence_ending(c) (c == '.' || c == '!' || c == '?')
#define word_ending(c) ((c == ' ') || (c == '\n') || (c == '\t'))
#define sentence_index books.get(reading_filename_index)->sentence

void read_book() {
  screen = Screen::READING;
  sentences.clear();
  SdFile book;
  if (!book.open(root, books.get(reading_filename_index)->filename.c_str(), O_READ)) {
    INFO1("Unable to open");
    return;
  }

  book.seekSet(0);
  Sentence* sentence = new Sentence();
  uint32_t block_i = 0;
  char prev_c = 0;
  while (true) {
    char buf[BOOK_BUF_SIZE];
    int bytes_read = book.read(buf, ARRAYSIZE(buf));
    for (uint32_t buf_i = 0; buf_i < min(bytes_read, BOOK_BUF_SIZE); ++buf_i) {
      uint32_t byte_i = buf_i + (block_i * BOOK_BUF_SIZE);
      char c = buf[buf_i];
      if (sentence_ending(c) && !sentence_ending(prev_c)) {
        INFO4("sentence ", sentence->offset, " ", sentence->word_count);
        sentences.append(sentence);
        sentence = new Sentence(byte_i);
      } else if ((word_ending(c) || ((bytes_read < BOOK_BUF_SIZE) && (buf_i == (bytes_read - 1)))) && !word_ending(prev_c)) {
        ++sentence->word_count;
      }
      prev_c = c;
    }
    if (bytes_read < BOOK_BUF_SIZE) {
      break;
    }
    ++block_i;
  }
  if (sentence->word_count) {
    sentences.append(sentence);
    INFO4("sentence ", sentence->offset, " ", sentence->word_count);
  }

  books.get(reading_filename_index)->num_sentences = sentences.length();
  read_sentence();
}

void read_sentence() {
  words.clear();
  SdFile book;
  if (!book.open(root, books.get(reading_filename_index)->filename.c_str(), O_READ)) {
    INFO1("Unable to open");
    return;
  }
  if (sentence_index >= sentences.length()) {
    INFO2("Not enough sentences", sentence_index);
    return;
  }
  if (!book.seekSet(sentences.get(sentence_index)->offset)) {
    INFO1("Unable to seek");
    return;
  }

  bool sentence_ended = false;
  String word;
  char prev_c = 0;
  while (true) {
    char buf[BOOK_BUF_SIZE];
    int bytes_read = book.read(buf, ARRAYSIZE(buf));
    for (uint32_t buf_i = 0; buf_i < min(bytes_read, BOOK_BUF_SIZE); ++buf_i) {
      char c = buf[buf_i];
      if (!word_ending(c)) {
        word += c;
      } else if (!word_ending(prev_c)) {
        INFO2("word ", word);
        words.append(word);
        if (sentence_ended) return;
        word = "";
      }
      if (sentence_ending(c)) {
        sentence_ended = true;
      }
      prev_c = c;
    }
    if (bytes_read < BOOK_BUF_SIZE) {
      break;
    }
  }
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
  switch (screen) {
    case Screen::SPRITZ:
      return MS_PER_MIN / wpm;
    default:
      return 200;
  }
}

// Evaluates to 1 or -1 depending on left_hand and which of buttons A or C is
// pressed.
#define BTN_HAND_SGN ((PRESSED(BUTTON_A) ? 1 : -1) * (left_hand ? 1 : -1))

void render_menu() {
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
      read_book();
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
  display.println();
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
  const Book* book = books.get(reading_filename_index);
  display.println(book->title); // TODO scroll
  display.print(book->sentence);
  display.print(" / ");
  display.print(book->num_sentences);
  display.print(" = ");
  uint32_t r = (book->sentence * 10000) / book->num_sentences;
  display.print(r / 100);
  display.print(".");
  display.print(r % 100);
  display.println("%");

  uint32_t mins = book->remaining_mins();
  if (mins >= 60) {
    display.print(mins / 60);
    display.print("h");
  }
  display.print(mins % 60);
  display.println("m remaining");
  // TODO scroll current sentence
  // TODO spritz definition of previously spritzed word
}
void controller_reading() {
  if (PRESSED(BUTTON_B)) {
    unsigned long start = millis();
    while (((millis() - start) < 400) && PRESSED(BUTTON_B)) {
      delay(10);
    }
    screen = PRESSED(BUTTON_B) ? Screen::SPRITZ : Screen::MENU;
    render();
    activity_timestamp = millis();
    return;
  }

  if (!PRESSED(BUTTON_A) && !PRESSED(BUTTON_C)) return;

  activity_timestamp = millis();

  int dsi = BTN_HAND_SGN;
  if (((sentence_index == 0) && (dsi < 0)) ||
      ((sentence_index == sentences.length()) && (dsi > 0))) {
    return;
  }

  sentence_index += dsi;
  render();
}

void render_spritz() {
  display.setFont(SPRITZ_FONT);
  display.setCursor(0, 10);
  if (words.length() < 1) {
    ++sentence_index;
    if (sentence_index >= sentences.length()) {
      screen = Screen::READING;
      render();
    } else {
      read_sentence();
    }
  }
  String word = words.pop();
  display.println(word);
}
void controller_spritz() {
  activity_timestamp = millis();
  if (!PRESSED(BUTTON_B)) {
    screen = Screen::READING;
    render();
    return;
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
