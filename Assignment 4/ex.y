%{
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int yylex(void);
void yyerror(char *s);
%}

%union {
    double dval;
}

%token <dval> NUMBER
%type  <dval> expr

%left '+' '-'
%left '*' '/'
%right '^'

%start program

%%

program : program line
        |
        ;

line    : expr '\n'    { printf("Result = %g\n", $1); }
        | '\n'
        | error '\n'    { yyerrok; }
        ;

expr    : expr '+' expr    { $$ = $1 + $3; }
        | expr '-' expr    { $$ = $1 - $3; }
        | expr '*' expr    { $$ = $1 * $3; }
        | expr '/' expr    { $$ = $1 / $3; }
        | expr '^' expr    { $$ = pow($1, $3); }
        | '(' expr ')'     { $$ = $2; }
        | NUMBER            { $$ = $1; }
        ;

%%

void yyerror(char *s) {
    printf("%s\n", s);
}

int main(void) {
    printf("Enter expression:\n");
    yyparse();
    return 0;
}
