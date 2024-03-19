/*
    Operating Systems LA7: foothread.h
    Anit Mangal
    21CS10005
*/
#ifndef FOOTHEAD_H
#define FOOTHEAD_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <errno.h>

// Macros
#define FOOTHREAD_THREADS_MAX 40
#define FOOTHREAD_DEFAULT_STACK_SIZE 2097152
#define FOOTHREAD_DETACHED 0
#define FOOTHREAD_JOINABLE 1
#define FOOTHREAD_ATTR_INITIALIZER {FOOTHREAD_DEFAULT_STACK_SIZE, FOOTHREAD_DETACHED}

// Types
typedef unsigned int foothread_t;
typedef struct foothread_attr_t {
    int type;
    int stack_size;
} foothread_attr_t;

typedef struct foothread_mutex_t{
    pid_t id;
    int sem_sync;
    int locked;
} foothread_mutex_t;

typedef struct foothread_barrier_t{
    int sem_sync;
    int count;
    int max;
} foothread_barrier_t;

// Function prototypes
extern void foothread_create(foothread_t *, foothread_attr_t *, int (*)(void *), void *);
extern void foothread_attr_setjointype (foothread_attr_t *, int);
extern void foothread_attr_setstacksize (foothread_attr_t *, int);
extern void foothread_exit();

extern void foothread_mutex_init(foothread_mutex_t *);
extern void foothread_mutex_lock(foothread_mutex_t *);
extern void foothread_mutex_unlock(foothread_mutex_t *);
extern void foothread_mutex_destroy(foothread_mutex_t *);

extern void foothread_barrier_init(foothread_barrier_t *, int);
extern void foothread_barrier_wait(foothread_barrier_t *);
extern void foothread_barrier_destroy(foothread_barrier_t *);
#endif

#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>