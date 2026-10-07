    MOV R0, #6
    MOV R1, #1
    MOV R2, #2
    MOV R3, #64
    MOV R4, #2
L1:
    MOV R9, #1
    CMP R0, R9
    JLE L2
    MUL R1, R0
    MOV R9, #1
    SUB R0, R9
    JMP L1
L2:
    MOV R9, R1
    MOV R10, #3
    DIV R9, R10
    MOV R10, R1
    ADD R10, R1
    MOV R5, R9
    ADD R5, R10
    MOV R6, R5
    MOV R9, R1
    NEG R9
    MOV R7, R9
    MOV R10, #2
    ADD R7, R10
    MOV R8, #0
L3:
    MOV R9, #4
    CMP R8, R9
    JGE L4
    MOV R9, R8
    MOV R10, #2
    MOD R9, R10
    MOV R10, #0
    CMP R9, R10
    JNE L5
    MOV R9, R8
    MUL R9, R8
    OUT R9
    OUT "\n"
    JMP L6
L5:
    MOV R9, R1
    SUB R9, R8
    OUT R9
    OUT "\n"
L6:
    MOV R9, #1
    ADD R8, R9
    JMP L3
L4:
    MOV R9, R5
    ADD R9, R6
    OUT R9
    OUT "\n"
    OUT R7
    OUT "\n"
    OUT #64
    OUT "\n"
