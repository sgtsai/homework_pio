// Implementation of the pretend transfer engine.
//
// Honesty note: this is a plain thread. Real transfer hardware costs the CPU
// nothing, while the thread below occupies a core of your laptop. What this
// file reproduces faithfully is the INTERFACE and its rules, not the "for
// free" part. See README.md.
#include "engine.h"

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void die(const char *what) {
  fprintf(stderr, "\n*** transfer engine refused the job: %s\n", what);
  exit(1);
}

typedef struct {
  bool         claimed;
  bool         configured;
  volatile bool busy;
  engine_job_t job;
  pthread_t    th;
} channel_t;

static channel_t chans[ENGINE_CHANNELS];

static void check_channel(int ch) {
  if (ch < 0 || ch >= ENGINE_CHANNELS) die("no such channel");
  if (!chans[ch].claimed)             die("that channel was never claimed");
}

int engine_claim(void) {
  for (int i = 0; i < ENGINE_CHANNELS; i++) {
    if (!chans[i].claimed) { chans[i].claimed = true; return i; }
  }
  return -1;   // all four in use -- did you forget to release one?
}

void engine_configure(int ch, const engine_job_t *job) {
  check_channel(ch);
  if (chans[ch].busy) die("the channel is still running the previous job");

  if (job->size != XFER_8 && job->size != XFER_16 && job->size != XFER_32)
    die("transfer size must be 1, 2 or 4 bytes");
  if (job->count == 0)
    die("count is zero -- count is the number of transfers, not bytes");
  if (((uintptr_t)job->src % (uintptr_t)job->size) != 0)
    die("source address is not aligned to the transfer size");
  if (((uintptr_t)job->dst % (uintptr_t)job->size) != 0)
    die("destination address is not aligned to the transfer size");

  chans[ch].job = *job;
  chans[ch].configured = true;
}

static void *run_job(void *arg) {
  channel_t *c = (channel_t *)arg;
  const unsigned char *s = (const unsigned char *)c->job.src;
  unsigned char       *d = (unsigned char *)c->job.dst;
  size_t step = (size_t)c->job.size;

  for (size_t i = 0; i < c->job.count; i++) {
    memcpy(d, s, step);
    if (c->job.src_increment) s += step;
    if (c->job.dst_increment) d += step;
    // Pretend one transfer takes a moment, so "busy" is observable at all.
    struct timespec nap = { 0, 2000 };
    nanosleep(&nap, NULL);
  }
  c->busy = false;
  return NULL;
}

void engine_start(int ch) {
  check_channel(ch);
  if (!chans[ch].configured) die("nothing was configured on that channel yet");
  if (chans[ch].busy)        die("that channel is already running");

  chans[ch].busy = true;
  pthread_create(&chans[ch].th, NULL, run_job, &chans[ch]);
}

bool engine_busy(int ch) { check_channel(ch); return chans[ch].busy; }

void engine_wait(int ch) {
  check_channel(ch);
  if (chans[ch].busy || chans[ch].th) pthread_join(chans[ch].th, NULL);
  chans[ch].th = 0;
}

void engine_release(int ch) {
  check_channel(ch);
  if (chans[ch].busy) die("release while the channel is still running");
  chans[ch].claimed = chans[ch].configured = false;
}
