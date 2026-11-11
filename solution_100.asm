# Full-score solution for the custom homework ISA (not hardware assembly).
# 32 instructions; 352 simulated bytes; 95 retired cycles per input.
# All registers start at zero under the homework contract, including r6.
# r6 is the rolling checksum: never reset it inside the packet loop.
#
# Registers during the bit loop:
# r1 = shifting input, r2 = reversed payload, r3 = flags population count
# r4 = loop counter, r5 = temporary, r6 = checksum, r7 = rotate temporary

start:
    pull
    out r1, 32
    li r2, 0
    li r3, 0
    li r4, 8

# Process one payload bit AND one flags bit per iteration.
# After k shifts, original payload bit k is at bit 0 and flags bit k at bit 8.
bits:
    shl r2, r2, 1
    andi r5, r1, 1
    or r2, r2, r5
    shr r5, r1, 8
    andi r5, r5, 1
    add r3, r3, r5
    shr r1, r1, 1
    addi r4, r4, -1
    jnz r4, bits

# Now r1 = original input >> 8:
# bits 23..16 = tag, bits 15..8 = sequence, bits 7..0 = flags.
# Increment sequence in a temporary, then mask away carry into the tag.
    addi r5, r1, 0x100
    andi r5, r5, 0xFF00
    andi r1, r1, 0xFF0000
    xori r1, r1, 0xA50000
    or r1, r1, r5
    shl r1, r1, 8
    shl r3, r3, 8
    or r2, r2, r3
    or r2, r2, r1

# r2 = transformed packet T. Compute C = ROTL32(old C, 5) XOR T.
    shl r7, r6, 5
    shr r6, r6, 27
    or r6, r6, r7
    xor r6, r6, r2

# Output T, then C. Blocking PUSH preserves the registers until space exists.
    in r2, 32
    push
    in r6, 32
    push
    jmp start
