#!/bin/sh
# TODO test in cygwin

CD="${CD:-.}"
subcmd="$1"
if [ -f "$subcmd" ];then
  CD="$(dirname "$subcmd")"
  if echo "$subcmd" | grep -E '\.ino$' &>/dev/null;then
    subcmd=upload
  else
    if echo "$subcmd" | grep -E '\.test$' &>/dev/null;then
      subcmd=test
    else
      subcmd=""
    fi
  fi
fi

orig_pwd="$PWD"
cd "$CD"
if [ "$orig_pwd" != "$PWD" ];then
  echo Now in "$PWD"
fi
unset CD orig_pwd cur_pwd

if [ -z "$subcmd" ] || ( [ "$subcmd" != "sketch" ] &&
                         [ "$subcmd" != "library" ] &&
                         [ "$subcmd" != "test" ] &&
                         [ "$subcmd" != "build" ] &&
                         [ "$subcmd" != "upload" ] &&
                         [ "$subcmd" != "clean" ] );then
  cat << MAIN_HELP_DOC
Usage:

$ arclino sketch Hello
Creates Hello/Hello.ino.

$ arclino library Hello
Creates Hello/Hello.h and Hello/Hello.cpp. Must be run in a sketch directory.

$ arclino upload
Compiles the sketch.ino and uploads it to the Arduino.
Configuration options are stored in .arclino.conf and will be prompted on first run.

$ arclino test
Compiles the sketch.ino and *.test C++ files for the host computer and runs them natively.
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

$ arclino clean
Deletes temporary files created by arclino.

$ arclino path/to/sketch.ino
Same as CD=path/to arclino upload, for convenient scripting.

$ arclino path/to/sketch.test
Same as CD=path/to arclino test, for convenient scripting.
MAIN_HELP_DOC
  exit
fi

if [ "$subcmd" == "sketch" ];then
  mkdir "$2"
  cat > "$2/$2.ino" <<NEW_SKETCH
void setup() {
}

void loop() {
}
NEW_SKETCH
  echo Now cd "$2" and run "'"arclino upload"'"
  "$EDITOR" "$2/$2.ino"
  exit 0
fi

if (! ls *.ino &>/dev/null) || [ $(ls *.ino | wc -l) -gt 1 ];then
  echo Please cd to a directory containing exactly one sketch.ino file or create one.
  exit 1
fi

INO="$(ls *.ino)"
SKETCH=${INO%.ino}
WORK=$(readlink -f .)/.arclino-work
TEST_OBJDIR=$WORK/test
TEST_CORE=$TEST_OBJDIR/.core
TEST_LIBRARIES=$TEST_OBJDIR/libraries
AVR_OBJDIR=$WORK/avr
AVR_MAIN_CC=$AVR_OBJDIR/$SKETCH.cc
AVR_CORE=$AVR_OBJDIR/.core
AVR_LIBRARIES=$AVR_OBJDIR/libraries

CONF_SH

ARDUINO_LIBRARIES=$arduino/libraries

if [ "$subcmd" == "library" ];then
  libname="$2"
  if [ -e "$ARDUINO_LIBRARIES/$libname" ];then
    echo "Error! $ARDUINO_LIBRARIES/$libname already exists!"
    exit 1
  fi
  mkdir "$ARDUINO_LIBRARIES/$libname"
  cat > "$ARDUINO_LIBRARIES/$libname/$libname.h" << NEW_LIBRARY_H
#ifndef ${libname}_h
#define ${libname}_h

#include <Arduino.h>

class $libname {
 public:
  $libname();
  ~$libname();

 private:
};

#endif
NEW_LIBRARY_H
  cat > "$ARDUINO_LIBRARIES/$libname/$libname.cpp" << NEW_LIBRARY_CPP
#include "$libname.h"

$libname::$libname() {
}

$libname::~$libname() {
}
NEW_LIBRARY_CPP
  echo Created "$ARDUINO_LIBRARIES/$libname/$libname.{h,cpp}"
  "$EDITOR" "$ARDUINO_LIBRARIES/$libname/$libname.h" "$ARDUINO_LIBRARIES/$libname/$libname.cpp"
  exit
fi

function include_libname() {
  local include="$1"
  if echo "$include" | grep '"' &>/dev/null;then
    include=$(echo "$include" | cut -d'"' -f2)
  else
    include=$(echo "$include" | cut -d'<' -f2 | cut -d'>' -f1)
  fi
  echo ${include%.h}
}

# Automatically install missing libraries
# // http://benjhayden.googlecode.com/ButtonPlex.zip
# #include "ButtonPlex.h"
function install_libs(){
  local -r src="$1"
  grep -E -B1 '^#include' "$src" | grep -E '^// http' | cut -d' ' -f2- | while read liburl;do
    libname=$(include_libname "$(grep -A1 "// $liburl" "$src" | grep -E '^#include' | head -1)")
    if [ ! -d "$ARDUINO_LIBRARIES/$libname" ];then
      echo Installing $libname from $liburl
      if ! curl -o "$ARDUINO_LIBRARIES/$libname.zip" "$liburl";then
        echo Unable to fetch $liburl
        exit 1
      fi
      mkdir "$libname"
      pushd "$ARDUINO_LIBRARIES/$libname"
      if ! unzip "$libname.zip";then
        echo Unable to unzip $libname.zip
        exit 1
      fi
      if [ ! -f "$libname.h" ] && [ -f "$libname/$libname.h" ];then
        # The zip contained a directory instead of the naked files.
        mv "$libname" ../."$libname"
        cd ..
        rmdir "$libname"
        mv {.,}"$libname"
      fi
      popd
      if [ ! -d "$libname" ];then
        echo $libname.zip did not contain a directory "'$libname'"
        exit 1
      fi
      (for ext in h cc cpp;do find "$libname" -iname "*.$ext";done) | while read src2;do
        install_libs "$src2"
      done
    fi
  done
}
install_libs "$INO"
ls *.h *.cc *.cpp 2>/dev/null | while read src;do
  install_libs "$src"
done

function ls_libs() {
  grep -E '^#include ' "$@" | sort -u | ( [ -n "$ls_libs_EXCEPT" ] && grep -Evw "($ls_libs_EXCEPT)" || cat ) | while read include;do
    libname=$(include_libname "$include")
    [ -d "$ARDUINO_LIBRARIES/$libname" ] && echo "$libname"
    ls_libs_EXCEPT="$ls_libs_EXCEPT|$libname" ls_libs "$ARDUINO_LIBRARIES/$libname"/* | grep -Ev "^$libname\$"
  done
}

function maybe_ln() {
  # gnu make will remake a file that depends on a symlink if either the
  # symlink's timestamp or the symlink's target's timestamp is newer than the
  # target, so be careful to not remake symlinks unnecessarily.
  local -r src=$(readlink -f "$1")
  local -r dest=$2/$(basename "$1")
  if [ ! -L "$dest" ] || [ "$(readlink -f "$dest")" != "$src" ];then
    ln -sf "$src" "$dest"
  fi
}

function setup_libraries() {
  local -r libobjdir="$1"
  local -r testf="${2:-/dev/null}"
  (ls_libs "$INO"
   ls *.h *.cc *.cpp 2>/dev/null | while read localf;do
     ls_libs "$localf"
   done
  ) | while read libname;do
    if grep -E "^// MOCK $libname$" "$testf" &>/dev/null;then
      outdir="$libobjdir/.mock.$libname"
      mkdir -p "$outdir"
      mock=$ARDUINO_LIBRARIES/$libname/$libname.mock
      if [ ! -f "$mock" ];then
        echo Missing "$mock" >&2
        exit 1
      fi
      (echo '#include "Arduino.h"'
       echo "#include \"$libname.h\""
       echo '#include "arclino_test.h"'
       echo "#include \"$mock\""
      ) | maybe_write "$outdir/$libname.cc"
    else
      outdir="$libobjdir/$libname"
      mkdir -p "$outdir"
      ls "$ARDUINO_LIBRARIES/$libname"/*.{c,cc,cpp} 2>/dev/null | while read c;do
        maybe_ln "$c" "$outdir"
      done
      local arduino_utility="$ARDUINO_LIBRARIES/$libname/utility"
      if [ -d "$arduino_utility" ];then
        local obj_utility="$outdir/utility"
        mkdir "$obj_utility"
        ls "$arduino_utility"
        ls "$arduino_utility" 2>/dev/null | while read utilf;do
          maybe_ln "$arduino_utility/$utilf" "$obj_utility"
        done
      fi
    fi
    ls "$ARDUINO_LIBRARIES/$libname/"*.h 2>/dev/null | while read h;do
      maybe_ln "$h" "$outdir"
    done
    echo "$outdir"
    unset outdir
  done
}

function run_make() {
  make -s -f - "$@" <<MAKEFILE_DOC
MAKEFILE_DOC
  res=$?
  if [ $res -ne 0 ];then
    exit 1
  fi
}

function maybe_write() {
  # preserve timestamps if file contents haven't changed so that make doesn't
  # rebuild unnecessarily.
  local -r dest="$1"
  local -r tmpf="$(tempfile -d . -p . -s ".${1##*.}")"
  cat > "$tmpf"
  if [ ! -f "$dest" ] || ! diff "$dest" "$tmpf" &>/dev/null;then
    mv "$tmpf" "$dest"
  else
    rm "$tmpf"
  fi
}

if [ "$subcmd" == "test" ];then
  NUMTESTS="$(ls *.test 2>/dev/null | wc -l)"

  if [ $NUMTESTS -eq 0 ];then
    cat << TEST_HELP_DOC
No *.test files found!
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
TEST_HELP_DOC
    exit 1
  fi

  mkdir -p "$TEST_CORE"
  maybe_write "$TEST_CORE/Arduino.h" << ARDUINO_H
ARDUINO_H
  maybe_write "$TEST_CORE/arclino_test.h" << ARCLINO_TEST_H
ARCLINO_TEST_H

  (echo "#define FAILPEEK_LIMIT ${failpeek:-10}"
   cat << ARCLINO_TEST_CC
ARCLINO_TEST_CC
  ) | maybe_write "$TEST_CORE/arclino_test.cc"

  ls *.test | while read TESTF;do
    TEST=${TESTF%.test}
    # TODO separate objdir, .conf for each test, shared core, libraries
    THIS_TEST_OBJDIR=$TEST_OBJDIR/$TEST
    mkdir -p "$THIS_TEST_OBJDIR"
    ln -sf "$TEST_CORE" "$THIS_TEST_OBJDIR"

    TEST_BIN=$THIS_TEST_OBJDIR/$TEST.elf
    TEST_LOG=$THIS_TEST_OBJDIR/$TEST.log
    TEST_MAIN_CC=$THIS_TEST_OBJDIR/$TEST.cc

    TEST_COVERAGE_LOG=$THIS_TEST_OBJDIR/coverage.log
    TEST_COVERAGE_DIR=$THIS_TEST_OBJDIR/coverage

    (echo '#include "Arduino.h"'
     echo "#include \"$INO\""
     echo '#include "arclino_test.h"'
     echo "#include \"$TESTF\""
    ) | maybe_write "$TEST_MAIN_CC"

    (echo CXX=$native_cxx
     echo CC=$native_cxx
     echo CORE=$TEST_CORE
     setup_libraries "$TEST_LIBRARIES" "$TESTF" | while read d;do
       echo LIBRARY_DIRS += $d
     done
     echo CPPFLAGS += -lm -fpermissive -w -rdynamic -Wno-pmf-conversions -DARCLINO_TEST
     echo LDFLAGS += -rdynamic
     if [ $coverage -eq 1 ];then
       echo CPPFLAGS += -fprofile-arcs -ftest-coverage
     fi
     echo TARGET=$TEST
    ) | maybe_write "$THIS_TEST_OBJDIR/.conf"

    OBJDIR="$THIS_TEST_OBJDIR" run_make "$TEST_BIN"
    if [ ! -f "$TEST_BIN" ];then
      exit 1
    fi
    "$(readlink -f "$TEST_BIN")"
    res=$?
    if [ $res -ne 0 ];then
      exit 1
    fi

    if [ $coverage -eq 1 ]; then
      gcov "$TEST_MAIN_CC" &> "$TEST_COVERAGE_LOG"
      ls *.ino *.cc *.cpp 2>/dev/null | while read f;do
        grep -A 1 "$f" "$TEST_COVERAGE_LOG" | grep . | grep -v :creating | grep -v '^--$'
      done
      rm -f *.mock.gcov *.h.gcov vector.tcc.gcov *.tests.gcov new.gcov .arclino_unittests.gcda .arclino_unittests.gcno .arclino_unittests.cc.gcov "$TEST_COVERAGE_LOG"
      if [ $NUMTESTS -ne 1 ]; then
        mkdir -p "$TEST_COVERAGE_DIR"
        rm -rf "$TEST_COVERAGE_DIR"/*
        mv *.gcov "$TEST_COVERAGE_DIR"
      fi
    fi  # coverage
  done  # for each TESTF
fi  # arclino test

function boardconf() {
  local -r conf="$1"
  ( grep -E "^$board\.$conf=" "$boards_txt"
    for menu in $board_menus;do
      grep -E "^$board\.menu\.$menu\.$(eval "echo \${board_menu_$menu}")\.$conf=" "$boards_txt"
    done
  ) | grep . | cut -d= -f2-
}

if [ "$subcmd" == "build" ] || [ "$subcmd" == "upload" ];then
  mkdir -p "$AVR_OBJDIR"
  (echo '#include "Arduino.h"'
   echo "#include \"$INO\""
  ) | maybe_write "$AVR_MAIN_CC"

  core_src=$(for d in "$arduino"/hardware/*;do
    subcore=$(grep -E "^$board.build.core=" "$d/boards.txt" 2>/dev/null | cut -d= -f2)
    if [ -n "$subcore" ];then
      echo "$d"/cores/$subcore
    fi
  done)

  mkdir -p "$AVR_CORE"
  ls "$core_src" 2>/dev/null | while read coref;do
    maybe_ln "$core_src/$coref" "$AVR_CORE"
  done

  setup_libraries "$AVR_LIBRARIES" &>/dev/null

  mcu=$(boardconf build.mcu)
  f_cpu=$(boardconf build.f_cpu)
  board_cppflags="$(boardconf "build.define[0-9]+")"

  (echo CORE=$AVR_CORE
   echo CXX=$avr_tools/avr-g++
   echo CC=$avr_tools/avr-gcc
   echo OBJCOPY=$avr_tools/avr-objcopy
   echo OBJDUMP=$avr_tools/avr-objdump
   echo AR=$avr_tools/avr-ar
   echo SIZE=$avr_tools/avr-size
   echo NM=$avr_tools/avr-nm
   echo ASFLAGS += -mmcu=$mcu -I. -x assembler-with-cpp
   echo LDFLAGS += -mmcu=$mcu -lm -Wl,--gc-sections -Os
   echo 'LIBRARY_DIRS=$(wildcard '$AVR_LIBRARIES'/*/)'
   echo CPPFLAGS += -mmcu=$mcu -DF_CPU=$f_cpu -g -Os -w -Wall -ffunction-sections -fdata-sections -I\"$core_src\" $board_cppflags
   echo TARGET=$SKETCH
  ) | maybe_write "$AVR_OBJDIR/.conf"

  OBJDIR="$AVR_OBJDIR" run_make "$AVR_OBJDIR/$SKETCH.hex"

  post_compile_script=$(boardconf build.post_compile_script)
  if [ -n "$post_compile_script" ];then
    base="$arduino/hardware/tools"
    "$base/$post_compile_script" -board=$board -tools="$base/" -path="$AVR_OBJDIR" -file="$SKETCH"
  fi
fi

function tools_avr_foo_avrdude() {
  "$arduino/hardware/tools/avr/bin/avrdude" -C"$arduino/hardware/tools/avr/etc/avrdude.conf" "$@"
}

function tools_avrdude() {
  "$arduino/hardware/tools/avrdude" -C"$arduino/hardware/tools/avrdude.conf" "$@"
}

function scan_serial_ports() {
  addrlist="28541 4984 18924 16924 27183 31091"
  for port in $addrlist;do
    if echo -n | nc -q0 localhost $port &>/dev/null;then
      echo $port
    fi
  done
}

function monitor_serial() {
  local -ir port=$1
  if [ -n "$serial_log" ];then
    stdbuf -i0 -o0 -e0 nc -d -q-1 localhost $port 2>&1 | stdbuf -i0 -o0 -e0 tee "$AVR_OBJDIR/$serial_log" &>/dev/null &
  else
    echo You are online with $SKETCH.ino:
    nc localhost $port
  fi
}

if [ "$subcmd" == "upload" ];then
  avrdude_wrapper=$(boardconf upload.avrdude_wrapper)
  if [ -n "$avrdude_wrapper" ];then
    base_cmd="$arduino/hardware/tools/$avrdude_wrapper"
  else
    if [ "$(uname)" == "Linux" ];then
      if [ -f "$arduino/hardware/tools/avrdude" ];then
        base_cmd=tools_avrdude
      else
        base_cmd=avrdude
      fi
    else
      base_cmd=tools_avr_foo_avrdude
    fi
  fi
  "$base_cmd" -p"$(boardconf build.mcu)" \
              -c"$(boardconf upload.protocol)" \
              -P"$(boardconf serial.port)" \
              -b"$(boardconf upload.speed)" \
              -D -V \
              -U"flash:w:$AVR_OBJDIR/$SKETCH.hex:i"

  port=$(scan_serial_ports)
  if [ -n "$port" ];then
    monitor_serial "$port"
  else
    fake_serial=$(boardconf fake_serial)
    if [ -n "$fake_serial" ];then
      "$arduino/hardware/tools/$fake_serial" &
      port=$(scan_serial_ports)
      if [ -z "$port" ];then
        echo Unable to connect to serial
        exit 1
      fi
      monitor_serial "$port"
    fi
  fi
fi

if [ "$subcmd" == "clean" ];then
  rm -rf "$WORK"
fi
