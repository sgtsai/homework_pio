// ============================================================================
//  A pretend "transfer engine": a separate unit you hand a job to.
//
//  You do NOT write the copy loop. You describe the job -- where from, where
//  to, how many, how big each one is, which address walks forward -- then you
//  start it and get on with something else until it reports done.
//
//  It is deliberately fussy about the rules, because the real thing is too.
//  Break one and it will stop and tell you which rule you broke.
// ============================================================================
#ifndef ENGINE_H
#define ENGINE_H

#include <stdbool.h>
#include <stddef.h>

#define ENGINE_CHANNELS 4

/** Bytes moved per transfer. Nothing else is allowed. */
typedef enum { XFER_8 = 1, XFER_16 = 2, XFER_32 = 4 } xfer_size_t;

typedef struct {
  const void *src;
  void       *dst;
  size_t      count;          ///< number of TRANSFERS -- not a number of bytes
  xfer_size_t size;           ///< bytes per transfer
  bool        src_increment;  ///< walk the source forward?
  bool        dst_increment;  ///< walk the destination forward?
} engine_job_t;

/** Take an idle channel. Returns its number, or -1 when all are in use. */
int  engine_claim(void);

/** Describe the job. The channel must be idle. */
void engine_configure(int ch, const engine_job_t *job);

/** Kick it off and return immediately -- the transfer runs on its own. */
void engine_start(int ch);

/** Still running? */
bool engine_busy(int ch);

/** Block until this channel is done. */
void engine_wait(int ch);

/** Give the channel back so someone else can claim it. */
void engine_release(int ch);

#endif // ENGINE_H
