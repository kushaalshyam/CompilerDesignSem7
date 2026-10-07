%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

int yylex(void);
void yyerror(const char *s);

extern int  yylineno;
extern char stmt_text[];

#define BASE    100             /* number given to the first instruction   */
#define MAXCODE 100

static char code[MAXCODE][80];  /* three address code of the current statement */
static int  ncode   = 0;        /* instructions generated so far           */
static int  ntemp   = 0;        /* temporaries used so far                 */
static int  hasBool = 0;        /* statement contains a boolean expression */

static char *newtemp(void)
{
    char buf[16];
    sprintf(buf, "t%d", ++ntemp);
    return strdup(buf);
}

static void emit(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(code[ncode++], sizeof(code[0]), fmt, ap);
    va_end(ap);
}

static void reset(void)
{
    ncode = ntemp = hasBool = 0;
}

/* Print the code of one statement: numbered from 100 only if it has booleans */
static void flush_statement(void)
{
    int k;
    printf("%s\n", stmt_text);
    for (k = 0; k < (int)strlen(stmt_text); k++) putchar('-');
    putchar('\n');
    for (k = 0; k < ncode; k++) {
        if (hasBool) printf("%d: %s\n", BASE + k, code[k]);
        else         printf("%s\n", code[k]);
    }
    putchar('\n');
    reset();
}

/* result = x op y  for arithmetic operators */
static char *arith(char *x, char op, char *y)
{
    char *t = newtemp();
    emit("%s = %s %c %s", t, x, op, y);
    return t;
}

/* result = x relop y : four instructions giving the value 0 or 1 */
static char *relational(char *x, char *op, char *y)
{
    char *t = newtemp();
    int   a = BASE + ncode;             /* number of the 'if' instruction */
    hasBool = 1;
    emit("if %s %s %s goto %d", x, op, y, a + 3);
    emit("%s := 0", t);
    emit("goto %d", a + 4);
    emit("%s := 1", t);
    return t;
}

/* result = x and y  /  x or y */
static char *logical(char *x, const char *op, char *y)
{
    char *t = newtemp();
    hasBool = 1;
    emit("%s := %s %s %s", t, x, op, y);
    return t;
}
%}

%union { char *str; }

%token <str> ID NUM RELOP
%token ASSIGN OR AND NOT EOL

%type <str> expr

%left  OR
%left  AND
%right NOT
%nonassoc RELOP
%left  '+' '-'
%left  '*' '/' '%'
%right UMINUS

%%

program : program stmt
        |
        ;

stmt    : ID ASSIGN expr EOL      { emit("%s = %s", $1, $3); flush_statement(); }
        | EOL
        | error EOL               { reset(); yyerrok; }
        ;

expr    : expr OR expr            { $$ = logical($1, "or",  $3); }
        | expr AND expr           { $$ = logical($1, "and", $3); }
        | NOT expr                { $$ = newtemp(); hasBool = 1;
                                    emit("%s := not %s", $$, $2); }
        | expr RELOP expr         { $$ = relational($1, $2, $3); }
        | expr '+' expr           { $$ = arith($1, '+', $3); }
        | expr '-' expr           { $$ = arith($1, '-', $3); }
        | expr '*' expr           { $$ = arith($1, '*', $3); }
        | expr '/' expr           { $$ = arith($1, '/', $3); }
        | expr '%' expr           { $$ = arith($1, '%', $3); }
        | '-' expr %prec UMINUS   { $$ = newtemp(); emit("%s = -%s", $$, $2); }
        | '(' expr ')'            { $$ = $2; }
        | ID                      { $$ = $1; }
        | NUM                     { $$ = $1; }
        ;

%%

void yyerror(const char *s)
{
    printf("Syntax error at line %d\n\n", yylineno);
}

int main(void)
{
    yyparse();
    return 0;
}
