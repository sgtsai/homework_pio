#ifndef HW23_SUBMISSION_H
#define HW23_SUBMISSION_H
#include <pthread.h>
#include "api.h"

/* Edit the queue representation if needed, but keep these three synchronization
 * members with their exact names/types. All mutable queue state belongs here.
 * Do not add heap storage, global state, thread-local state, or pointer-based
 * external metadata. The jobs' src/dst pointers refer to caller-owned buffers. */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;  // Consumers wait for a job.
    pthread_cond_t not_full;   // Producers wait for space.
    hw_job slots[HW_CAPACITY];
    size_t head;
    size_t tail;
    size_t used;
    bool closed;
} hw_queue;

int hw_validate(const hw_job *job);  /* 0 valid, -1 invalid */
int hw_transfer(const hw_job *job);  /* validate, then copy; 0 or -1 */
int hw_queue_init(hw_queue *q);      /* 0 success, -1 initialization failure */
void hw_queue_destroy(hw_queue *q); /* caller has already joined all users */
int hw_push(hw_queue *q, const hw_job *job); /* 1 accepted, 0 closed */
int hw_pop(hw_queue *q, hw_job *job);        /* 1 obtained, 0 closed+empty */
void hw_close(hw_queue *q);          /* idempotent; wake all waiting threads */
void *hw_worker(void *arg);          /* arg is hw_queue*, return NULL */
#endif
