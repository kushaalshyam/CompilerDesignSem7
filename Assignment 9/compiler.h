/*
 * Mini compiler - shared declarations
 *
 * Intermediate code is kept as a table of quadruples  (op, arg1, arg2, result):
 *
 *     +  -  *  /  %  ^       result = arg1 op arg2
 *     neg                     result = -arg1
 *     =                       result = arg1
 *     <  <=  >  >=  ==  !=    if arg1 op arg2 goto result
 *     goto                    goto result
 *     label                   result :
 *     print                   print arg1   (cout << arg1)
 */
#ifndef COMPILER_H
#define COMPILER_H

#include <stdio.h>

#define MAXQ    1000                /* maximum number of quadruples        */
#define NAMELEN 128                 /* maximum length of a name or string  */

typedef struct {
    char op[8];
    char a1[NAMELEN], a2[NAMELEN], res[NAMELEN];
    int  del;                       /* 1 = removed by the optimizer        */
} Quad;

extern Quad q[MAXQ];
extern int  nq;

/* quad.c */
void emit(const char *op, const char *a1, const char *a2, const char *res);
char *newTemp(void);
char *newLabel(void);
int  isTemp(const char *s);         /* t1, t2, ...                         */
int  isNum(const char *s);          /* integer constant                    */
int  isStr(const char *s);          /* "string constant"                   */
int  isVar(const char *s);          /* name of a source variable           */
int  isBin(const char *op);         /* + - * / % ^                         */
int  isRel(const char *op);         /* < <= > >= == !=                     */
int  defines(const Quad *x);        /* quad assigns a value to x->res      */
void printTAC(void);
int  liveCount(void);

/* optimizer.c */
void optimize(void);
void printOptimizationReport(void);

/* codegen.c */
void generate(FILE *asmFile);

#endif
