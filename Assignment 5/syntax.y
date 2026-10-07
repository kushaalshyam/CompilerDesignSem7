%{
#include <stdio.h>

int yylex(void);
void yyerror(const char *s);

extern int   yylineno;
extern char *yytext;
%}

%token TYPE IF ELSE WHILE DO FOR ID NUM STR
%token RETURN COUT CIN ENDL SHL SHR
%token INC DEC AND OR EQOP RELOP NOT

%left  OR
%left  AND
%left  EQOP
%left  RELOP
%left  '+' '-'
%left  '*' '/' '%'
%right NOT UMINUS

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

program     : stmt_list
            | function
            ;

/* int main() { ... }      (optionally  int main(void) ) */
function    : TYPE ID '(' ')' block
            | TYPE ID '(' TYPE ')' block
            ;

stmt_list   : stmt_list stmt
            | stmt
            ;

stmt        : decl_stmt
            | assign_stmt
            | if_stmt
            | while_stmt
            | do_stmt
            | for_stmt
            | io_stmt
            | return_stmt
            | block
            | ';'
            ;

/* Output / input statements:  cout << a << "text" << endl;   cin >> a; */
io_stmt     : COUT out_list ';'
            | CIN in_list ';'
            ;

out_list    : SHL out_item
            | out_list SHL out_item
            ;

out_item    : STR
            | ENDL
            | expr
            ;

in_list     : SHR ID
            | in_list SHR ID
            ;

return_stmt : RETURN expr ';'
            | RETURN ';'
            ;

block       : '{' stmt_list '}'
            | '{' '}'
            ;

/* Declaration statement:  int i, a = 5; */
decl_stmt   : TYPE decl_list ';'
            ;

decl_list   : decl_list ',' declarator
            | declarator
            ;

declarator  : ID
            | ID '=' expr
            ;

/* Assignment statement:  i = i + 1;   i++; */
assign_stmt : simple_stmt ';'
            ;

simple_stmt : ID '=' expr
            | ID INC
            | ID DEC
            ;

/* Conditional statements:  if, if-else */
if_stmt     : IF '(' expr ')' stmt %prec LOWER_THAN_ELSE
            | IF '(' expr ')' stmt ELSE stmt
            ;

/* Looping statements:  while, do-while, for */
while_stmt  : WHILE '(' expr ')' stmt
            ;

do_stmt     : DO stmt WHILE '(' expr ')' ';'
            ;

for_stmt    : FOR '(' simple_stmt ';' expr ';' simple_stmt ')' stmt
            ;

expr        : expr OR expr
            | expr AND expr
            | expr EQOP expr
            | expr RELOP expr
            | expr '+' expr
            | expr '-' expr
            | expr '*' expr
            | expr '/' expr
            | expr '%' expr
            | NOT expr
            | '-' expr %prec UMINUS
            | '(' expr ')'
            | ID
            | NUM
            ;

%%

void yyerror(const char *s) {
    printf("Syntax Error at line %d near '%s'\n", yylineno, yytext);
}

int main(void) {
    if (yyparse() == 0)
        printf("Syntactically correct\n");
    return 0;
}
