// ============================================================================
//  Critical section -- two threads, one shared variable
//
//  Build and run:   make run-critical-section
//
//  Two workers each add 1 to the same counter, N times. The final value should
//  obviously be 2*N. Run part 1 a few times and watch it not be.
//
//  What to take away:
//    * "counter++" is not one step. It is read, add, write -- and the other
//      worker can slip in between the read and the write, so one of the two
//      updates is simply lost.
//    * A lock makes the three steps behave as one. The section it protects is
//      called a CRITICAL SECTION.
//    * A lock is not free. Part 3 holds it for longer than it has to and the
//      whole program slows to a crawl -- both workers spend their time queueing
//      instead of working. Keep the critical section as short as it can be.
// ============================================================================
#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#ifdef __linux__
#include <sched.h>
#endif

#define WORKERS  2
#define ROUNDS   20000

// volatile: every ++ must really read, add and write memory, which is
// exactly the three-step sequence the demo is about.
static volatile long counter;
static pthread_spinlock_t lock;

// Stand-in for "some real work this worker has to do". Deliberately not free.
static uint32_t work(uint32_t x) {
  for (int i = 0; i < 2000; i++) x = x * 1664525u + 1013904223u;
  return x;
}

static uint32_t sink;   // keeps the compiler from deleting work()

// How many CPUs may this process actually run on? Not the same as how many the
// machine has -- a container or `taskset` can hand you fewer.
static long usable_cpus(void) {
#ifdef __linux__
  cpu_set_t set;
  if (sched_getaffinity(0, sizeof set, &set) == 0) return CPU_COUNT(&set);
#endif
  return sysconf(_SC_NPROCESSORS_ONLN);
}

// ---------------------------------------------------------------- part 1
static void *worker_unlocked(void *arg) {
  (void)arg;
  for (int i = 0; i < ROUNDS; i++) {
    sink += work(i);
    long v = counter;               // step 1: read
    counter = v + 1;                // steps 2 and 3: add, write back
                                    // the other worker can land in between
  }
  return NULL;
}

// ---------------------------------------------------------------- part 2
static void *worker_locked(void *arg) {
  (void)arg;
  for (int i = 0; i < ROUNDS; i++) {
    sink += work(i);                // the work stays OUTSIDE the lock
    pthread_spin_lock(&lock);
    counter++;                      // critical section: one line
    pthread_spin_unlock(&lock);
  }
  return NULL;
}

// ---------------------------------------------------------------- part 3
static void *worker_locked_too_long(void *arg) {
  (void)arg;
  for (int i = 0; i < ROUNDS; i++) {
    pthread_spin_lock(&lock);
    sink += work(i);                // the work is now INSIDE the lock
    counter++;                      // ... so only one worker runs at a time
    pthread_spin_unlock(&lock);
  }
  return NULL;
}

static double run(void *(*fn)(void *)) {
  struct timespec t0, t1;
  pthread_t th[WORKERS];

  counter = 0;
  clock_gettime(CLOCK_MONOTONIC, &t0);
  for (int i = 0; i < WORKERS; i++) pthread_create(&th[i], NULL, fn, NULL);
  for (int i = 0; i < WORKERS; i++) pthread_join(th[i], NULL);
  clock_gettime(CLOCK_MONOTONIC, &t1);

  return (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
}

int main(void) {
  pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);

  long cpus = usable_cpus();
  printf("expected counter = %d   (%ld CPU%s available)\n",
         WORKERS * ROUNDS, cpus, cpus == 1 ? "" : "s");
  if (cpus < 2)
    printf("NOTE: with only one CPU the two workers rarely overlap, so line 1\n"
           "      below may come out correct. That does not make it safe.\n");
  printf("\n");

  double ms = run(worker_unlocked);
  printf("1. no lock            counter = %-7ld  %s   %6.1f ms\n",
         counter, counter == WORKERS * ROUNDS ? "ok  " : "LOST UPDATES", ms);

  ms = run(worker_locked);
  printf("2. lock, short hold   counter = %-7ld  %s   %6.1f ms\n",
         counter, counter == WORKERS * ROUNDS ? "ok  " : "LOST UPDATES", ms);

  ms = run(worker_locked_too_long);
  printf("3. lock, long hold    counter = %-7ld  %s   %6.1f ms\n",
         counter, counter == WORKERS * ROUNDS ? "ok  " : "LOST UPDATES", ms);

  printf("\nrun `make run-critical-section` a few times -- line 1 changes, lines 2 and 3 do not.\n");
  pthread_spin_destroy(&lock);
  return 0;
}
