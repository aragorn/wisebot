" Vim syntax file 
" Language:     SoftBot config files (Apache, ProFTPd, etc) 
" Maintainer:   Jung-gyum Kim <aragorn@softwise.co.kr> 
" URL:          none 
" Last Change:  2002 Nov 14 
 
" SoftBot config files look this way: 
" 
" Option        value 
" Option        value1 value2 
" <module.c> 
"       Option  value 
"       # Comments... 
" </module.c> 
 
" Remove any old syntax stuff hanging around 
syn clear 
syn case ignore 
 
" useful keywords 
syn keyword     sbTodo          contained TODO FIXME XXX 
 
" specials 
syn match  sbComment    /^\s*#.*/ contains=sbTodo 
 
" options and values 
"syn cluster sbConfig   contains=sbConfigN,sbValue,sbSecondValue 
syn region sbConfig     start=/^\s*[A-Za-z]/ end=/$/ contains=sbConfigN 
syn match  sbConfigN    contained /[a-zA-Z0-9-_]\+/ skipwhite nextgroup=sbValue 
syn match  sbValue      contained /[^ \t]\+/ skipwhite nextgroup=sbSecondValue 
"syn match  sbString    contained /"[^"]*"/ nextgroup=sbSecondValue 
syn match  sbSecondValue        contained /[^ \t]\+/ skipnl nextgroup=sbSecondValue 
 
" tags 
syn region sbModule     start=/</ end=/>/ contains=sbModuleN,sbModuleError 
syn match  sbModule     /<.*>/ms=s+1,me=e-1 
syn match  sbModuleN    contained /[a-zA-Z0-9-_]\+\.c/ms=s+1 
syn match  sbModuleError        contained /[^>]</ms=s+1 
 
if !exists("did_softbot_syntax_inits") 
  let did_softbot_syntax_inits = 1 
  hi Special term=bold cterm=bold 
 
  hi link sbComment                     Comment 
"  hi link sbConfig                     String 
  hi link sbConfigN                     Type 
  hi link sbValue                       Label 
  hi link sbString                      String 
  hi link sbModuleN                     Identifier 
  hi link sbModuleError                 Error 
  hi link sbTodo                        Error 
 
endif 
 
let b:current_syntax = "softbot" 
" vim: ts=8 
