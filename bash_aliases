export PS1='\[\e[0;36m\]\w \[\e[m\]'
export EDITOR=vw
export CHROME_DEVEL_SANDBOX=/usr/local/sbin/chrome-devel-sandbox

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
addpath $HOME/Downloads/android-sdk-linux/platform-tools
addpath $HOME/Downloads/google_appengine
