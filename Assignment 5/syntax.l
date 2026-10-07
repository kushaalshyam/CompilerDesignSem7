%{
#include "y.tab.h"
%}

%option yylineno noyywrap nounput noinput

DIGIT   [0-9]
LETTER  [a-zA-Z_]

%%

"//".*                              ;
"/*"([^*]|\*+[^*/])*\*+"/"          ;

^[ \t]*"#"[^\n]*                    ;
"using"[ \t]+"namespace"[ \t]+{LETTER}({LETTER}|{DIGIT})*[ \t]*";"   ;

"int"|"float"|"char"|"double"|"bool" { return TYPE; }
"return"                            { return RETURN; }
"std::"?"cout"                      { return COUT; }
"std::"?"cin"                       { return CIN; }
"std::"?"endl"                      { return ENDL; }
"true"|"false"                      { return NUM; }
"if"                                { return IF; }
"else"                              { return ELSE; }
"while"                             { return WHILE; }
"do"                                { return DO; }
"for"                               { return FOR; }

{LETTER}({LETTER}|{DIGIT})*         { return ID; }
{DIGIT}+(\.{DIGIT}+)?               { return NUM; }
\"([^"\\\n]|\\.)*\"                { return STR; }
\'([^'\\]|\\.)\'                   { return NUM; }

"++"                                { return INC; }
"--"                                { return DEC; }
"<<"                                { return SHL; }
">>"                                { return SHR; }
"&&"                                { return AND; }
"||"                                { return OR; }
"=="|"!="                           { return EQOP; }
"<="|">="|"<"|">"                   { return RELOP; }
"!"                                 { return NOT; }
"="                                 { return '='; }
[+\-*/%]                            { return yytext[0]; }
[(){};,]                            { return yytext[0]; }

[ \t\r\n]                           ;
.                                   { return yytext[0]; }

%%

