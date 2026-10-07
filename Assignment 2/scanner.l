%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ---------- symbol table ---------- */
typedef struct {
    char name[32];
    char type[12];
    int  bytes;
    char value[32];
} SymEntry;

SymEntry table[100];
int symCount = 0;

char activeType[12] = "";   /* type currently being declared on this line */
int  lastIdIndex = -1;      /* index of most recently declared identifier */
int  awaitingEquals = 0;    /* just saw an identifier that could get "= value" */
int  awaitingValue  = 0;    /* just saw "=" after that identifier */

int sizeOfType(char *t) {
    if (!strcmp(t, "char"))   return 1;
    if (!strcmp(t, "bool"))   return 1;
    if (!strcmp(t, "float"))  return 4;
    if (!strcmp(t, "double")) return 8;
    return 2;
}

void recordSymbol(char *name) {
    awaitingEquals = 0;
    if (activeType[0] == '\0') { lastIdIndex = -1; return; }
    strcpy(table[symCount].name, name);
    strcpy(table[symCount].type, activeType);
    table[symCount].bytes = sizeOfType(activeType);
    strcpy(table[symCount].value, "-");
    lastIdIndex = symCount;
    awaitingEquals = 1;
    symCount++;
}

void captureValue(char *val) {
    if (awaitingValue && lastIdIndex >= 0) {
        strcpy(table[lastIdIndex].value, val);
    }
    awaitingValue = 0;
    lastIdIndex = -1;
}

void printSymbolTable(void) {
    printf("\nContent of Symbol Table\n");
    printf("%-15s %-8s %-12s %-10s\n", "Identifier", "Type", "No of bytes", "Value");
    for (int k = 0; k < symCount; k++)
        printf("%-15s %-8s %-12d %-10s\n",
               table[k].name, table[k].type, table[k].bytes, table[k].value);
}
%}

%option nounput noinput
%x COMMENT

DIGIT     [0-9]
LETTER    [a-zA-Z_]
HEX       [0-9a-fA-F]

%%

"//".*                              { /* single line comment: ignore */ }

"/*"                                 { BEGIN(COMMENT); }
<COMMENT>"*/"                        { BEGIN(INITIAL); }
<COMMENT>\n                          { /* stay in comment */ }
<COMMENT>.                           { /* consume comment body */ }
<COMMENT><<EOF>>                     {
                                        printf("Error: unterminated comment\n");
                                        yyterminate();
                                      }

"auto"|"break"|"case"|"catch"|"class"|"const"|"constexpr"|"continue"|"default"|"delete"|"do"|"else"|"enum"|"explicit"|"extern"|"for"|"friend"|"goto"|"if"|"inline"|"mutable"|"namespace"|"new"|"nullptr"|"operator"|"private"|"protected"|"public"|"register"|"return"|"sizeof"|"static"|"struct"|"switch"|"template"|"this"|"throw"|"try"|"typedef"|"typename"|"union"|"unsigned"|"using"|"virtual"|"void"|"volatile"|"while"|"signed" {
                                        printf("%s - keyword\n", yytext);
                                        activeType[0] = '\0';
                                      }

"int"|"char"|"float"|"double"|"long"|"short"|"bool" {
                                        printf("%s - keyword\n", yytext);
                                        strcpy(activeType, yytext);
                                      }

^[ \t]*"#"[^\n]*                    { printf("%s - preprocessor directive\n", yytext + strspn(yytext, " \t")); }

"std::"?"cout"                       { printf("%s - output stream (function call)\n", yytext); }
"std::"?"cin"                        { printf("%s - input stream (function call)\n", yytext); }
"std::"?"endl"                       { printf("%s - stream manipulator\n", yytext); }

"printf"[ \t]*"("[^)]*")"            { printf("%s - function call\n", yytext); }
"scanf"[ \t]*"("[^)]*")"             { printf("%s - function call\n", yytext); }
"getch"[ \t]*"("[^)]*")"             { printf("%s - function call\n", yytext); }
"clrscr"[ \t]*"("[^)]*")"            { printf("%s - function call\n", yytext); }
"main"[ \t]*"("[^)]*")"              { printf("%s - function call\n", yytext); }

"true"|"false"                       { printf("%s - boolean constant\n", yytext); captureValue(yytext); }

{LETTER}({LETTER}|{DIGIT})*          {
                                        printf("%s - identifier\n", yytext);
                                        recordSymbol(yytext);
                                      }

"0"[xX]{HEX}+                        { printf("%s - hexadecimal constant\n", yytext); captureValue(yytext); }

{DIGIT}+"."{DIGIT}+                  { printf("%s - double constant\n", yytext); captureValue(yytext); }

{DIGIT}+                             { printf("%s - integer constant\n", yytext); captureValue(yytext); }

\"([^"\n])*\"                        { printf("%s - string constant\n", yytext); captureValue(yytext); }

\'([^'\\]|\\.)\'                     { printf("%s - character constant\n", yytext); captureValue(yytext); }

"+="|"-="|"*="|"/="|"%="             { printf("%s - arithmetic assignment operator\n", yytext); awaitingEquals = 0; awaitingValue = 0; }
"++"|"--"                            { printf("%s - unary operator\n", yytext); }
"&&"                                 { printf("&& - logical operator\n"); }
"||"                                 { printf("|| - logical operator\n"); }
"=="                                 { printf("== - relational operator\n"); }
"!="                                 { printf("!= - relational operator\n"); }
"<="                                 { printf("<= - relational operator\n"); }
">="                                 { printf(">= - relational operator\n"); }
"<<"                                 { printf("<< - bitwise / stream insertion operator\n"); }
">>"                                 { printf(">> - bitwise / stream extraction operator\n"); }
"::"                                 { printf(":: - scope resolution operator\n"); }
"->"                                 { printf("-> - member access operator\n"); }

[+\-*/%]                             { printf("%c - arithmetic operator\n", yytext[0]); }
[<>]                                 { printf("%c - relational operator\n", yytext[0]); }
"="                                  {
                                        printf("= - assignment operator\n");
                                        if (awaitingEquals) { awaitingValue = 1; awaitingEquals = 0; }
                                      }
"!"                                  { printf("! - logical operator\n"); }
[&|^]                                { printf("%c - bitwise operator\n", yytext[0]); }

[;,.\[\](){}?:]                      {
                                        printf("%c - special character\n", yytext[0]);
                                        if (yytext[0] == ';') activeType[0] = '\0';
                                      }

[ \t\n]+                             { /* skip whitespace */ }

.                                    { /* skip anything unrecognized */ }

%%

int yywrap(void) { return 1; }
int main(void) {
    yylex();
    printSymbolTable();
    return 0;
}