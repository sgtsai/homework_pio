# Starter only: this echoes each word twice and is intentionally incorrect.
# Replace it with your optimized packet processor. r6 can hold rolling state.
    li r7, 0
start:
    pull
    out r1, 32
    xori r1, r1, 0xA5000000
    addi r3, r1, 0x00010000
    andi r3, r3, 0x00FF0000 #r3 used
    li r4, 0x00000100
    li r5, 8 # index for counting
    JJ: jnz r5, count_one
    li r6, 8
    addi r4, r1, 0
    andi r1, r1, 0xFF000000
    add r1, r1, r3
    li r2, 0
    JJ2: jnz r6, rev_once
    add r1, r1, r2
    in r1, 32
    push
    andi r2, r7, 0xF8000000
    shr r2, r2, 27
    shl r7, r7, 5
    add r7, r7, r2
    xor r7, r7, r1
    in r7, 32
    push
    jmp start

count_one:
    addi r5, r5, -1
    and r6, r1, r4
    shl r4, r4, 1
    jnz r6, addone
    jmp JJ
addone:
    addi r3, r3, 0x00000100
    jmp JJ
rev_once:
    addi r6, r6, -1
    andi r5, r4, 1
    shl r2, r2, 1
    shr r4, r4, 1
    jnz r5, addone2
    jmp JJ2
addone2:
    addi r2, r2, 1
    jmp JJ2



