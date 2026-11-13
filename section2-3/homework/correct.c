/* Intentionally incomplete starter. Implement the contract in homework.txt.
 * This starter compiles, but earns zero points. No solution is included. */
#include "submission.h"

int hw_validate(const hw_job *job) {
    if(job == NULL) return -1;
    if(job -> src == NULL || job -> dst == NULL) return -1;
    if(job -> count == 0) return -1;
    if(job -> size != 1 && job -> size != 2 && job -> size != 4) return -1;
    if((uintptr_t)(job -> src) % job -> size != 0 || (uintptr_t)(job -> dst) % job -> size != 0) return -1;
    if(job->count > SIZE_MAX || job->size > SIZE_MAX / job -> count) return -1;
    if(job->src_bytes < (uintptr_t)job->count * (uintptr_t)job->size && job->src_increment) return -1;
    if(job->src_bytes < (uintptr_t)job->size) return -1;
    if(job->dst_bytes < (uintptr_t)job->count * (uintptr_t)job->size && job->dst_increment) return -1;
    if(job->dst_bytes < (uintptr_t)job->size) return -1;
    return 0; /* TODO */
}

int hw_transfer(const hw_job *job) {
    if(hw_validate(job) == -1) return -1;
    if(job -> src_increment && job -> dst_increment) {
        for(size_t i = 0; i < job->count; i++) {
            hw_copy_unit(job->dst + i * job->size, job->src + i * job->size, job->size);
        }
    } else if(job -> src_increment && !job -> dst_increment) {
        for(size_t i = 0; i < job->count; i++) {
            hw_copy_unit(job->dst, job->src + i * job->size, job->size);
        }
    } else if(!job -> src_increment && job -> dst_increment) {
        for(size_t i = 0; i < job->count; i++) {
            hw_copy_unit(job->dst + i * job->size, job->src, job->size);
        }
    } else {
        for(size_t i = 0; i < job->count; i++) {
            hw_copy_unit(job->dst, job->src, job->size);
        }
    }
    return 0;
}

int hw_queue_init(hw_queue *q) {
    int result = pthread_mutex_init(&q->mutex, NULL);
    if (result != 0) {
        return -1;
    }
    result = pthread_cond_init(&q->not_full, NULL);
    if (result != 0) {
        pthread_mutex_destroy(&q->mutex);
        return -1;
    }
    result = pthread_cond_init(&q->not_empty, NULL);
    if (result != 0) {
        pthread_cond_destroy(&q->not_full);
        pthread_mutex_destroy(&q->mutex);
        return -1;
    }
    q -> closed = false;
    q -> head = 0;
    q -> tail = -1;
    q -> qsize = 0;
    return 0;
}

void hw_queue_destroy(hw_queue *q) {
    pthread_cond_destroy(&q->not_empty);
    pthread_cond_destroy(&q->not_full);
    pthread_mutex_destroy(&q->mutex);
}

int hw_push(hw_queue *q, const hw_job *job) {
    pthread_mutex_lock(&q->mutex);
        while(q -> qsize == HW_CAPACITY && !q -> closed) {
            pthread_cond_wait(&q->not_full, &q->mutex);
        }
        if (q->closed) {
            pthread_mutex_unlock(&q->mutex);
            return 0;
        }
        q -> tail = (q -> tail + 1) % HW_CAPACITY;
        q -> slots[q -> tail] = *job;
        q -> qsize++;
        pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
    return 1; /* TODO: wait when full; stop waiting when closed */
}

int hw_pop(hw_queue *q, hw_job *job) {
    pthread_mutex_lock(&q->mutex);
        while(q -> qsize == 0) {
            if(q -> closed) {
                pthread_mutex_unlock(&q->mutex);
                return 0;
            }
            pthread_cond_wait(&q->not_empty, &q->mutex);
        }
        
        *job = q -> slots[q -> head];
        q -> head = (q -> head + 1) % HW_CAPACITY;
        q -> qsize--;
        pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    return 1; /* TODO: wait when full; stop waiting when closed */
}

void hw_close(hw_queue *q) {
    pthread_mutex_lock(&q->mutex);
        pthread_cond_broadcast(&q->not_empty);
        pthread_cond_broadcast(&q->not_full);
        q -> closed = true;
    pthread_mutex_unlock(&q->mutex);
    
}

void *hw_worker(void *arg) {
    hw_queue *q = arg;
    hw_job job;
    while(hw_pop(q, &job) == 1) {
        hw_transfer(&job);
    }
    return NULL; /* TODO: pop, transfer outside queue lock, repeat */
}
