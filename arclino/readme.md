INSTALLATION
Copy 'arclino' to somewhere in your $PATH. That's it.

PRE-REQUISITES
Arduino from http://arduino.cc/en/Main/Software
A native C++ compiler like gcc.gnu.org or clang.llvm.org
If on Windows, http://www.cygwin.com

USAGE
In a command line, cd to a directory containing a sketch.ino.
Run 'arclino upload' or 'arclino sketch.ino' to compile and upload the sketch to a connected Arduino.
The script will walk you through configuration on the first run and save your options in '.arclino.conf'.
Create a file 'sketch.test' and run 'arclino test' or 'arclino sketch.test' to compile and run tests natively on the host machine.

TESTS FILES
*.test files look like this:
TEST(my_sketch) {
  EXPECT(pinMode, buttonPin, INPUT_PULLUP);
  for (int i = 0; i < 10; ++i) {
    // The matching digitalRead call in sketch.ino will return HIGH.
    EXPECT[HIGH](digitalRead, buttonPin);
    EXPECT(delay, 1);
  }
}
The .test file will be included after sketch.ino so it has access to all configuration constants defined there.
Each .test file may contain one or more TEST(){}. setup() will be called between them so that it can reset any state as necessary.
Mock a library by creating a 'library_name.mock' file. See libraries/EEPROM/EEPROM.mock for a sample.
By default, tests will use the real implementations of libraries. In order to use the mock implementation, add a comment like this to the .test file:
// MOCK Stepper

VIM USERS
autocmd BufRead *.ino nmap <buffer> <leader>a :up<CR>:!arclino %<CR>
autocmd BufRead *.test nmap <buffer> <leader>a :up<CR>:!arclino %<CR>
autocmd BufRead *.ino set filetype=cpp
autocmd BufRead *.test set filetype=cpp
autocmd BufRead *.mock set filetype=cpp
