#include "arclino_test.h"
#include "Arduino.h"

#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void setup();
void loop();

namespace arclino_test {

DEMANGLE_CC

bool func_name(void* fp, char* buf) {
  void* fps[] = {fp};
  char** strs = backtrace_symbols(fps, 1);
  if (!strs)
    return false;
  char* op = strchr(strs[0], '(');
  char* cp = strchr(strs[0], ')');
  if (!op || !cp)
    return false;
  char mangled[256], demangled[256];
  memset(mangled, 0, sizeof(mangled));
  memset(demangled, 0, sizeof(demangled));
  if (cp-op<=3)
    return false;
  strncpy(mangled, op + 1, cp-op-3);
  char* plus = strrchr(mangled, '+');
  if (plus) *plus = 0;
  if (!Demangle(mangled, demangled, sizeof(demangled)))
    return false;
  op = strchr(demangled, '(');
  if (!op)
    return false;
  *op = 0;
  strcpy(buf, demangled);
  return true;
}

int CaughtEvent(const Event& evt);

Event& Event::From(const char* fn, int line) {
  filename = const_cast<char*>(fn);
  lineno = line;
  return *this;
}
Event& Event::Caught() {
  caught = true;
  return *this;
}
Event& Event::Ignore() {
  ignore = true;
  return *this;
}

Event& Event::operator[](int ret) {
  returns = ret;
  return *this;
}

int Event::operator()(void* t) {
  type = t;
  return caught ? CaughtEvent(*this) : 0;
}
int Event::operator()(void* t, int a0) {
  type = t;
  args.push_back(a0);
  return caught ? CaughtEvent(*this) : 0;
}
int Event::operator()(void* t, int a0, int a1) {
  type = t;
  args.push_back(a0);
  args.push_back(a1);
  return caught ? CaughtEvent(*this) : 0;
}
int Event::operator()(void* t, int a0, int a1, int a2) {
  type = t;
  args.push_back(a0);
  args.push_back(a1);
  args.push_back(a2);
  return caught ? CaughtEvent(*this) : 0;
}
int Event::operator()(void* t, int a0, int a1, int a2, int a3) {
  type = t;
  args.push_back(a0);
  args.push_back(a1);
  args.push_back(a2);
  args.push_back(a3);
  return caught ? CaughtEvent(*this) : 0;
}
int Event::operator()(void* t, int a0, int a1, int a2, int a3, int a4) {
  type = t;
  args.push_back(a0);
  args.push_back(a1);
  args.push_back(a2);
  args.push_back(a3);
  args.push_back(a4);
  return caught ? CaughtEvent(*this) : 0;
}
int Event::operator()(void* t, int a0, int a1, int a2, int a3, int a4, int a5) {
  type = t;
  args.push_back(a0);
  args.push_back(a1);
  args.push_back(a2);
  args.push_back(a3);
  args.push_back(a4);
  args.push_back(a5);
  return caught ? CaughtEvent(*this) : 0;
}
int Event::operator()(void* t, int a0, int a1, int a2, int a3, int a4, int a5, int a6) {
  type = t;
  args.push_back(a0);
  args.push_back(a1);
  args.push_back(a2);
  args.push_back(a3);
  args.push_back(a4);
  args.push_back(a5);
  args.push_back(a6);
  return caught ? CaughtEvent(*this) : 0;
}

Event::Event() : type(0), returns(0), filename(NULL), lineno(0), ignore(false), caught(false) {}
Event::~Event() {}

int Event::Get(int argi, int def=0) const {
  return (argi >= args.size()) ? def : args[argi];
}
bool Event::Matches(const Event& that) const {
  if (type != that.type) return false;
  for (size_t i = 0; i < min(args.size(), that.args.size()); ++i) {
    if (args[i] != that.args[i])
      return false;
  }
  return true;
}
void Event::Format(char* buf, int bufsize) const {
  memset(buf, 0, bufsize);
  if (!Formatter::Format(*this, buf)) {
    if (!func_name(type, buf)) {
      sprintf(buf, "_unknown_");
    }
    if (strlen(buf) > bufsize - 3)
      return;
    *(buf + strlen(buf)) = '(';
    for (int argi = 0; argi < args.size(); ++argi) {
      if (strlen(buf) > bufsize - (3 + round(log10(args[argi]))))
        break;
      if (argi > 0) {
        sprintf(buf + strlen(buf), ", ");
      }
      sprintf(buf + strlen(buf), "%d", args[argi]);
    }
    if (strlen(buf) > bufsize - 2)
      return;
    *(buf + strlen(buf)) = ')';
  }
}

bool Event::Formatter::Format(const Event& evt, char* buf) {
  Formatter* current = list_;
  while (current) {
    if (current->type_ == evt.type) {
      (*(current->fp_))(buf, evt);
      return true;
    }
    current = current->next_;
  }
  return false;
}

Event::Formatter::Formatter(void* type, FunType fp)
    : next_(list_),
      type_(type),
      fp_(fp) {
  list_ = this;
}
Event::Formatter::~Formatter() {}

Event::Formatter* Event::Formatter::list_ = NULL;

int TestCase::Run(int argc, char* argv[]) {
  if (TestCase::caselist == NULL) {
    return 1;
  }
  TestCase* c = TestCase::caselist;
  while (c) {
    if (!c->Run()) {
      return 1;
    }
    c = c->next_;
  }
  return 0;
}

int TestCase::Caught(const Event& caught) {
  for (size_t ignore_i = 0; ignore_i < ignoring_.size(); ++ignore_i) {
    if (caught.Matches(ignoring_[ignore_i]))
      return ignoring_[ignore_i].returns;
  }
  if (!success_) {
    if (failpeek_count_ < FAILPEEK_LIMIT) {
      char buf[256];
      caught.Format(buf, sizeof(buf));
      printf("  %s\n", buf);
      ++failpeek_count_;
      if (failpeek_count_ == FAILPEEK_LIMIT) {
        running_ = false;
      }
    }
    return 0;
  }
  if (expectation_index_ >= expectations_.size()) {
    running_ = false;
    return 0;
  }
  const Event& expected = expectations_[expectation_index_];
  ++expectation_index_;
  eat_ignores();
  for (size_t ignore_i = 0; ignore_i < ignoring_.size(); ++ignore_i) {
    if (expectations_[expectation_index_].Matches(ignoring_[ignore_i])) {
      ignoring_.erase(ignoring_.begin() + ignore_i);
      --ignore_i;
    }
  }
  if (expectation_index_ == expectations_.size()) {
    running_ = false;
  }
  if (!caught.Matches(expected)) {
    char ebuf[256], cbuf[256];
    expected.Format(ebuf, sizeof(ebuf));
    caught.Format(cbuf, sizeof(cbuf));
    printf("Expected %s from %s:%d\nCaught %s\n", ebuf, expected.filename, expected.lineno, cbuf);
    success_ = false;
    void* fps[100];
    int nfps = backtrace(fps, sizeof(fps));
    printf("Traceback:\n");
    char name[256];
    for (int stri = nfps - 4; stri > 0; --stri) {
      memset(name, 0, sizeof(name));
      if (func_name(fps[stri], name)) {
        static const char ignore[] = "arclino_test::";
        if (strncmp(name, ignore, strlen(ignore))) {
          printf("  %s\n", name);
        }
      }
    }
    if (FAILPEEK_LIMIT == 0) {
      running_ = false;
    } else {
      printf("Failpeek %d:\n", FAILPEEK_LIMIT);
    }
    return 0;
  }
  return expected.returns;
}

TestCase::TestCase(const char* name, ExpectationsFunction expectations)
    : name_(name),
      next_(TestCase::caselist),
      expectations_fp_(expectations) {
  TestCase::caselist = this;
}

bool TestCase::Run() {
  printf("Running %s\n", name_);
  expectation_index_ = 0;
  expectations_fp_(&expectations_);
  current_ = this;
  running_ = true;
  success_ = true;
  failpeek_count_ = 0;
  eat_ignores();

  if (expectation_index_ < expectations_.size()) {
    setup();
    while (running_) loop();
  }

  printf("%s %s\n", success_ ? "SUCCESS" : "FAILURE", name_);
  expectations_.clear();
  ignoring_.clear();
  return success_;
}

void TestCase::eat_ignores() {
  char ibuf[256];
  while (expectation_index_ < expectations_.size() &&
      expectations_[expectation_index_].ignore) {
    ignoring_.push_back(expectations_[expectation_index_]);
    expectations_[expectation_index_].Format(ibuf, sizeof(ibuf));
    printf("Ignoring %s\n", ibuf);
    ++expectation_index_;
  }
}

TestCase* TestCase::caselist = NULL;
TestCase* TestCase::current_ = NULL;
bool TestCase::running_ = false;
bool TestCase::success_ = true;
int TestCase::failpeek_count_ = 0;
TestCase::EventVector TestCase::expectations_;
TestCase::EventVector TestCase::ignoring_;
int TestCase::expectation_index_ = 0;

int CaughtEvent(const Event& evt) {
  return TestCase::Caught(evt);
}

}  // namespace arclino_test

unsigned long millis() {return MOCK(millis);}
unsigned long micros() {return MOCK(micros);}
void pinMode(int pin, int mode) {MOCK(pinMode, pin, mode);}
void analogReference(int ref) {MOCK(analogReference, ref);}
void analogReadResolution(int res) {MOCK(analogReadResolution, res);}
void analogWriteResolution(int res) {MOCK(analogWriteResolution, res);}
int analogRead(int pin) {return MOCK(analogRead, pin);}
void analogWrite(int pin, int value) {MOCK(analogWrite, pin, value);}
void tone(int pin, int freq, int dur){dur?MOCK(tone, pin, freq, dur):MOCK(tone, pin, freq);}
void noTone(int pin) {MOCK(noTone, pin);}
void shiftOut(int datapin, int clockpin, int bitorder, int value) {MOCK(shiftOut, datapin, clockpin, bitorder, value);}
int shiftIn(int datapin, int clockpin, int bitorder) {return MOCK(shiftIn, datapin, clockpin, bitorder);}
long pulseIn(int pin, int value, int timeout) {return timeout?MOCK(pulseIn, pin, value, timeout):MOCK(pulseIn, pin, value);}
void digitalWrite(int pin, bool value) {MOCK(digitalWrite, pin, value);}
bool digitalRead(int pin) {return MOCK(digitalRead, pin);}
void delay(int ms) {MOCK(delay, ms);}
void delayMicroseconds(long us) {MOCK(delayMicroseconds, us);}
int random(long bound, long upper){return upper?MOCK(random, bound, upper):MOCK(random, bound);}
void randomSeed(int s) {MOCK(randomSeed, s);}
void KeyboardClass::set_modifier(uint8_t mod) {MOCK(&KeyboardClass::set_modifier, mod);}
void KeyboardClass::set_key1(uint8_t key) {MOCK(&KeyboardClass::set_key1, key);}
void KeyboardClass::send_now() {MOCK(&KeyboardClass::send_now);}
KeyboardClass Keyboard;
FORMAT(&KeyboardClass::set_modifier) {sprintf(buf, "Keyboard.set_modifier(%d)", e.Get(0));}
FORMAT(&KeyboardClass::set_key1) {sprintf(buf, "Keyboard.set_key1(%d)", e.Get(0));}
FORMAT(&KeyboardClass::send_now) {sprintf(buf, "Keyboard.send_now()");}
void MouseClass::move(int8_t dx, int8_t dy) {MOCK(&MouseClass::move, dx, dy);}
void MouseClass::scroll(int8_t ds) {MOCK(&MouseClass::scroll, ds);}
void MouseClass::set_buttons(bool left, bool middle, bool right) {MOCK(&MouseClass::set_buttons, left, middle, right);}
MouseClass Mouse;
FORMAT(&MouseClass::move) {sprintf(buf, "Mouse.move(%d, %d)", e.Get(0), e.Get(1));}
FORMAT(&MouseClass::scroll) {sprintf(buf, "Mouse.scroll(%d)", e.Get(0));}
FORMAT(&MouseClass::set_buttons) {sprintf(buf, "Mouse.set_buttons(%d, %d, %d)", e.Get(0), e.Get(1), e.Get(2));}

size_t Print::print(const char* s){return MOCK(Print_print_char_array, s);}
FORMAT(Print_print_char_array){sprintf(buf, "Print::print(\"%s\")", e.Get(0));}
size_t Print::print(char s){return MOCK(Print_print_char, s);}
FORMAT(Print_print_char){sprintf(buf, "Print::print('%c')", e.Get(0));}
size_t Print::print(int i, int b = DEC){return MOCK(Print_print_int_base, i, b);}
size_t Print::print(double d, int p){return MOCK(Print_print_double_precision, round(d * pow(10, p)), p);}
FORMAT(Print_print_double_precision){sprintf(buf, "Print::print(%f, %d)", 1.0 * e.Get(0) / pow(10, e.Get(1, 2)), e.Get(1, 2));}

size_t Print::println(const char* s){return MOCK(Print_println_char_array, s);}
FORMAT(Print_println_char_array){sprintf(buf, "Print::println(\"%s\")", e.Get(0));}
size_t Print::println(char s){return MOCK(Print_println_char, s);}
FORMAT(Print_println_char){sprintf(buf, "Print::println('%c')", e.Get(0));}
size_t Print::println(int i, int b = DEC){return MOCK(Print_println_int_base, i, b);}
size_t Print::println(double d, int p){return MOCK(Print_println_double_precision, round(d * pow(10, p)), p);}
FORMAT(Print_println_double_precision){sprintf(buf, "Print::println(%f, %d)", 1.0 * e.Get(0) / pow(10, e.Get(1, 2)), e.Get(1, 2));}

void Serial_::begin(uint16_t baud_count) {MOCK(&Serial_::begin, baud_count);}
FORMAT(&Serial_::begin){sprintf(buf, "Serial.begin(%d)", e.Get(0));}
void Serial_::end() {MOCK(&Serial_::end);}
FORMAT(&Serial_::end){sprintf(buf, "Serial.end()");}
int Serial_::available() {return MOCK(&Serial_::available);}
FORMAT(&Serial_::available){sprintf(buf, "Serial.available()");}
void Serial_::accept() {MOCK(&Serial_::accept);}
FORMAT(&Serial_::accept){sprintf(buf, "Serial.accept()");}
int Serial_::peek() {return MOCK(&Serial_::peek);}
FORMAT(&Serial_::peek){sprintf(buf, "Serial.peek()");}
int Serial_::read() {return MOCK(&Serial_::read);}
FORMAT(&Serial_::read){sprintf(buf, "Serial.read()");}
void Serial_::flush() {MOCK(&Serial_::flush);}
FORMAT(&Serial_::flush){sprintf(buf, "Serial.flush()");}
size_t Serial_::write(uint8_t b) {return MOCK(Serial_write_byte, b);}
FORMAT(Serial_write_byte){sprintf(buf, "Serial.write(%d)", e.Get(0));}
size_t Serial_::write(const char* s){return MOCK(Serial_write_str, s);}
FORMAT(Serial_write_str){sprintf(buf, "Serial.write(\"%s\")", (e.Get(0) ? (const char*)e.Get(0) : ""));}
size_t Serial_::write(const char* s, size_t n){return MOCK(Serial_write_str, s, n);}
FORMAT(Serial_write_strn){sprintf(buf, "Serial.write(\"%s\", %d)", (e.Get(0) ? (const char*)e.Get(0) : ""), e.Get(1));}

size_t Serial_::print(const char* s){return MOCK(Serial_print_char_array, s);}
FORMAT(Serial_print_char_array){sprintf(buf, "Serial.print(\"%s\")", e.Get(0));}
size_t Serial_::print(char s){return MOCK(Serial_print_char, s);}
FORMAT(Serial_print_char){sprintf(buf, "Serial.print('%c')", e.Get(0));}
size_t Serial_::print(int i, int b = DEC){return MOCK(Serial_print_int_base, i, b);}
FORMAT(Serial_print_int_base){sprintf(buf, "Serial.print(%d, %d)", e.Get(0), e.Get(1));}
size_t Serial_::print(double d, int p){return MOCK(Serial_print_double_precision, round(d * pow(10, p)), p);}
FORMAT(Serial_print_double_precision){sprintf(buf, "Serial.print(%f, %d)", 1.0 * e.Get(0) / pow(10, e.Get(1, 2)), e.Get(1, 2));}

size_t Serial_::println(const char* s){return MOCK(Serial_println_char_array, s);}
FORMAT(Serial_println_char_array){sprintf(buf, "Serial.println(\"%s\")", e.Get(0));}
size_t Serial_::println(char s){return MOCK(Serial_println_char, s);}
FORMAT(Serial_println_char){sprintf(buf, "Serial.println('%c')", e.Get(0));}
size_t Serial_::println(int i, int b = DEC){return MOCK(Serial_println_int_base, i, b);}
FORMAT(Serial_println_int_base){sprintf(buf, "Serial.println(%d, %d)", e.Get(0), e.Get(1) || 10);}
size_t Serial_::println(double d, int p){return MOCK(Serial_println_double_precision, round(d * pow(10, p)), p);}
FORMAT(Serial_println_double_precision){sprintf(buf, "Serial.println(%f, %d)", 1.0 * e.Get(0) / pow(10, e.Get(1, 2)), e.Get(1, 2));}

bool Serial_::_bool() {return MOCK(&Serial_::_bool);}
FORMAT(&Serial_::_bool){sprintf(buf, "if(Serial)");}
Serial_ Serial;
// MISSING DEFINITION?

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int main(int argc, char* argv[]) {
  return arclino_test::TestCase::Run(argc, argv);
}
