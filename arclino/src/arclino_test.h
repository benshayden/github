#ifndef ARCLINO_TEST_H_
#define ARCLINO_TEST_H_

#include "Arduino.h"
#include <vector>

namespace arclino_test {

struct Event {
  void* type;
  std::vector<int> args;
  int returns;
  char* filename;
  int lineno;
  bool ignore;
  bool caught;

  Event& From(const char* fn, int line);
  Event& Caught();
  Event& Ignore();

  Event& operator[](int ret);

  int operator()(void* t);
  int operator()(void* t, int a0);
  int operator()(void* t, int a0, int a1);
  int operator()(void* t, int a0, int a1, int a2);
  int operator()(void* t, int a0, int a1, int a2, int a3);
  int operator()(void* t, int a0, int a1, int a2, int a3, int a4);
  int operator()(void* t, int a0, int a1, int a2, int a3, int a4, int a5);
  int operator()(void* t, int a0, int a1, int a2, int a3, int a4, int a5, int a6);

  Event();
  ~Event();

  int Get(int argi, int def=0) const;
  bool Matches(const Event& that) const;
  void Format(char* buf, int bufsize) const;

  class Formatter {
   public:
    typedef void (*FunType)(char* buf, const Event& args);

    static bool Format(const Event& evt, char* buf);

    Formatter(void* type, FunType fp);
    ~Formatter();

   private:
    static Formatter* list_;
    Formatter* next_;
    void* type_;
    FunType fp_;
  };
};

class TestCase {
 public:
  typedef std::vector<Event> EventVector;
  typedef void(*ExpectationsFunction)(EventVector*);

  static int Run(int argc, char* argv[]);
  static int Caught(const Event& caught);

  TestCase(const char* name, ExpectationsFunction expectations);

 private:
  static TestCase* caselist;
  static TestCase* current_;
  static bool running_;
  static bool success_;
  static int failpeek_count_;
  static EventVector expectations_;
  static EventVector ignoring_;
  static int expectation_index_;

  bool Run();

  static void eat_ignores();

  const char* name_;
  TestCase* next_;
  ExpectationsFunction expectations_fp_;
};

}  // namespace arclino_test

#define TOKENPASTE(x, y) x ## y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)

#define TEST(name) \
  void TOKENPASTE(name, _expectations)(arclino_test::TestCase::EventVector* events); \
  arclino_test::TestCase TOKENPASTE(name, _TestCase)(#name, TOKENPASTE(name, _expectations)); \
  void TOKENPASTE(name, _expectations)(arclino_test::TestCase::EventVector* events)

#define EXPECT events->push_back(arclino_test::Event().From(__FILE__, __LINE__));events->back()
#define IGNORE events->push_back(arclino_test::Event().From(__FILE__, __LINE__).Ignore());events->back()

#define MOCK arclino_test::Event().Caught()

#define FORMAT(name) namespace arclino_test {\
  void TOKENPASTE2(__FORMAT__, __LINE__)(char* buf, const Event& e); \
  Event::Formatter TOKENPASTE2(__FORMATTER__, __LINE__)((name), &TOKENPASTE2(__FORMAT__, __LINE__));\
} \
void arclino_test::TOKENPASTE2(__FORMAT__, __LINE__)(char* buf, const Event& e)

#define Print_print_char_array ((size_t(*)(const char*))&Print::print)
#define Print_print_char ((size_t(*)(char))&Print::print)
#define Print_print_int_base ((size_t(*)(int,int))&Print::print)
#define Print_print_double_precision ((size_t(*)(double,int))&Print::print)
#define Print_println_char_array ((size_t(*)(const char*))&Print::println)
#define Print_println_char ((size_t(*)(char))&Print::println)
#define Print_println_int_base ((size_t(*)(int,int))&Print::println)
#define Print_println_double_precision ((size_t(*)(double,int))&Print::println)
#define Serial_write_byte ((size_t(*)(uint8_t))&Serial_::write)
#define Serial_write_str ((size_t(*)(const char*))&Serial_::write)
#define Serial_write_strn ((size_t(*)(const char*,size_t))&Serial_::write)
#define Serial_print_char_array ((size_t(*)(const char*))&Serial_::print)
#define Serial_print_char ((size_t(*)(char))&Serial_::print)
#define Serial_print_int_base ((size_t(*)(int,int))&Serial_::print)
#define Serial_print_double_precision ((size_t(*)(double,int))&Serial_::print)
#define Serial_println_char_array ((size_t(*)(const char*))&Serial_::println)
#define Serial_println_char ((size_t(*)(char))&Serial_::println)
#define Serial_println_int_base ((size_t(*)(int,int))&Serial_::println)
#define Serial_println_double_precision ((size_t(*)(double,int))&Serial_::println)

#endif  // ARCLINO_TEST_H_
