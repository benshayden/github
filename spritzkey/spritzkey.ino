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
#define INFO_(x) Serial.print(x)
#define POSTINFO Serial.println()
#define PREINFO INFO_(__FILE__); INFO_(":"); INFO_(__LINE__); INFO_("> ")
#define INFO1(x) PREINFO; INFO_(x); POSTINFO
#define INFO2(x, y) PREINFO; INFO_(x); INFO_(y); POSTINFO
#define INFO3(x, y, z) PREINFO; INFO_(x); INFO_(y); INFO_(z); POSTINFO

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
List<String> library;
uint32_t reading_filename_index = 0;
List<uint32_t> sentence_offsets;
uint32_t sentence_index = 0;
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
  make_library();
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

  // TODO does this preclude charging from wall wart?
  Serial.begin(115200);
  while (!Serial) delay(10);

  if (!card.init(SPI_HALF_SPEED, CHIP_SELECT)) {
    Serial.println("initialization failed. Things to check:");
    Serial.println("* is a card inserted?");
    Serial.println("* is your wiring correct?");
    Serial.println("* did you change the CHIP_SELECT pin to match your shield or module?");
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
  INFO1(s);
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

void make_library() {
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
      if (str.length() > 0 && str.charAt(0) != '.') {
        library.prepend(str);
      }
      haveLong = false;
    } else {
      if (dir.name[0] == DIR_NAME_FREE) break;
      haveLong = false;
    }
  }
  library.sort();
  //for (uint32_t i = 0; i < library.length(); ++i) INFO1(library.get(i));
  reading_filename_index = 0;
  if (library.length() == 1) {
    //read_book();
  }
}

void read_book() {
  screen = Screen::READING;
  sentence_index = 0;
  // TODO build sentence_offsets and words in a scheduler task
  // TODO read sentence_index from .PROGRES.TXT
}

void read_progress() {
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
  uint32_t num_options = library.length() + 1;
  for (uint32_t di = 0; di < min(num_options, 4); ++di) {
    uint32_t x = (reading_filename_index + di + num_options - 1) % num_options;
    display.print(((x == reading_filename_index) || (num_options == 1)) ? ">" : " ");
    if (x == library.length()) {
      display.println("|Settings|");
    } else {
      display.println(library.get(x));
    }
  }
}
void controller_menu() {
  if (!PRESSED(BUTTON_A) && !PRESSED(BUTTON_B) && !PRESSED(BUTTON_C)) return;
  activity_timestamp = millis();
  if (PRESSED(BUTTON_B) && !prev_button_b) {
    if (reading_filename_index == library.length()) {
      screen = Screen::SETTINGS;
    } else {
      read_book();
    }
  } else {
    uint32_t num_options = library.length() + 1;
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
  display.println("TODO render_reading");
  // TODO scroll library.get(reading_filename_index);
  // TODO print sentence_index / sentence_offsets.length() = %f%%
  // TODO print HHhMMm remaining
  // TODO scroll current sentence
  // TODO spritz definition of previously spritzed word
}
void controller_reading() {
  if (PRESSED(BUTTON_B)) {
    // TODO if momentary press, screen = menu
    // TODO if hold, screen = spritz
    render();
    activity_timestamp = millis();
    return;
  }

  if (!PRESSED(BUTTON_A) && !PRESSED(BUTTON_C)) return;

  activity_timestamp = millis();

  int dsi = BTN_HAND_SGN;
  if (((sentence_index == 0) && (dsi < 0)) ||
      ((sentence_index == sentence_offsets.length()) && (dsi > 0))) {
    return;
  }

  sentence_index += dsi;
  render();
}

void render_spritz() {
  display.setFont(SPRITZ_FONT);
  display.setCursor(0, 5);
  display.println(words.pop());
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
  // TODO after timeout: make_library();
  return bytes_written;
}

void msc_flush_cb (void) {
}
