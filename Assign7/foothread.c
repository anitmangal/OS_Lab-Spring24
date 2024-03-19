/*
    Operating Systems LA7: foothread.c
    Anit Mangal
    21CS10005
*/

#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>

#include "foothread.h"

// Structure to store thread information
typedef struct foothread_t{
    pid_t id;
    int type;
    int stack_size;
} __foothread_t;

__foothread_t * foothread_table;    // Table to store thread information

int num_followers;          // Number of joinable threads
int leader_sem_id;          // Semaphore for leader to wait for followers
pid_t leader_tid;           // Leader's tid

// Function to create a new thread
void foothread_create(foothread_t * thread, foothread_attr_t * attr, int (*start_routine)(void *), void * args) {
    // If table is not initialized, initialize it
    if (foothread_table == NULL) {
        foothread_table = (__foothread_t *)malloc(FOOTHREAD_THREADS_MAX * sizeof(__foothread_t));
        if (foothread_table == NULL) {
            perror("foothread_create: malloc failed");
            exit(1);
        }

        // Since table is not initialized, this is the leader thread
        leader_tid = gettid();
        leader_sem_id = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);

        if (leader_sem_id == -1) {
            free(foothread_table);
            perror("foothread_create: semget failed");
            exit(1);
        }

        semctl(leader_sem_id, 0, SETVAL, 0);
    }

    // Find an empty slot in the table
    unsigned int i;
    for (i = 0; i < FOOTHREAD_THREADS_MAX; i++) if (foothread_table[i].id == 0) break;
    if (i == FOOTHREAD_THREADS_MAX) {
        errno = ENOMEM;
        *thread = -1;
        perror("foothread_create: too many threads");
        exit(1);
    }

    if (thread) *thread = i;    // Set the thread id
    else thread = &i;           // NULL permitted, set the thread id to the first empty slot

    // Set the thread information
    foothread_table[*thread].type = (attr == NULL) ? FOOTHREAD_DETACHED : attr->type;
    foothread_table[*thread].stack_size = (attr == NULL) ? FOOTHREAD_DEFAULT_STACK_SIZE : attr->stack_size;

    if (foothread_table[*thread].type == FOOTHREAD_JOINABLE) num_followers++;   // If the thread is joinable, increment the number of joinable threads

    void * stack = (void *)malloc(foothread_table[*thread].stack_size);   // Allocate stack for the thread
    if (stack == NULL) {
        errno = ENOMEM;
        *thread = -1;
        perror("foothread_create: malloc failed");
        exit(1);
    }

    // Create the thread
    foothread_table[*thread].id = clone(start_routine, stack+foothread_table[*thread].stack_size, CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_THREAD | CLONE_SIGHAND, args);
    if (foothread_table[*thread].id == -1) {
        free(stack);
        *thread = -1;
        perror("foothread_create: clone failed");
        exit(1);
    }
}

// Function to set the join type of a thread attr
void foothread_attr_setjointype (foothread_attr_t * attr, int type) {
    if (attr != NULL) attr->type = type;
}

// Function to set the stack size of a thread attr
void foothread_attr_setstacksize (foothread_attr_t * attr, int stack_size) {
    if (attr != NULL) attr->stack_size = stack_size;
}

// Function to synchronize the leader and followers
void foothread_exit() {
    pid_t tid = gettid();
    if (tid == leader_tid) {
        // Leader, wait on the semaphore for all followers to finish
        struct sembuf sem_op;
        sem_op.sem_num = 0;
        sem_op.sem_op = -num_followers;
        sem_op.sem_flg = 0;
        semop(leader_sem_id, &sem_op, 1);

        free(foothread_table);               // Free the table
        num_followers = 0;                   // Reset the number of joinable threads
        leader_tid = 0;                      // Reset the leader's tid
        semctl(leader_sem_id, 0, IPC_RMID);  // Remove the semaphore
    }
    else {
        // Follower
        // Find the thread in the table
        int i;
        for (i = 0; i < FOOTHREAD_THREADS_MAX; i++) if (foothread_table[i].id == tid) break;
        if (i == FOOTHREAD_THREADS_MAX) return;
        if (foothread_table[i].type == FOOTHREAD_DETACHED) {
            // If the thread is detached, remove it from the table
            foothread_table[i].id = 0;
            return;
        }
        // If the thread is joinable, signal the leader
        struct sembuf sem_op;
        sem_op.sem_num = 0;
        sem_op.sem_op = 1;
        sem_op.sem_flg = 0;
        semop(leader_sem_id, &sem_op, 1);
    }
}

// Function to initialize a mutex
void foothread_mutex_init(foothread_mutex_t * mutex) {
    if (mutex == NULL) {
        errno = EINVAL;
        perror("foothread_mutex_init");
        exit(1);
    }
    mutex->id = -1;
    mutex->locked = 0;
    mutex->sem_sync = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    semctl(mutex->sem_sync, 0, SETVAL, 1);
}

// Function to lock a mutex
void foothread_mutex_lock(foothread_mutex_t * mutex) {
    if (mutex->locked && mutex->id == gettid()) return;         // If the mutex is already locked by the current thread, return
    // If the mutex is locked by another thread, wait. If the mutex is not locked, lock it
    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    semop(mutex->sem_sync, &sem_op, 1);
    mutex->locked = 1;
    mutex->id = gettid();
}

// Function to unlock a mutex
void foothread_mutex_unlock(foothread_mutex_t * mutex) {
    // If the mutex is not locked or is locked by another thread, error
    if (mutex->locked == 0 || mutex->id != gettid()) {
        errno = EPERM;
        perror("foothread_mutex_unlock");
        exit(1);
    }

    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;
    // Before unlocking, set the id to -1 and signal any of the waiting threads
    mutex->locked = 0;
    mutex->id = -1;
    semop(mutex->sem_sync, &sem_op, 1);
}

// Function to destroy a mutex
void foothread_mutex_destroy(foothread_mutex_t * mutex) {
    semctl(mutex->sem_sync, 0, IPC_RMID);
    mutex->id = -1;
    mutex->locked = 0;
}

// Function to initialize a barrier
void foothread_barrier_init(foothread_barrier_t * barrier, int count) {
    barrier->count = 0;
    barrier->max = count;
    barrier->sem_sync = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT);
    semctl(barrier->sem_sync, 0, SETVAL, count);
}

// Function to wait on a barrier
void foothread_barrier_wait(foothread_barrier_t * barrier) {
    barrier->count++;
    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    semop(barrier->sem_sync, &sem_op, 1);
    sem_op.sem_op = 0;
    semop(barrier->sem_sync, &sem_op, 1);
}

// Function to destroy a barrier
void foothread_barrier_destroy(foothread_barrier_t * barrier) {
    semctl(barrier->sem_sync, 0, IPC_RMID);
    barrier->count = 0;
    barrier->max = 0;
}