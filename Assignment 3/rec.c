#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *input;
int i = 0;
int error = 0;

void E();
void E_prime();
void T();
void T_prime();
void F();

void match(char expected) {
    if (input[i] == expected) {
        i++;
    } else {
        error = 1;
    }
}

void match_id() {
    if (input[i] == 'i' && input[i + 1] == 'd') {
        i += 2;
    } else {
        error = 1;
    }
}

void E() {
    T();
    E_prime();
}

void E_prime() {
    if (input[i] == '+') {
        match('+');
        T();
        E_prime();
    }
}

void T() {
    F();
    T_prime();
}

void T_prime() {
    if (input[i] == '*') {
        match('*');
        F();
        T_prime();
    }
}

void F() {
    if (input[i] == '(') {
        match('(');
        E();
        match(')');
    } else if (input[i] == 'i' && input[i + 1] == 'd') {
        match_id();
    } else {
        error = 1;
    }
}

void parse_string(const char *str) {
    input = str;
    i = 0;
    error = 0;
    printf("Parsing string: \"%s\"\n", input);

    E();
    if (error == 0 && input[i] == '\0') {
        printf("Result: SUCCESS (String successfully parsed!)\n\n");
    } else {
        printf("Result: REJECTED (Syntax Error detected!)\n\n");
    }
}

int main() {
    parse_string("id+id*id");
    parse_string("(id+id*id");
    parse_string("id-id");
    return 0;
}