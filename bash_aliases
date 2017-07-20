export PS1='\[\e[0;36m\]\w \[\e[m\]'
export EDITOR=vw
export CHROME_DEVEL_SANDBOX=/usr/local/sbin/chrome-devel-sandbox
export tt=tracing/tracing
export PYTHONSTARTUP=~/.pythonrc.py

alias get="sudo apt-get install"
alias gets="aptitude search"
alias kd=popd
alias l="ls -Alh"
alias p="ps -Ao pid,command"
alias pd=pushd
alias py=python
alias tyep=type

function addpath() {
  echo $PATH | grep -q "\(^$1\|:$1\)" || export PATH="$1:$PATH"
}
addpath $HOME/bin
addpath $HOME/dev/depot_tools
addpath $HOME/dev/goma
addpath $HOME/Downloads/android-sdk-linux/platform-tools
alias cd..="cd .."
export dd=dashboard/dashboard

function pdc() {
  for c in {a..z};do
    if [ ! -f ~/dev/$c.desc ]; then
      pushd ~/dev/$c
      break;
    fi
  done
}

function watchdir() {
  inotifywait "$1" -r 2>/dev/null | grep "$1" | grep CREATE >/dev/null && ( sleep 1; $2 )
  watchdir "$1" "$2"
  sleep 0.3
}
