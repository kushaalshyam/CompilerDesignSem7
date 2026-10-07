%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "compiler.h"

int yylex(void);
void yyerror(const char *s);
extern int   yylineno;
extern char *yytext;
extern FILE *yyin;

static int phase2Started = 0;

static void startPhase2(void)
{
    if (!phase2Started) printf("\n----- PHASE 2: SYNTAX ANALYSIS -----\n");
    phase2Started = 1;
}

/* ---------------------------------------------------------------- symbol table */

#define MAXSYM 200

typedef struct {
    char name[NAMELEN];
    char type[12];
    int  bytes, addr;
    char value[NAMELEN];            /* initial value if it is a constant, else "-" */
    int  depth;                     /* block in which the variable is declared      */
    int  hidden;                    /* the block has ended: no longer visible       */
    int  assigned;                  /* a value has been given to the variable       */
} Sym;

static Sym sym[MAXSYM];
static int nsym = 0, nextAddr = 1000, depth = 0;

static int findSym(const char *name)
{
    int k;
    for (k = nsym - 1; k >= 0; k--)
        if (!sym[k].hidden && !strcmp(sym[k].name, name)) return k;
    return -1;
}

static void semError(const char *msg, const char *name)
{
    startPhase2();
    printf("Semantic error at line %d: %s '%s'\n", yylineno, msg, name);
    exit(1);
}

static void addSym(const char *name, const char *value, int assigned)
{
    if (findSym(name) >= 0)  semError("variable already declared:", name);
    if (isTemp(name))        semError("name is reserved for temporaries:", name);
    if (nsym >= MAXSYM)      semError("too many variables:", name);
    strcpy(sym[nsym].name, name);
    strcpy(sym[nsym].type, "int");
    sym[nsym].bytes = 4;
    sym[nsym].addr  = nextAddr;
    nextAddr += 4;
    strcpy(sym[nsym].value, value);
    sym[nsym].depth    = depth;
    sym[nsym].hidden   = 0;
    sym[nsym].assigned = assigned;
    nsym++;
}

static void endBlock(void)
{
    int k;
    for (k = 0; k < nsym; k++)
        if (sym[k].depth == depth) sym[k].hidden = 1;
    depth--;
}

static void checkDeclared(const char *name)
{
    if (findSym(name) < 0) semError("undeclared variable", name);
}

static void checkUse(const char *name)
{
    int k = findSym(name);
    if (k < 0)             semError("undeclared variable", name);
    if (!sym[k].assigned)  semError("variable might not have been initialized:", name);
}

static void markAssigned(const char *name)
{
    sym[findSym(name)].assigned = 1;
}

/* --------------------------------------------------------- code generation helpers */

static char *binary(const char *op, const char *a, const char *b)
{
    char *t = newTemp();
    emit(op, a, b, t);
    return t;
}

static const char *negate(const char *rel)          /* jump when the condition is false */
{
    if (!strcmp(rel, "<"))  return ">=";
    if (!strcmp(rel, ">"))  return "<=";
    if (!strcmp(rel, "<=")) return ">";
    if (!strcmp(rel, ">=")) return "<";
    if (!strcmp(rel, "==")) return "!=";
    return "==";
}
%}

%union { char *str; }

%token <str> ID NUM STR RELOP
%token USING NAMESPACE VOID MAIN INT RETURN IF ELSE WHILE COUT ENDL POWFN SHL
%type  <str> expr cond

%left  '+' '-'
%left  '*' '/' '%'
%right UMINUS

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

program
    : usings INT MAIN '(' params ')' mainblock
    ;

/* using namespace std;   (#include lines are removed by the scanner) */
usings
    : usings USING NAMESPACE ID ';'
    | /* empty */
    ;

params
    : VOID
    | /* empty */
    ;

/* body of main :  statements, then an optional  return expr;  */
mainblock
    : '{'                       { depth++; }
      stmts opt_return '}'      { endBlock(); }
    ;

opt_return
    : RETURN expr ';'
    | /* empty */
    ;

block
    : '{'                       { depth++; }
      stmts '}'                 { endBlock(); }
    ;

stmts
    : stmts stmt
    | /* empty */
    ;

stmt
    : decl ';'
    | assign
    | ifstmt
    | whilestmt
    | print
    | block
    ;

/* Declaration statement :  int a, b = 5; */
decl
    : INT declist
    ;

declist
    : declitem
    | declist ',' declitem
    ;

declitem
    : ID                        { addSym($1, "-", 0); }
    | ID '=' expr               { addSym($1, isNum($3) ? $3 : "-", 1);
                                  emit("=", $3, "", $1); }
    ;

/* Assignment statement :  a = b + 1; */
assign
    : ID '=' expr ';'           { checkDeclared($1);
                                  emit("=", $3, "", $1);
                                  markAssigned($1); }
    ;

/* Conditional statement :  if (c) s   |   if (c) s else s */
ifstmt
    : IF '(' cond ')' stmt %prec LOWER_THAN_ELSE
                                { emit("label", "", "", $3); }
    | IF '(' cond ')' stmt ELSE { char *end = newLabel();
                                  emit("goto", "", "", end);
                                  emit("label", "", "", $3);
                                  $<str>$ = end; }
      stmt                      { emit("label", "", "", $<str>7); }
    ;

/* Looping statement :  while (c) s */
whilestmt
    : WHILE                     { char *top = newLabel();
                                  emit("label", "", "", top);
                                  $<str>$ = top; }
      '(' cond ')' stmt         { emit("goto", "", "", $<str>2);
                                  emit("label", "", "", $4); }
    ;

/* Output statement :  cout << "text" << a + b << endl; */
print
    : COUT outlist ';'
    ;

outlist
    : SHL outitem
    | outlist SHL outitem
    ;

outitem
    : STR                       { emit("print", $1, "", ""); }
    | ENDL                      { emit("print", "\"\\n\"", "", ""); }
    | expr                      { emit("print", $1, "", ""); }
    ;

/* the generated jump leaves the statement when the condition is FALSE */
cond
    : expr RELOP expr           { char *out = newLabel();
                                  emit(negate($2), $1, $3, out);
                                  $$ = out; }
    ;

expr
    : expr '+' expr             { $$ = binary("+", $1, $3); }
    | expr '-' expr             { $$ = binary("-", $1, $3); }
    | expr '*' expr             { $$ = binary("*", $1, $3); }
    | expr '/' expr             { $$ = binary("/", $1, $3); }
    | expr '%' expr             { $$ = binary("%", $1, $3); }
    | POWFN '(' expr ',' expr ')'
                                { $$ = binary("^", $3, $5); }
    | '-' expr %prec UMINUS     { char *t = newTemp();
                                  emit("neg", $2, "", t);
                                  $$ = t; }
    | '(' expr ')'              { $$ = $2; }
    | NUM                       { $$ = $1; }
    | ID                        { checkUse($1); $$ = $1; }
    ;

%%

void yyerror(const char *s)
{
    startPhase2();
    printf("Syntactically incorrect: %s near '%s' at line %d\n", s, yytext, yylineno);
    exit(1);
}

int main(int argc, char *argv[])
{
    const char *path = (argc > 1) ? argv[1] : "input.cpp";
    FILE *asmFile;
    int k, before;

    yyin = fopen(path, "r");
    if (!yyin) {
        printf("Cannot open %s\n", path);
        return 1;
    }

    printf("----- PHASE 1: LEXICAL ANALYSIS -----\n");
    yyparse();
    startPhase2();
    printf("Syntactically correct\n");

    printf("\n----- SYMBOL TABLE -----\n");
    printf("%-12s %-8s %-8s %-10s %-10s\n", "Name", "Type", "Bytes", "Address", "Value");
    for (k = 0; k < nsym; k++)
        printf("%-12s %-8s %-8d %-10d %-10s\n",
               sym[k].name, sym[k].type, sym[k].bytes, sym[k].addr, sym[k].value);

    printf("\n----- PHASE 3: INTERMEDIATE CODE (TAC) -----\n");
    printTAC();
    before = liveCount();

    printf("\n----- PHASE 4: OPTIMIZED CODE -----\n");
    optimize();
    printTAC();
    printf("\n");
    printOptimizationReport();
    printf("Instructions: %d -> %d\n", before, liveCount());

    printf("\n----- PHASE 5: TARGET CODE (8086 style) -----\n");
    asmFile = fopen("output.asm", "w");
    if (!asmFile) {
        printf("Cannot create output.asm\n");
        return 1;
    }
    generate(asmFile);
    fclose(asmFile);
    printf("\nAssembly code written to output.asm\n");
    return 0;
}
