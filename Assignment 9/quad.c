/* Quadruple table : storage, temporaries, labels and printing */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "compiler.h"

Quad q[MAXQ];
int  nq = 0;

static int tcount = 0, lcount = 0;

void emit(const char *op, const char *a1, const char *a2, const char *res)
{
    if (nq >= MAXQ) {
        printf("Error: program is too long\n");
        exit(1);
    }
    strcpy(q[nq].op, op);
    strcpy(q[nq].a1, a1);
    strcpy(q[nq].a2, a2);
    strcpy(q[nq].res, res);
    q[nq].del = 0;
    nq++;
}

char *newTemp(void)
{
    char buf[16];
    sprintf(buf, "t%d", ++tcount);
    return strdup(buf);
}

char *newLabel(void)
{
    char buf[16];
    sprintf(buf, "L%d", ++lcount);
    return strdup(buf);
}

int isTemp(const char *s)
{
    int k;
    if (s[0] != 't' || !s[1]) return 0;
    for (k = 1; s[k]; k++)
        if (!isdigit((unsigned char)s[k])) return 0;
    return 1;
}

int isNum(const char *s)
{
    if (*s == '-') s++;
    if (!*s) return 0;
    while (*s)
        if (!isdigit((unsigned char)*s++)) return 0;
    return 1;
}

int isStr(const char *s)
{
    return s[0] == '"';
}

int isVar(const char *s)
{
    return (isalpha((unsigned char)s[0]) || s[0] == '_') && !isTemp(s);
}

int isBin(const char *op)
{
    return op[0] && !op[1] && strchr("+-*/%^", op[0]);
}

int isRel(const char *op)
{
    return !strcmp(op, "<") || !strcmp(op, ">") || !strcmp(op, "<=") ||
           !strcmp(op, ">=") || !strcmp(op, "==") || !strcmp(op, "!=");
}

int defines(const Quad *x)
{
    return isBin(x->op) || !strcmp(x->op, "neg") || !strcmp(x->op, "=");
}

int liveCount(void)
{
    int k, n = 0;
    for (k = 0; k < nq; k++)
        if (!q[k].del) n++;
    return n;
}

void printTAC(void)
{
    int k;
    for (k = 0; k < nq; k++) {
        const Quad *x = &q[k];
        if (x->del) continue;
        if (!strcmp(x->op, "label"))
            printf("%s:\n", x->res);
        else if (!strcmp(x->op, "goto"))
            printf("    goto %s\n", x->res);
        else if (isRel(x->op))
            printf("    if %s %s %s goto %s\n", x->a1, x->op, x->a2, x->res);
        else if (!strcmp(x->op, "print"))
            printf("    print %s\n", x->a1);
        else if (!strcmp(x->op, "="))
            printf("    %s = %s\n", x->res, x->a1);
        else if (!strcmp(x->op, "neg"))
            printf("    %s = -%s\n", x->res, x->a1);
        else
            printf("    %s = %s %s %s\n", x->res, x->a1, x->op, x->a2);
    }
}
