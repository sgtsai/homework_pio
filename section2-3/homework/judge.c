/* Trusted public tests. Do not edit for an official submission. */
#include "submission.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); exit(1); } } while (0)
static pthread_mutex_t audit = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
static size_t calls;
static const void *pause_source;
static int paused, resume_copy;

void hw_copy_unit(void *dst, const void *src, size_t size) {
    CHECK(pthread_mutex_lock(&audit) == 0);
    ++calls;
    if (src == pause_source && pause_source != NULL) {
        paused = 1;
        CHECK(pthread_cond_broadcast(&changed) == 0);
        while (!resume_copy) CHECK(pthread_cond_wait(&changed, &audit) == 0);
    }
    CHECK(pthread_mutex_unlock(&audit) == 0);
    memcpy(dst, src, size);
}

static hw_job make_job(const void *src, void *dst, size_t count, size_t size,
                       size_t sb, size_t db, bool si, bool di) {
    hw_job j = {src, dst, count, size, sb, db, si, di};
    return j;
}

static void copy_modes(void) {
    _Alignas(4) unsigned char src[128], dst[128], want[128];
    for (size_t i=0; i<sizeof src; ++i) src[i]=(unsigned char)(i*37+11);
    for (size_t width=1; width<=4; width*=2) {
        for (int si=0; si<=1; ++si) for (int di=0; di<=1; ++di) {
            memset(dst, 0xcc, sizeof dst); memset(want, 0xcc, sizeof want);
            size_t count=17, before=calls;
            hw_job j=make_job(src,dst,count,width,si?count*width:width,
                              di?count*width:width,si,di);
            CHECK(hw_validate(&j)==0);
            CHECK(hw_transfer(&j)==0);
            for (size_t k=0; k<count; ++k)
                memcpy(want+(di?k*width:0),src+(si?k*width:0),width);
            CHECK(memcmp(dst,want,sizeof dst)==0);
            CHECK(calls-before==count);
        }
    }
}

static void reject(const hw_job *j, const unsigned char *dst, size_t n) {
    size_t before=calls;
    CHECK(hw_validate(j)==-1);
    CHECK(hw_transfer(j)==-1);
    CHECK(calls==before);
    for (size_t i=0;i<n;++i) CHECK(dst[i]==0xcc);
}

static void validation(void) {
    _Alignas(4) unsigned char src[64]={0}, dst[64];
    memset(dst,0xcc,sizeof dst);
    hw_job good=make_job(src,dst,8,4,32,32,true,true), j;
    CHECK(hw_validate(&good)==0);
    reject(NULL,dst,sizeof dst);
    j=good; j.src=NULL; reject(&j,dst,sizeof dst);
    j=good; j.dst=NULL; reject(&j,dst,sizeof dst);
    j=good; j.size=3; reject(&j,dst,sizeof dst);
    j=good; j.size=0; reject(&j,dst,sizeof dst);
    j=good; j.count=0; reject(&j,dst,sizeof dst);
    j=good; j.src=src+1; reject(&j,dst,sizeof dst);
    j=good; j.dst=dst+2; reject(&j,dst,sizeof dst);
    j=good; j.src_bytes=31; reject(&j,dst,sizeof dst);
    j=good; j.dst_bytes=31; reject(&j,dst,sizeof dst);
    j=good; j.count=SIZE_MAX/4+1; reject(&j,dst,sizeof dst);
    j=good; j.src_increment=false; j.src_bytes=3; reject(&j,dst,sizeof dst);
    j=good; j.dst_increment=false; j.dst_bytes=3; reject(&j,dst,sizeof dst);
    j=good; j.src_increment=false; j.src_bytes=4; CHECK(hw_validate(&j)==0);
    j=good; j.dst_increment=false; j.dst_bytes=4; CHECK(hw_validate(&j)==0);
}

static void fifo(void) {
    hw_queue q;
    CHECK(hw_queue_init(&q)==0);
    uint32_t src[8], dst[8]={0};
    /* Repeatedly fill/drain to exercise ring wrap and descriptor copies. */
    for (int round=0;round<37;++round) {
        for (int i=0;i<8;++i) {
            src[i]=(uint32_t)(round*8+i);
            hw_job j=make_job(&src[i],&dst[i],1,4,4,4,true,true);
            CHECK(hw_push(&q,&j)==1);
            memset(&j,0,sizeof j); /* queue must own a descriptor copy */
        }
        for (int i=0;i<8;++i) {
            hw_job j;
            CHECK(hw_pop(&q,&j)==1);
            CHECK(j.src==&src[i] && j.dst==&dst[i]);
            CHECK(j.count==1 && j.size==4 && j.src_bytes==4 && j.dst_bytes==4);
        }
    }
    hw_close(&q); hw_close(&q);
    hw_job j=make_job(src,dst,1,4,4,4,true,true);
    CHECK(hw_push(&q,&j)==0);
    CHECK(hw_pop(&q,&j)==0);
    hw_queue_destroy(&q);
}

typedef struct { hw_queue *q; hw_job j; int result; int push; } waiter;
static int arrivals;
static void *wait_once(void *arg) {
    waiter *w=arg;
    CHECK(pthread_mutex_lock(&audit)==0);
    ++arrivals;
    CHECK(pthread_cond_broadcast(&changed)==0);
    CHECK(pthread_mutex_unlock(&audit)==0);
    w->result=w->push?hw_push(w->q,&w->j):hw_pop(w->q,&w->j);
    return NULL;
}

static void closing(void) {
    for (int full=0;full<=1;++full) {
        hw_queue q; uint32_t src=42,dst=0;
        CHECK(hw_queue_init(&q)==0);
        hw_job j=make_job(&src,&dst,1,4,4,4,true,true);
        if(full) for(int i=0;i<8;++i) CHECK(hw_push(&q,&j)==1);
        pthread_t threads[4]; waiter ws[4]; arrivals=0;
        for(int i=0;i<4;++i) {
            ws[i]=(waiter){&q,j,-99,full};
            CHECK(pthread_create(&threads[i],NULL,wait_once,&ws[i])==0);
        }
        CHECK(pthread_mutex_lock(&audit)==0);
        while(arrivals!=4) CHECK(pthread_cond_wait(&changed,&audit)==0);
        CHECK(pthread_mutex_unlock(&audit)==0);
        hw_close(&q);
        for(int i=0;i<4;++i) {
            CHECK(pthread_join(threads[i],NULL)==0);
            CHECK(ws[i].result==0);
        }
        if(full) for(int i=0;i<8;++i) CHECK(hw_pop(&q,&j)==1);
        CHECK(hw_pop(&q,&j)==0);
        hw_queue_destroy(&q);
    }
}

#define TOTAL 2048
static uint32_t ids[TOTAL], output[TOTAL];
static unsigned seen[TOTAL];
typedef struct { hw_queue *q; int begin, end; } producer_args;
static void *producer(void *arg) {
    producer_args *p=arg;
    for(int i=p->begin;i<p->end;++i) {
        hw_job j=make_job(&ids[i],&output[i],1,4,4,4,true,true);
        CHECK(hw_push(p->q,&j)==1);
    }
    return NULL;
}
static void *consumer(void *arg) {
    hw_job j;
    while(hw_pop(arg,&j)==1) {
        uint32_t id=*(const uint32_t *)j.src;
        CHECK(id<TOTAL);
        CHECK(pthread_mutex_lock(&audit)==0);
        ++seen[id];
        CHECK(pthread_mutex_unlock(&audit)==0);
    }
    return NULL;
}

static void threaded(bool transfer) {
    hw_queue q;
    CHECK(hw_queue_init(&q)==0);
    for(int i=0;i<TOTAL;++i) {ids[i]=(uint32_t)i; output[i]=0xdeadbeef; seen[i]=0;}
    if(transfer) {
        hw_job invalid=make_job(ids,output,1,3,sizeof ids,sizeof output,true,true);
        CHECK(hw_push(&q,&invalid)==1); /* worker must reject it and continue */
    }
    pthread_t producers[4], consumers[4]; producer_args args[4];
    for(int i=0;i<4;++i)
        CHECK(pthread_create(&consumers[i],NULL,transfer?hw_worker:consumer,&q)==0);
    for(int i=0;i<4;++i) {
        args[i]=(producer_args){&q,i*(TOTAL/4),(i+1)*(TOTAL/4)};
        CHECK(pthread_create(&producers[i],NULL,producer,&args[i])==0);
    }
    for(int i=0;i<4;++i) CHECK(pthread_join(producers[i],NULL)==0);
    hw_close(&q);
    for(int i=0;i<4;++i) CHECK(pthread_join(consumers[i],NULL)==0);
    for(int i=0;i<TOTAL;++i) {
        if(transfer) CHECK(output[i]==ids[i]);
        else CHECK(seen[i]==1);
    }
    if(transfer) CHECK(calls==TOTAL);
    hw_queue_destroy(&q);
}

static void progress(void) {
    hw_queue q; pthread_t worker;
    uint32_t slow_src=0x12345678, slow_dst=0, fast_src=0xa5a5a5a5, fast_dst=0;
    CHECK(hw_queue_init(&q)==0);
    pause_source=&slow_src; paused=resume_copy=0;
    hw_job slow=make_job(&slow_src,&slow_dst,1,4,4,4,true,true);
    hw_job fast=make_job(&fast_src,&fast_dst,1,4,4,4,true,true), got;
    CHECK(hw_push(&q,&slow)==1);
    CHECK(pthread_create(&worker,NULL,hw_worker,&q)==0);
    CHECK(pthread_mutex_lock(&audit)==0);
    while(!paused) CHECK(pthread_cond_wait(&changed,&audit)==0);
    CHECK(pthread_mutex_unlock(&audit)==0);
    /* Slow transfer is deliberately unable to finish yet. Queue operations
     * and an unrelated transfer must still make progress, without sleeps. */
    CHECK(hw_push(&q,&fast)==1);
    CHECK(hw_pop(&q,&got)==1);
    CHECK(got.src==&fast_src);
    CHECK(hw_transfer(&got)==0 && fast_dst==fast_src);
    CHECK(pthread_mutex_lock(&audit)==0);
    resume_copy=1;
    CHECK(pthread_cond_broadcast(&changed)==0);
    CHECK(pthread_mutex_unlock(&audit)==0);
    hw_close(&q);
    CHECK(pthread_join(worker,NULL)==0);
    CHECK(slow_dst==slow_src && calls==2);
    hw_queue_destroy(&q);
}

int main(int argc,char **argv) {
    CHECK(argc==2);
    if(strcmp(argv[1],"memory")==0) {
        size_t normalized=sizeof(hw_queue)-sizeof(pthread_mutex_t)-2*sizeof(pthread_cond_t);
        printf("%zu %zu\n",normalized,(size_t)(HW_CAPACITY*sizeof(hw_job)));
        return 0;
    }
    if(strcmp(argv[1],"copy_modes")==0) copy_modes();
    else if(strcmp(argv[1],"validation")==0) validation();
    else if(strcmp(argv[1],"fifo")==0) fifo();
    else if(strcmp(argv[1],"closing")==0) closing();
    else if(strcmp(argv[1],"concurrent")==0) threaded(false);
    else if(strcmp(argv[1],"pipeline")==0) threaded(true);
    else if(strcmp(argv[1],"progress")==0) progress();
    else CHECK(0);
    puts("PASS");
    return 0;
}
