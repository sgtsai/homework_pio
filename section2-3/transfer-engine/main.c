// ============================================================================
//  Transfer engine -- describing a job instead of writing the loop
//
//  Build and run:   make run-transfer-engine   (and: make mistake)
//
//  A transfer engine is a separate unit that moves data for you. You never
//  write the copy loop: you fill in a description (from, to, how many, how
//  big, which side walks forward), start it, and later collect the result.
//
//  What to take away:
//    * The four beats: DESCRIBE -> START -> WAIT -> USE THE RESULT.
//    * `count` is a number of TRANSFERS, not a number of bytes. Getting this
//      wrong is the single most common mistake, so part 3 makes it happen.
//    * Whether each address walks forward is part of the description, and it
//      is what makes the difference between copying and filling.
//    * A channel is a resource: claim it, then give it back.
//
//  Read ../README.md for what this demo does NOT show you.
// ============================================================================
#include "engine.h"

#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define N 4096
// The buffers are four times the size we ever mean to use, so that part 3 can
// overrun the intended area and still be caught rather than crash.
#define ROOM (4 * N)

static uint32_t src[ROOM], dst[ROOM];

static void reset(void) {
  for (int i = 0; i < ROOM; i++) { src[i] = (uint32_t)(i * 2654435761u); dst[i] = 0; }
}

int main(int argc, char **argv) {
  int ch = engine_claim();
  printf("claimed channel %d (there are %d)\n\n", ch, ENGINE_CHANNELS);

  // ------------------------------------------------ 1. copy, and do not idle
  reset();
  engine_job_t copy = {
    .src = src, .dst = dst,
    .count = N,                 // N transfers ...
    .size  = XFER_32,           // ... of 4 bytes each
    .src_increment = true,      // walk the source
    .dst_increment = true,      // walk the destination
  };
  engine_configure(ch, &copy);
  engine_start(ch);             // returns immediately -- the job is not done yet

  unsigned long spins = 0;
  while (engine_busy(ch)) {
    spins++;                         // your program is free to do other things
    // Hand the CPU over now and then, so this still makes progress on a
    // machine with only one core to share.
    if ((spins & 0xffff) == 0) sched_yield();
  }
  engine_wait(ch);

  printf("1. copy %d words          %s\n", N,
         memcmp(src, dst, N * sizeof(uint32_t)) == 0 ? "dst matches src" : "MISMATCH");
  printf("   while it ran, this program went round its own loop %lu times\n\n",
         spins);

  // ------------------------------------------------ 2. same engine, fill
  reset();
  uint32_t pattern = 0xA5A5A5A5u;
  engine_job_t fill = {
    .src = &pattern, .dst = dst,
    .count = N,
    .size  = XFER_32,
    .src_increment = false,     // stay on the pattern ...
    .dst_increment = true,      // ... and walk the destination
  };
  engine_configure(ch, &fill);
  engine_start(ch);
  engine_wait(ch);

  int filled = 1;
  for (int i = 0; i < N; i++) if (dst[i] != pattern) { filled = 0; break; }
  printf("2. fill %d words          %s\n", N,
         filled ? "every word is 0xA5A5A5A5" : "MISMATCH");
  printf("   same engine, same count -- only the increment flags changed\n\n");

  engine_release(ch);

  // ------------------------------------------------ 3. the classic mistake
  if (argc > 1 && strcmp(argv[1], "--mistake") == 0) {
    reset();
    ch = engine_claim();
    engine_job_t wrong = copy;
    wrong.count = N * sizeof(uint32_t);   // BYTES -- but count is in transfers
    engine_configure(ch, &wrong);
    engine_start(ch);
    engine_wait(ch);
    engine_release(ch);

    size_t touched = 0;
    for (size_t i = 0; i < ROOM; i++) if (dst[i] != 0) touched = i + 1;
    printf("3. count given as %zu (the byte count) instead of %d\n",
           (size_t)(N * sizeof(uint32_t)), N);
    printf("   -> the engine moved %zu words and wrote %zu past the %d you meant\n",
           touched, touched - N, N);
    printf("   nothing complained: the engine did exactly what you described.\n");
  } else {
    printf("3. run `make mistake` to see what happens when count is given\n");
    printf("   in bytes instead of transfers.\n");
  }
  return 0;
}
