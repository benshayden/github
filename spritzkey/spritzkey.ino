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

// Configuration variables depend on your board and display.
#define CHIP_SELECT 4
#define BUTTON_A  9
#define BUTTON_B  6
#define BUTTON_C  5
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

#define PRESSED(btn) (!digitalRead(btn))
Adafruit_USBD_MSC usb_msc;
Sd2Card card;
SdVolume volume;
SdFile root;

template<class T>
class List {
 private:
  class ListNode {
   public:
    T value;
    ListNode* next;
    ListNode(T v, ListNode* n) : value(v), next(n) {}
  };
  uint32_t length_;
  ListNode* head_;

 public:
  List() : length_(0), head_(nullptr) {}

  void prepend(T element) {
    ++length_;
    head_ = new ListNode(element, head_);
  }

  uint32_t length() { return length_; }

  T get(uint32_t index) {
    ListNode* node = head_;
    while (index) {
      --index;
      node = node->next;
    }
    return node->value;
  }

  T pop() {
    ListNode* node = head_;
    T value = node->value;
    head_ = node->next;
    delete node;
    return value;
  }

  void sort() {
    if (length_ < 2) return;
    for (uint32_t pass = 0; pass < length_ - 1; ++pass) {
      bool sorted = true;
      ListNode* current = head_;
      for (uint32_t index = 0; index < length_ - pass - 1; ++index) {
        if (current->value > current->next->value) {
          sorted = false;
          T temp = current->value;
          current->value = current->next->value;
          current->next->value = temp;
        }
        current = current->next;
      }
      if (sorted) break;
    }
  }
};

List<String> library;
uint32_t reading_filename_index = 0;
List<uint32_t> sentences;
List<String> words;

void setup() {
  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setRotation(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.display();

  usb_msc.setID("bshayden", "Spritzkey", "0.1");
  usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
  usb_msc.setUnitReady(false);
  usb_msc.begin();

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
  make_library();
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
      if (haveLong) {
        String str = lfn + lfnIn;
        library.prepend(str);
      } else if (dir.reservedNT) {
        String str = name;
        str.toLowerCase();
        library.prepend(str);
      }
      haveLong = false;
    } else {
      if (dir.name[0] == DIR_NAME_FREE) break;
      haveLong = false;
    }
  }
  library.sort();
  if (library.length() == 1) reading_filename_index = 0;
}

void render() {
  if (reading_filename_index == library.length()) {
    display_menu();
  } else if (PRESSED(BUTTON_B)) {
    spritz();
  } else {
    display_reading();
  }
}

void display_menu() {
  display.clearDisplay();
  display.setCursor(0, 0);
  // TODO
  for (uint32_t i = 0; i < min(library.length(), 3); ++i) {
    display.println(library.get(i));
  }
  display.display();
}

void display_reading() {
  display.clearDisplay();
  display.setCursor(0, 0);
  // TODO scroll library.get(reading_filename_index);
  // TODO print sentence_index / sentences.length() = %f%%
  // TODO print HHhMMm remaining
  // TODO scroll current sentence
  // TODO spritz definition of previously spritzed word
  display.display();
}

void spritz() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println();
  display.println(words.pop());
  display.display();
}

void loop() {
  if (!digitalRead(BUTTON_A)) {
    display.print("A");
    display.display();
    Serial.println("A");
  }
  if (!digitalRead(BUTTON_B)) {
    display.print("B");
    display.display();
    Serial.println("B");
  }
  if (!digitalRead(BUTTON_C)) {
    display.print("C");
    display.display();
    Serial.println("C");
  }
  delay(10);
  // Watchdog.sleep(128);
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
