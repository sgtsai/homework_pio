# Starter only: this echoes each word twice and is intentionally incorrect.
# Replace it with your optimized packet processor. r6 can hold rolling state.
start:
    pull
    out r1, 32
    in r1, 32
    push
    in r1, 32
    push
    jmp start
