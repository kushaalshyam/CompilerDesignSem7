# Compiler Design (Sem 7) – C++ edition

| Assignment | Topic |
|---|---|
| 1 | Hand-written lexical analyzer + symbol table |
| 2 | Lexical analyzer using Lex | Yes – C++ keywords, `cout/cin/endl`, `#include`, `bool`, `true/false` |
| 3 | Recursive descent parser | 
| 4 | Desk calculator (Lex + Yacc) | 
| 5 | Syntax checker (Lex + Yacc) | 
| 6 | Three address code (Lex + Yacc) | No |
| 7 | Code optimization on TAC | No |
| 8 | Code generation (TAC -> 8086 style) | No |
| 9 | Mini compiler (lex + yacc + optimizer + codegen) |

Requirements (Ubuntu / WSL): `sudo apt install gcc flex bison`


## Assignment 1 – Lexical analyzer + symbol table
Reads `source.cpp` from the current folder.
```
cd Assignment1
gcc scanner.c -o scanner
./scanner
```

## Assignment 2 – Lexical analyzer using Lex
Input is read from standard input.
```
cd Assignment2
flex scanner.l
gcc lex.yy.c -o scanner
./scanner < sample.cpp
./scanner < sample2.cpp
```

## Assignment 3 – Recursive descent parser
Parses three hard-coded strings.
```
cd Assignment3
gcc rec.c -o rec
./rec
```

## Assignment 4 – Desk calculator
Type expressions one per line (Ctrl+D to stop), or pipe them in.
```
cd Assignment4
flex ex.l
yacc -d ex.y
gcc lex.yy.c y.tab.c -o calc -lm
./calc
```
e.g. `printf '3+9\n(3+4)*7\n4^2^1\n' | ./calc`

## Assignment 5 – Syntax checker
Prints `Syntactically correct` or the line of the first error.
`input.txt`, `input2.txt`, `input5.txt` are valid; `input3.txt`, `input4.txt`,
`input6.txt` contain errors. `input5.txt` is a complete C++ program.
```
cd Assignment5
flex syntax.l
yacc -d syntax.y
gcc lex.yy.c y.tab.c -o syncheck
./syncheck < input.txt
./syncheck < input5.txt
./syncheck < input6.txt
```

## Assignment 6 – Three address code
```
cd Assignment6
flex tac.l
yacc -d tac.y
gcc lex.yy.c y.tab.c -o tac
./tac < input.txt
./tac < input2.txt
```

## Assignment 7 – Code optimizer
```
cd Assignment7
gcc optimizer.c -o optimizer
./optimizer < input.txt
./optimizer < input2.txt
```

## Assignment 8 – Code generator (TAC -> 8086 style assembly)
```
cd Assignment8
gcc codegen.c -o codegen
./codegen < input.txt
./codegen < input2.txt
```

## Assignment 9 – Mini compiler for C++
Takes a C++ file as the argument (default `input.cpp`) and runs all phases:
lexical analysis, syntax analysis + symbol table, three address code, optimization,
8086-style code (also written to `output.asm`).
```
cd Assignment9
flex scanner.l
yacc -d parser.y
gcc lex.yy.c y.tab.c quad.c optimizer.c codegen.c -o minicompiler
./minicompiler input.cpp
./minicompiler input2.cpp
```
Error demonstrations:
```
./minicompiler input3.cpp     # missing semicolon  -> syntax error
./minicompiler input4.cpp     # undeclared variable -> semantic error
./minicompiler input5.cpp     # illegal character '#' -> lexical error
```

### C++ subset accepted by Assignment 9
```cpp
#include <iostream>            // preprocessor lines are listed and skipped
#include <cmath>
using namespace std;           // optional

int main() {                   // or  int main(void)
    int a = 10, b;             // int declarations (with optional initialisers)
    b = a * 2 + pow(a, 2);     // + - * / %, parentheses, pow(x, n) for power
    if (a < b) { ... } else { ... }          // relational: < <= > >= == !=
    while (a <= 5) { ... }
    cout << "text" << a + b << endl;         // also std::cout, std::endl
    return 0;                  // optional, last statement of main
}
```
`pow(x, n)` replaces Java-version `x ** n` (in C++ `^` is XOR, so it is not used for power).
`endl` is printed as `OUT "\n"` in the generated assembly.

Each assignment folder also contains its report, renamed `CD01-071.docx` ... `CD09-071.docx`.
