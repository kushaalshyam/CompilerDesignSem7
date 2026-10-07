/*
 * Code optimizer for the quadruple table
 *
 *   1. Constant propagation and constant folding   (data flow analysis, works across loops)
 *   2. Algebraic identities       x+0  x*1  x*0  x-x  x^1  x^0 ...
 *   3. Strength reduction         x^2 -> x*x      x*2 -> x+x
 *   4. Merging of temporaries     t = a+b ; x = t  ->  x = a+b
 *   5. Common sub-expression elimination (inside a basic block)
 *   6. Copy propagation           t2 = t1
 *   7. Dead code elimination      unused temporaries, jumps to the next statement,
 *                                 unreferenced labels, unreachable code
 *
 * The passes are repeated until nothing changes, because one pass often creates a
 * chance for another one.  Variables of the source program are treated as live at the
 * end of the program, so only temporaries and unreachable code are ever deleted.
 * All arithmetic uses 32 bit int semantics, like C++ on a machine with 32 bit int.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "compiler.h"

#define MAXV 400                    /* variables and temporaries */

enum { S_PROP, S_FOLD, S_ALG, S_STR, S_CSE, S_COPY, S_MERGE, S_DEAD, S_UNREACH, S_COUNT };

static const char *title[S_COUNT] = {
    "Constant propagation", "Constant folding", "Algebraic identities",
    "Strength reduction", "Common sub-expression elimination", "Copy propagation",
    "Merging of temporaries", "Dead code elimination", "Unreachable code elimination"
};
static int count[S_COUNT];

typedef int32_t i32;

/* ------------------------------------------------------------------ arithmetic */

static i32 wrap(long long v)
{
    return (i32)(uint32_t)v;
}

static i32 power(i32 base, i32 e)           /* e <= 0 gives 1 */
{
    uint32_t r = 1, x = (uint32_t)base;
    uint32_t n = e > 0 ? (uint32_t)e : 0;
    while (n) {
        if (n & 1) r *= x;
        x *= x;
        n >>= 1;
    }
    return (i32)r;
}

static int evaluate(const char *op, i32 a, i32 b, i32 *r)
{
    switch (op[0]) {
    case '+': *r = wrap((long long)a + b); return 1;
    case '-': *r = wrap((long long)a - b); return 1;
    case '*': *r = wrap((long long)a * b); return 1;
    case '/': if (b == 0 || (a == INT32_MIN && b == -1)) return 0;
              *r = a / b; return 1;
    case '%': if (b == 0 || (a == INT32_MIN && b == -1)) return 0;
              *r = a % b; return 1;
    case '^': *r = power(a, b); return 1;
    }
    return 0;
}

static void setCopy(Quad *x, const char *v)
{
    char tmp[NAMELEN];
    strcpy(tmp, v);                         /* v may point inside x */
    strcpy(x->op, "=");
    strcpy(x->a1, tmp);
    x->a2[0] = '\0';
}

static int hasA1(const Quad *x)
{
    return defines(x) || isRel(x->op) || !strcmp(x->op, "print");
}

static int hasA2(const Quad *x)
{
    return isBin(x->op) || isRel(x->op);
}

static int nextLive(int i)
{
    for (i++; i < nq; i++)
        if (!q[i].del) return i;
    return -1;
}

static int labelIndex(const char *name)
{
    int k;
    for (k = 0; k < nq; k++)
        if (!q[k].del && !strcmp(q[k].op, "label") && !strcmp(q[k].res, name)) return k;
    return -1;
}

/* ------------------------------------------- 1. constant propagation and folding */

typedef struct { int k; i32 v; } Val;       /* k: 1 = constant v, 2 = not a constant */

static char vname[MAXV][NAMELEN];
static int  nv = 0;
static Val  in[MAXQ][MAXV];                 /* value of every variable before quad i */
static char reached[MAXQ];
static int  succ[MAXQ][2];

static int varIndex(const char *n)
{
    int k;
    for (k = 0; k < nv; k++)
        if (!strcmp(vname[k], n)) return k;
    if (nv >= MAXV) {
        printf("Error: too many variables\n");
        exit(1);
    }
    strcpy(vname[nv], n);
    return nv++;
}

static Val getVal(const Val *st, const char *s)
{
    Val r = { 2, 0 };
    if (isNum(s)) { r.k = 1; r.v = wrap(atoll(s)); }
    else if (isVar(s) || isTemp(s)) r = st[varIndex(s)];
    return r;
}

static void transfer(Val *st, const Quad *x)
{
    Val r = { 2, 0 }, a, b;
    i32 v;
    if (!defines(x)) return;
    a = getVal(st, x->a1);
    if (!strcmp(x->op, "=")) {
        if (a.k == 1) r = a;
    } else if (!strcmp(x->op, "neg")) {
        if (a.k == 1) { r.k = 1; r.v = wrap(-(long long)a.v); }
    } else {
        b = getVal(st, x->a2);
        if (a.k == 1 && b.k == 1 && evaluate(x->op, a.v, b.v, &v)) { r.k = 1; r.v = v; }
    }
    st[varIndex(x->res)] = r;
}

static int meet(Val *dst, const Val *src)   /* returns 1 if dst changed */
{
    int v, changed = 0;
    for (v = 0; v < nv; v++) {
        if (dst[v].k == 2) continue;
        if (src[v].k == 2 || dst[v].v != src[v].v) { dst[v].k = 2; changed = 1; }
    }
    return changed;
}

static void substitute(const Val *st, char *name, int *changes)
{
    char num[32];
    Val r;
    if (!isVar(name) && !isTemp(name)) return;
    r = st[varIndex(name)];
    if (r.k != 1) return;
    sprintf(num, "%d", (int)r.v);
    strcpy(name, num);
    count[S_PROP]++;
    (*changes)++;
}

static int constantPropagation(void)
{
    Val out[MAXV];
    int i, v, s, first, changed, changes = 0;

    for (i = 0; i < nq; i++) {                      /* make a slot for every name */
        if (q[i].del) continue;
        if (defines(&q[i])) varIndex(q[i].res);
        if (hasA1(&q[i]) && (isVar(q[i].a1) || isTemp(q[i].a1))) varIndex(q[i].a1);
        if (hasA2(&q[i]) && (isVar(q[i].a2) || isTemp(q[i].a2))) varIndex(q[i].a2);
    }

    /* control flow graph */
    for (i = 0; i < nq; i++) {
        reached[i] = 0;
        succ[i][0] = succ[i][1] = -1;
        if (q[i].del) continue;
        if (!strcmp(q[i].op, "goto")) {
            succ[i][0] = labelIndex(q[i].res);
        } else if (isRel(q[i].op)) {
            succ[i][0] = nextLive(i);
            succ[i][1] = labelIndex(q[i].res);
        } else {
            succ[i][0] = nextLive(i);
        }
    }

    first = nextLive(-1);
    if (first < 0) return 0;
    reached[first] = 1;
    for (v = 0; v < nv; v++) { in[first][v].k = 2; in[first][v].v = 0; }

    do {                                            /* iterate until nothing changes */
        changed = 0;
        for (i = 0; i < nq; i++) {
            if (q[i].del || !reached[i]) continue;
            memcpy(out, in[i], sizeof(Val) * nv);
            transfer(out, &q[i]);
            for (s = 0; s < 2; s++) {
                int t = succ[i][s];
                if (t < 0) continue;
                if (!reached[t]) {
                    reached[t] = 1;
                    memcpy(in[t], out, sizeof(Val) * nv);
                    changed = 1;
                } else if (meet(in[t], out)) {
                    changed = 1;
                }
            }
        }
    } while (changed);

    for (i = 0; i < nq; i++) {                      /* rewrite the code */
        Quad *x = &q[i];
        if (x->del) continue;
        if (!reached[i]) { x->del = 1; count[S_UNREACH]++; changes++; continue; }

        if (hasA1(x)) substitute(in[i], x->a1, &changes);
        if (hasA2(x)) substitute(in[i], x->a2, &changes);

        if (isBin(x->op) && isNum(x->a1) && isNum(x->a2)) {
            i32 r;
            if (evaluate(x->op, wrap(atoll(x->a1)), wrap(atoll(x->a2)), &r)) {
                char num[32];
                sprintf(num, "%d", (int)r);
                setCopy(x, num);
                count[S_FOLD]++; changes++;
            }
        } else if (!strcmp(x->op, "neg") && isNum(x->a1)) {
            char num[32];
            sprintf(num, "%d", (int)wrap(-atoll(x->a1)));
            setCopy(x, num);
            count[S_FOLD]++; changes++;
        } else if (isRel(x->op) && isNum(x->a1) && isNum(x->a2)) {
            i32 a = wrap(atoll(x->a1)), b = wrap(atoll(x->a2));
            int t = !strcmp(x->op, "<")  ? a < b  : !strcmp(x->op, ">")  ? a > b  :
                    !strcmp(x->op, "<=") ? a <= b : !strcmp(x->op, ">=") ? a >= b :
                    !strcmp(x->op, "==") ? a == b : a != b;
            if (t) { strcpy(x->op, "goto"); x->a1[0] = x->a2[0] = '\0'; }
            else   x->del = 1;
            count[S_FOLD]++; changes++;
        }
    }
    return changes;
}

/* ----------------------------------------------------- 2. algebraic identities */

static int algebraicIdentities(void)
{
    int i, changes = 0;
    for (i = 0; i < nq; i++) {
        Quad *x = &q[i];
        char o;
        int done = 1;
        if (x->del || !isBin(x->op)) continue;
        o = x->op[0];
        if (o == '+') {
            if (!strcmp(x->a2, "0")) setCopy(x, x->a1);
            else if (!strcmp(x->a1, "0")) setCopy(x, x->a2);
            else done = 0;
        } else if (o == '-') {
            if (!strcmp(x->a2, "0")) setCopy(x, x->a1);
            else if (!strcmp(x->a1, x->a2)) setCopy(x, "0");
            else done = 0;
        } else if (o == '*') {
            if (!strcmp(x->a1, "0") || !strcmp(x->a2, "0")) setCopy(x, "0");
            else if (!strcmp(x->a2, "1")) setCopy(x, x->a1);
            else if (!strcmp(x->a1, "1")) setCopy(x, x->a2);
            else done = 0;
        } else if (o == '/') {
            if (!strcmp(x->a2, "1")) setCopy(x, x->a1);
            else done = 0;
        } else if (o == '%') {
            if (!strcmp(x->a2, "1")) setCopy(x, "0");
            else done = 0;
        } else if (o == '^') {
            if (!strcmp(x->a2, "1")) setCopy(x, x->a1);
            else if (!strcmp(x->a2, "0")) setCopy(x, "1");
            else done = 0;
        }
        if (done) { count[S_ALG]++; changes++; }
    }
    return changes;
}

/* ------------------------------------------------------ 3. strength reduction */

static int strengthReduction(void)
{
    int i, changes = 0;
    for (i = 0; i < nq; i++) {
        Quad *x = &q[i];
        if (x->del || !isBin(x->op)) continue;
        if (x->op[0] == '^' && !strcmp(x->a2, "2") && !isNum(x->a1)) {
            strcpy(x->op, "*");                         /* x ^ 2  ->  x * x */
            strcpy(x->a2, x->a1);
        } else if (x->op[0] == '*' && !strcmp(x->a2, "2") && !isNum(x->a1)) {
            strcpy(x->op, "+");                         /* x * 2  ->  x + x */
            strcpy(x->a2, x->a1);
        } else if (x->op[0] == '*' && !strcmp(x->a1, "2") && !isNum(x->a2)) {
            strcpy(x->op, "+");                         /* 2 * x  ->  x + x */
            strcpy(x->a1, x->a2);
        } else continue;
        count[S_STR]++; changes++;
    }
    return changes;
}

/* ----------------------------------------------------- uses of a name */

static int uses(const char *n)
{
    int i, c = 0;
    for (i = 0; i < nq; i++) {
        if (q[i].del) continue;
        if (hasA1(&q[i]) && !strcmp(q[i].a1, n)) c++;
        if (hasA2(&q[i]) && !strcmp(q[i].a2, n)) c++;
    }
    return c;
}

/* ------------------------------------------------ 4. merging of temporaries */

static int mergeTemps(void)
{
    int i, j, changes = 0;
    for (i = 0; i < nq; i++) {
        Quad *x = &q[i];
        if (x->del || !defines(x) || !isTemp(x->res)) continue;
        j = nextLive(i);
        if (j < 0) continue;
        if (!strcmp(q[j].op, "=") && !strcmp(q[j].a1, x->res) && uses(x->res) == 1) {
            strcpy(x->res, q[j].res);                   /* t = a op b ; x = t */
            q[j].del = 1;
            count[S_MERGE]++; changes++;
        }
    }
    return changes;
}

/* -------------------------------- 5. common sub-expression elimination */

typedef struct { char op[8], a1[NAMELEN], a2[NAMELEN], holder[NAMELEN]; } Avail;
static Avail av[MAXQ];
static int   nav;

static void kill(const char *n)                 /* n is assigned: forget what depends on it */
{
    int k;
    for (k = 0; k < nav; ) {
        if (!strcmp(av[k].a1, n) || !strcmp(av[k].a2, n) || !strcmp(av[k].holder, n))
            av[k] = av[--nav];
        else
            k++;
    }
}

static int commonSubexpressions(void)
{
    int i, k, changes = 0;
    nav = 0;
    for (i = 0; i < nq; i++) {
        Quad *x = &q[i];
        char op[8], a1[NAMELEN], a2[NAMELEN];
        int found = -1;
        if (x->del) continue;
        if (!strcmp(x->op, "label")) { nav = 0; continue; }     /* new basic block */
        if (!defines(x)) continue;
        strcpy(op, x->op); strcpy(a1, x->a1); strcpy(a2, x->a2);

        if (isBin(op)) {
            int comm = (op[0] == '+' || op[0] == '*');
            for (k = 0; k < nav && found < 0; k++) {
                if (strcmp(av[k].op, op)) continue;
                if ((!strcmp(av[k].a1, a1) && !strcmp(av[k].a2, a2)) ||
                    (comm && !strcmp(av[k].a1, a2) && !strcmp(av[k].a2, a1)))
                    found = k;
            }
        }
        if (found >= 0) {
            char holder[NAMELEN];
            strcpy(holder, av[found].holder);
            setCopy(x, holder);
            count[S_CSE]++; changes++;
        }
        kill(x->res);
        if (found < 0 && isBin(op) && strcmp(a1, x->res) && strcmp(a2, x->res)) {
            strcpy(av[nav].op, op); strcpy(av[nav].a1, a1);
            strcpy(av[nav].a2, a2); strcpy(av[nav].holder, x->res);
            nav++;
        }
    }
    return changes;
}

/* ----------------------------------------------------- 6. copy propagation */

static int copyPropagation(void)
{
    int i, j, changes = 0;
    for (i = 0; i < nq; i++) {
        Quad *x = &q[i];
        if (x->del || strcmp(x->op, "=") || !isTemp(x->res) || !isTemp(x->a1)) continue;
        for (j = 0; j < nq; j++) {                      /* temporaries are assigned only once */
            if (q[j].del) continue;
            if (hasA1(&q[j]) && !strcmp(q[j].a1, x->res)) strcpy(q[j].a1, x->a1);
            if (hasA2(&q[j]) && !strcmp(q[j].a2, x->res)) strcpy(q[j].a2, x->a1);
        }
        x->del = 1;
        count[S_COPY]++; changes++;
    }
    return changes;
}

/* ------------------------------------------------- 7. dead code elimination */

static int labelReferenced(const char *name)
{
    int j;
    for (j = 0; j < nq; j++)
        if (!q[j].del && (!strcmp(q[j].op, "goto") || isRel(q[j].op)) &&
            !strcmp(q[j].res, name)) return 1;
    return 0;
}

static int deadCode(void)
{
    int i, changes = 0, changed = 1;
    while (changed) {
        changed = 0;
        for (i = 0; i < nq; i++) {
            Quad *x = &q[i];
            int n;
            if (x->del) continue;
            n = nextLive(i);
            if (defines(x) && isTemp(x->res) && uses(x->res) == 0)
                x->del = 1;                                     /* result never used  */
            else if (!strcmp(x->op, "=") && !strcmp(x->a1, x->res))
                x->del = 1;                                     /* x = x              */
            else if ((!strcmp(x->op, "goto") || isRel(x->op)) && n >= 0 &&
                     !strcmp(q[n].op, "label") && !strcmp(q[n].res, x->res))
                x->del = 1;                                     /* jump to next quad  */
            else if (!strcmp(x->op, "label") && !labelReferenced(x->res))
                x->del = 1;                                     /* nobody jumps here  */
            else
                continue;
            count[S_DEAD]++; changes++; changed = 1;
        }
    }
    return changes;
}

/* ------------------------------------------------------------------- driver */

void optimize(void)
{
    int round, c;
    for (round = 0; round < 20; round++) {
        c  = constantPropagation();
        c += algebraicIdentities();
        c += strengthReduction();
        c += mergeTemps();
        c += commonSubexpressions();
        c += copyPropagation();
        c += deadCode();
        if (!c) break;
    }
}

void printOptimizationReport(void)
{
    int k, any = 0;
    printf("Optimizations applied\n");
    for (k = 0; k < S_COUNT; k++)
        if (count[k]) { printf("    %-36s : %d\n", title[k], count[k]); any = 1; }
    if (!any) printf("    none\n");
}
