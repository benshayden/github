export PS1='\[\e[0;36m\]\w \[\e[m\]'

alias get="sudo apt-get install"
alias gets="sudo aptitude search"
alias kd=popd
alias l="ls -Alh"
alias p="ps -Ao pid,command"
alias pd=pushd

function addpath() {
  echo $PATH | grep -q "\(^$1\|:$1\)" || export PATH="$1:$PATH"
}
addpath $HOME/bin
addpath $HOME/dev/depot_tools
addpath $HOME/Downloads/android-sdk-linux/platform-tools
unset addpath
