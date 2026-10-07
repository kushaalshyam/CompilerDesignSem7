%{
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "y.tab.h"

char stmt_text[256];             /* source text of the statement just read */
static char srcline[256];        /* characters of the statement being read */

#define YY_USER_ACTION  strncat(srcline, yytext, sizeof(srcline) - strlen(srcline) - 1);

/* Move srcline into stmt_text (without the terminator) and start afresh */
static void end_statement(void)
{
    char *s = srcline;
    int n;
    while (isspace((unsigned char)*s)) s++;
    strcpy(stmt_text, s);
    n = strlen(stmt_text);
    while (n > 0 && (isspace((unsigned char)stmt_text[n-1]) || stmt_text[n-1] == ';'))
        stmt_text[--n] = '\0';
    srcline[0] = '\0';
}

static int pending(void)
{
    char *s;
    for (s = srcline; *s; s++)
        if (!isspace((unsigned char)*s)) return 1;
    return 0;
}
%}

%option yylineno

DIGIT   [0-9]
LETTER  [a-zA-Z_]

%%

"or"|"||"                           { return OR; }
"and"|"&&"                          { return AND; }
"not"|"!"                           { return NOT; }

{LETTER}({LETTER}|{DIGIT})*         { yylval.str = strdup(yytext); return ID; }
{DIGIT}+(\.{DIGIT}+)?               { yylval.str = strdup(yytext); return NUM; }

":="|"="                            { return ASSIGN; }
"<="|">="|"=="|"!="|"<"|">"         { yylval.str = strdup(yytext); return RELOP; }
[+\-*/%()]                          { return yytext[0]; }

[\n;]                               { end_statement(); return EOL; }
[ \t\r]+                            ;
.                                   { return yytext[0]; }

<<EOF>>                             { if (pending()) { end_statement(); return EOL; }
                                      return 0; }

%%

int yywrap(void)
{
    return 1;
}
