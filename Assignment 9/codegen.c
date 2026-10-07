/*
 * Code generator : optimized quadruples -> 8086 style assembly code
 *
 * Registers : every variable of the source program owns a register (R0, R1, ...).
 *             Temporaries share the registers that follow: a temporary gets a free
 *             register when it is defined and gives it back after its last use, and a
 *             result may reuse the register of an operand that dies in the same quad.
 * Constants : a constant operand is first moved to a scratch register (MOV R, #n).
 * Power     : x ^ n is expanded into repeated MUL (small constant n) or into a loop.
 * Output    : the assembly code is printed and also written to output.asm
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "compiler.h"

#define MAXR 600

static FILE *out;

static char vname[MAXR][NAMELEN];           /* source variables -> R0 .. R(nvar-1) */
static int  nvar = 0;

typedef struct { char name[NAMELEN]; int reg; int last; } Temp;
static Temp tmp[MAXQ];                      /* temporaries: register and last use  */
static int  ntmp = 0;

static char busy[MAXR];                     /* register in use (temporaries only)  */
static int  maxReg = -1;
static int  scratch[8], nscratch = 0;
static int  powLabels = 0;

/* ---------------------------------------------------------------------- output */

static void ins(const char *fmt, ...)
{
    char buf[200];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (buf[strlen(buf) - 1] == ':') {
        printf("%s\n", buf);
        fprintf(out, "%s\n", buf);
    } else {
        printf("    %s\n", buf);
        fprintf(out, "    %s\n", buf);
    }
}

/* -------------------------------------------------------------------- registers */

static int varReg(const char *n)
{
    int k;
    for (k = 0; k < nvar; k++)
        if (!strcmp(vname[k], n)) return k;
    return -1;
}

static void addVar(const char *n)
{
    if (isVar(n) && varReg(n) < 0) strcpy(vname[nvar++], n);
}

static Temp *findTemp(const char *n)
{
    int k;
    for (k = 0; k < ntmp; k++)
        if (!strcmp(tmp[k].name, n)) return &tmp[k];
    strcpy(tmp[ntmp].name, n);
    tmp[ntmp].reg  = -1;
    tmp[ntmp].last = -1;
    return &tmp[ntmp++];
}

static int regOf(const char *n)
{
    return isTemp(n) ? findTemp(n)->reg : varReg(n);
}

static int allocReg(void)
{
    int r;
    for (r = nvar; r < MAXR; r++)
        if (!busy[r]) {
            busy[r] = 1;
            if (r > maxReg) maxReg = r;
            return r;
        }
    printf("Error: out of registers\n");
    exit(1);
}

static int newScratch(void)
{
    int r = allocReg();
    scratch[nscratch++] = r;
    return r;
}

static void loadTo(int r, const char *a)
{
    if (isNum(a)) ins("MOV R%d, #%s", r, a);
    else if (regOf(a) != r) ins("MOV R%d, R%d", r, regOf(a));
}

static int operandReg(const char *a)            /* register that holds operand a */
{
    int r;
    if (!isNum(a)) return regOf(a);
    r = newScratch();
    ins("MOV R%d, #%s", r, a);
    return r;
}

/* Register for the result of quad i.  A temporary that dies in this quad lends its
   register to the result. */
static int destReg(int i, const Quad *x)
{
    Temp *t;
    const char *ops[2];
    int k;
    if (!isTemp(x->res)) return varReg(x->res);
    t = findTemp(x->res);
    ops[0] = x->a1; ops[1] = x->a2;
    for (k = 0; k < 2; k++)
        if (isTemp(ops[k]) && findTemp(ops[k])->last == i) {
            t->reg = findTemp(ops[k])->reg;
            return t->reg;
        }
    t->reg = allocReg();
    return t->reg;
}

static void finish(int i, const Quad *x, int d)
{
    const char *ops[2];
    int k;
    ops[0] = x->a1; ops[1] = x->a2;
    for (k = 0; k < 2; k++)
        if (isTemp(ops[k]) && findTemp(ops[k])->last == i && findTemp(ops[k])->reg != d)
            busy[findTemp(ops[k])->reg] = 0;
    if (isTemp(x->res) && findTemp(x->res)->last < 0) busy[d] = 0;     /* never used */
    for (k = 0; k < nscratch; k++) busy[scratch[k]] = 0;
    nscratch = 0;
}

/* ------------------------------------------------------------------ instructions */

static const char *mnemonic(char op)
{
    switch (op) {
    case '+': return "ADD";
    case '-': return "SUB";
    case '*': return "MUL";
    case '/': return "DIV";
    default : return "MOD";
    }
}

static const char *jumpFor(const char *rel)
{
    if (!strcmp(rel, "<"))  return "JL";
    if (!strcmp(rel, ">"))  return "JG";
    if (!strcmp(rel, "<=")) return "JLE";
    if (!strcmp(rel, ">=")) return "JGE";
    if (!strcmp(rel, "==")) return "JE";
    return "JNE";
}

/* d = a1 ^ a2 */
static void genPower(int d, const Quad *x)
{
    int base = newScratch(), acc = newScratch();
    int e = isNum(x->a2) ? atoi(x->a2) : -1;

    if (isNum(x->a2) && e >= 2 && e <= 16) {             /* repeated multiplication */
        int k;
        loadTo(base, x->a1);
        ins("MOV R%d, R%d", acc, base);
        for (k = 1; k < e; k++) ins("MUL R%d, R%d", acc, base);
    } else if (isNum(x->a2) && e <= 1) {                 /* x ^ 1 or x ^ 0 */
        if (e == 1) loadTo(acc, x->a1);
        else        ins("MOV R%d, #1", acc);
    } else {                                             /* loop */
        int cnt = newScratch(), zero = newScratch(), one = newScratch();
        int n = ++powLabels;
        loadTo(base, x->a1);
        loadTo(cnt, x->a2);
        ins("MOV R%d, #1", acc);
        ins("MOV R%d, #0", zero);
        ins("MOV R%d, #1", one);
        ins("P%d:", n);
        ins("CMP R%d, R%d", cnt, zero);
        ins("JLE P%d_end", n);
        ins("MUL R%d, R%d", acc, base);
        ins("SUB R%d, R%d", cnt, one);
        ins("JMP P%d", n);
        ins("P%d_end:", n);
    }
    ins("MOV R%d, R%d", d, acc);
}

static void genBinary(int d, const Quad *x)
{
    const char *m = mnemonic(x->op[0]);
    int r1 = isNum(x->a1) ? -1 : regOf(x->a1);
    int r2 = isNum(x->a2) ? -1 : regOf(x->a2);
    int commutative = (x->op[0] == '+' || x->op[0] == '*');

    if (r2 == d && r1 != d) {                   /* the result register holds operand 2 */
        if (commutative) {
            ins("%s R%d, R%d", m, d, operandReg(x->a1));
        } else {
            int s = newScratch();
            loadTo(s, x->a1);
            ins("%s R%d, R%d", m, s, d);
            ins("MOV R%d, R%d", d, s);
        }
    } else {
        loadTo(d, x->a1);
        ins("%s R%d, R%d", m, d, operandReg(x->a2));
    }
}

void generate(FILE *asmFile)
{
    int i, k;
    out = asmFile;

    /* registers of the variables (in order of first appearance) and last use of temporaries */
    for (i = 0; i < nq; i++) {
        const Quad *x = &q[i];
        if (x->del) continue;
        if (defines(x)) addVar(x->res);
        if (defines(x) || isRel(x->op) || !strcmp(x->op, "print")) addVar(x->a1);
        if (isBin(x->op) || isRel(x->op)) addVar(x->a2);
        if (isTemp(x->a1)) findTemp(x->a1)->last = i;
        if ((isBin(x->op) || isRel(x->op)) && isTemp(x->a2)) findTemp(x->a2)->last = i;
    }

    for (i = 0; i < nq; i++) {
        const Quad *x = &q[i];
        int d = -1;
        if (x->del) continue;

        if (!strcmp(x->op, "label")) {
            ins("%s:", x->res);
        } else if (!strcmp(x->op, "goto")) {
            ins("JMP %s", x->res);
        } else if (isRel(x->op)) {
            int r1 = operandReg(x->a1), r2 = operandReg(x->a2);
            ins("CMP R%d, R%d", r1, r2);
            ins("%s %s", jumpFor(x->op), x->res);
        } else if (!strcmp(x->op, "print")) {
            if (isStr(x->a1))      ins("OUT %s", x->a1);
            else if (isNum(x->a1)) ins("OUT #%s", x->a1);
            else                   ins("OUT R%d", regOf(x->a1));
        } else if (!strcmp(x->op, "=")) {
            d = destReg(i, x);
            loadTo(d, x->a1);
        } else if (!strcmp(x->op, "neg")) {
            d = destReg(i, x);
            loadTo(d, x->a1);
            ins("NEG R%d", d);
        } else if (x->op[0] == '^') {
            d = destReg(i, x);
            genPower(d, x);
        } else {
            d = destReg(i, x);
            genBinary(d, x);
        }
        finish(i, x, d);
    }

    printf("\nRegister Allocation\n");
    for (k = 0; k < nvar; k++) printf("    %-10s -> R%d\n", vname[k], k);
    if (maxReg >= nvar) printf("    temporaries  -> R%d .. R%d\n", nvar, maxReg);
    else                printf("    temporaries  -> none needed\n");
}
