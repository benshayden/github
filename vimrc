set vb
set t_vb=
set nocompatible
set viminfo=
set shortmess=I
"call pathogen#helptags()
"call pathogen#runtime_append_all_bundles()
"disable shiftwidth=4 in ftplugin/python.vim
let g:python_recommended_style=0
autocmd filetype python set expandtab shiftwidth=2 softtabstop=2 tabstop=4
cmap w!! w !sudo tee % > /dev/null
filetype indent on
filetype on
filetype plugin on
let NERDTreeShowBookmarks=1
let NERDTreeShowFiles=1
let NERDTreeShowHidden=1
let g:yankring_history_dir="~/.vim"
let g:yankring_history_file=".yankring"
let mapleader=","
let g:ConqueTerm_EscKey="jj"
let g:ConqueTerm_ReadUnfocused=1
nmap <C-h> <C-w>h
nmap <C-j> <C-w>j
nmap <C-k> <C-w>k
nmap <C-l> <C-w>l
nmap <silent> <leader>1 <C-w>t
nmap <silent> <leader>2 <C-w>t<C-w>l
nmap <silent> <leader>3 <C-w>t2<C-w>l
nmap <silent> <leader>4 <C-w>b
nmap <silent> <leader>/ :let @/=""<CR>:echo<CR>
nmap <silent> <leader><Tab> :e %:h/
nmap <silent> <leader>ev :e $MYVIMRC<CR>
nmap <silent> <leader>sv :so $MYVIMRC<CR>
nmap <silent> <leader>c :bd<CR>
nmap <silent> <leader>h {jV}k:sort u<CR>
function! MySetPath()
  exe 'set path=' . matchstr(expand('%:h'), '\C\zs.\{-}dev/./src\ze') . ',,.,/usr/include'
endfunction
autocmd BufRead,BufEnter,BufFilePost,BufWinEnter **/dev/?/src/** call MySetPath()
autocmd BufEnter * :syntax sync fromstart
nmap <silent> <leader>P :call MySetPath()<CR>
autocmd BufRead **/dev/?/src/** nmap <silent> <buffer> <leader>m :silent !n %:h &<CR>:redraw!<CR>
autocmd BufRead *.ino nmap <buffer> <leader>a :up<CR>:!arclino %<CR>
autocmd BufRead *.test nmap <buffer> <leader>a :up<CR>:!arclino %<CR>
autocmd BufRead *.ino set filetype=cpp
autocmd BufRead *.test set filetype=cpp
autocmd BufRead *.mock set filetype=cpp
autocmd BufRead BUILD set filetype=conf
nmap <silent> <leader>R <leader>1:vert resize 90<CR><leader>2:vert resize 90<CR><leader>3:vert resize 90<CR><leader>4:vert resize 90<CR>
nmap <silent> <leader>fm /\(<<<<<<<\\|=======\\|>>>>>>>\)<CR>
nmap <silent> <leader>hs <C-h>:q<CR>:call SwitchSourceHeader()<CR>
nmap <silent> <leader>ls <C-l>:q<CR>:call SwitchSourceHeader()<CR>
nmap <F6> <leader>
nmap <F1> <Esc>
imap jj <Esc>
imap <F6> <Esc><leader>
map <F1> <Esc>
imap <F1> <Esc>
nnoremap <silent> <leader>b :buffers<CR>:b<Space>
nnoremap ; :
set autoindent
set autoread
set background=dark
set backspace=indent,eol,start
set copyindent
set csto=0
set expandtab
set hidden
set history=1000
set hlsearch
set ignorecase
set incsearch
set laststatus=0
set lazyredraw
set list
autocmd filetype conque_term set nolist
set listchars=tab:>.,trail:.,extends:@,nbsp:.
set modeline
set nobackup
set noerrorbells
set noswapfile
set notitle
set nowrap
set pastetoggle=<F2>
set ruler
set shiftround
set shiftwidth=2
set showmatch
set smartcase
set smarttab
" set spell spelllang=en_us
set tabstop=2
set tags=tags;/
set textwidth=80
set undolevels=1000
set visualbell
set wildignore=*.swp,*.bak,*.pyc,*.class
set wildmenu
set wildmode=longest,list,full
syntax on
hi VertSplit term=none gui=none cterm=none
hi link javaScript NONE
"autocmd filetype cpp setlocal foldenable
set nofoldenable
"set foldmethod=expr

ab hte the
ab teh the

function! MyFoldExpr(line)
  if a:line=~'{{{'
    return 'a1'
  endif
  if a:line=~'}}}'
    return 's1'
  endif
  let closes=(a:line=~'^\s*}')
  if a:line=~'{$'
    if closes
      return '='
    endif
    return 'a1'
  endif
  if closes
    return 's1'
  endif
  return '='
endfunction
set foldexpr=MyFoldExpr(getline(v:lnum))

if filereadable("~/.vimrc.site")
  source ~/.vimrc.site
endif

function! SwitchSourceHeader()
  "update!
  if (expand ("%:e") == "cc")
    find %:r.h
  else
    find %:r.cc
  endif
endfunction
autocmd filetype cpp nmap <buffer> <silent> <leader>s :call SwitchSourceHeader()<CR>

autocmd filetype html nmap <buffer> <silent> <leader>s :e %:r.js<CR>
autocmd filetype javascript nmap <buffer> <silent> <leader>s :e %:r.html<CR>

function! MySmartBraceComplete()
  if getline(line('.')-1) =~ '^\s*\(class\|struct\|enum\)'
    normal i};
  else
    normal i}
  endif
endfunction
inoremap {<CR> {<CR><Esc>:call MySmartBraceComplete()<CR>O

python << EOF
import vim

def iter_back(buffer, start):
  for ln_no in xrange(start, -1, -1):
    yield buffer[ln_no]

def get_indent(line):
  indent = 0
  while line[indent].isspace():
    indent += 1
  return indent

def find_python_ctx(buffer, start):
  func = None
  cls = None
  cur_indent = 10000000
  for line in iter_back(buffer, start):
    if line.strip() == "":
      continue
    line_indent = get_indent(line)
    if line_indent < cur_indent:
      cur_indent = line_indent
      tokens = line.split()
      if tokens[0] in set(['class', 'def']):
        name = tokens[1].split("(")[0]
        if tokens[0] == 'def':
          func = name
          if line_indent == 0:
            return (cls, func)
        elif tokens[0] == 'class':
          cls = name
          return (cls, func)
  return (cls, func)

def get_cur_python_ctx():
  row, col = vim.current.window.cursor
  return find_python_ctx(vim.current.buffer, row-1)

def print_cur_python_ctx():
  cls, func = get_cur_python_ctx()
  if func and cls is None:
    print "def %s" % func
  elif cls and func is None:
    print "class %s" % cls
  elif cls and func:
    print "%s.%s" % (cls, func)
EOF


augroup python_context
  autocmd!
  autocmd CursorMoved *.py :python print_cur_python_ctx()
augroup END

