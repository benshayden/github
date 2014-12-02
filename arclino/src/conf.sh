function isDarwin() {
  uname | grep Darwin &>/dev/null
}

function invalid_root() {
  [ ! -d "$1/hardware" ] || [ ! -d "$1/libraries" ]
}

function valid_native_cxx() {
  which "$1" &>/dev/null || return 1
  inp="$(tempfile -d . -s .cc)"
  outp="$(tempfile -d .)"
  echo 'int main(){}' > "$inp"
  "$1" -fprofile-arcs -ftest-coverage -lm -fpermissive -w -rdynamic -Wno-pmf-conversions "$inp" -o "$outp" &>/dev/null
  local -ir res=$?
  rm -f "$inp" "$outp" "${inp%.cc}.gcno"
  return $res
}

function find_avr_tools() {
  for avr_tools in "$arduino/hardware/tools" "$arduino/hardware/tools/avr/bin" "$(dirname "$(which avr-gcc)")";do
    if [ -f "$avr_tools/avr-g++" ];then
      echo "$avr_tools"
      return
    fi
  done
}

CONF=.arclino.conf
if [ ! -f "$CONF" ];then
  arduino="${arduino:-$(isDarwin && echo /Applications/Arduino.app/Contents/Resources/Java || echo)}"
  while invalid_root "$arduino";do
    echo -n 'Where is Arduino installed? (Tab auto-completes!) '
    read -e arduino
    arduino="$(readlink -f "$(echo "$arduino" | sed -e "s/^~/$(echo $HOME | sed -e 's/\//\\\//g')/")")"
  done

  avr_tools="$(find_avr_tools)"
  if [ -z "$avr_tools" ];then
    echo Please install gcc-avr
    exit 1
  fi

  native_cxx=$(isDarwin && echo clang++ || echo g++)
  while ! valid_native_cxx "$native_cxx";do
    echo -n 'What command can compile C++ natively for test coverage with gcov? '
    read native_cxx
  done

  # all_mcus="$( for w in $( (for w in $(avr-gcc --target-help|grep -E -A45 '^Known MCU names:'|grep -E '^  a');do echo $w;done) | sort );do echo -n "$w ";done)"

  echo All known arduino boards:
  echo -e "[codename]\t[board name]"
  cat "$arduino"/hardware/*/boards.txt | grep -E '^\w+\.name=' | while read board_name;do
    codename=$(echo $board_name | cut -d. -f1)
    echo -e "$codename\t$( [ $(echo "$codename" | wc -c) -le 8 ] && echo "\t")$(echo $board_name | cut -d= -f2-)"
  done
  board=""
  while ! grep -E "^$board\.name=" "$arduino"/hardware/*/boards.txt &>/dev/null;do
    if [ -n "$board" ];then
      echo Did not find $board
    fi
    echo -n 'Type the codename of the board you are using: '
    read board
  done
  boards_txt=$(for d in "$arduino"/hardware/*;do
    if grep -E "^$board\.name=" "$d/boards.txt" &>/dev/null;then
      echo "$d/boards.txt"
    fi
  done)
  board_menus=$(grep -E '^menu\.' "$boards_txt" | cut -d. -f2 | cut -d= -f1)
  for menu in $board_menus;do
    opts=$(grep -E "^$board\.menu\.$menu\.[a-zA-Z0-9_\\-]+\.name=" "$boards_txt" | cut -d. -f4)
    menudefault="$(for opt in $opts;do echo $opt;break;done)"
    eval "board_menu_$menu=$menudefault"
    echo Defaulting menu $menu to $menudefault
  done

  (echo arduino=$arduino
   echo native_cxx=$native_cxx
   echo board=$board
   for menu in $board_menus;do
     opts=$(grep -E "^$board\.menu\.$menu\.[a-zA-Z0-9_\\-]+\.name=" "$boards_txt" | cut -d. -f4)
     echo -n '#'
     for w in $opts;do
       echo -n " $w($(grep -E "^$board\.menu\.$menu\.$w\.name" "$boards_txt" | cut -d= -f2-))"
     done
     echo
     echo board_menu_$menu=$(eval "echo \$board_menu_$menu")
   done
   echo '# Number of events to peek at after a test fails:'
   echo failpeek=10
   echo '# Set to 1 to compute code test coverage:'
   echo coverage=0
   echo '# Set to "" if your sketch reads from Serial in order to interact with it. If your sketch does not read from Serial, then anything that it writes to Serial will be logged at .arclino-work/avr/$serial_log'
   echo serial_log=log
  ) > "$CONF"
  echo Wrote "$CONF"
else
  echo Reading "$CONF"

  function getconf() {
    grep -E "^$1=" "$CONF" | cut -d= -f2-
  }
  arduino="$(getconf arduino)"
  board="$(getconf board)"
  boards_txt=$(for d in "$arduino"/hardware/*;do
    if grep -E "^$board\.name=" "$d/boards.txt" &>/dev/null;then
      echo "$d/boards.txt"
    fi
  done)
  board_menus=$(grep -E '^menu\.' "$boards_txt" | cut -d. -f2 | cut -d= -f1)
  for menu in $board_menus;do
    eval "board_menu_$menu=\$(getconf board_menu_$menu)"
  done
  native_cxx="$(getconf native_cxx)"
  failpeek=${failpeek:-$(getconf failpeek)}
  coverage=${coverage:-$(getconf coverage)}
  serial_log=${serial_log:-$(getconf serial_log)}

  if invalid_root "$arduino";then
    echo Did not find Arduino in "$arduino"
    echo Please fix "$CONF"
    exit 1
  fi

  avr_tools="$(find_avr_tools)"
  if [ -z "$avr_tools" ];then
    echo Please install gcc-avr
    exit 1
  fi

  if ! valid_native_cxx "$native_cxx" &>/dev/null;then
    cat << TEST_MISSING_CXX
Please install a non-avr C++ compiler such as gcc.gnu.org or clang.llvm.org. If
you have already installed a non-avr C++ compiler, then please set
native_cxx=/the/path/to/c++/compiler
TEST_MISSING_CXX
    echo in "$CONF"
    exit 1
  fi
fi

