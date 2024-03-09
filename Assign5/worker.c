/*
    OS Laboratory Assignment 5: worker.c
    Anit Mangal
    21CS10005
*/

#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

int main(int argc, char *argv[]) {
    int n = atoi(argv[1]);
    int id = atoi(argv[2]);

    // Get shared memory. Get key from ftok, get shared memory id from shmget, attach shared memory to pointer from shmat.
    key_t key_shm_A = ftok("graph.txt", 'A');
    int shm_A = shmget(key_shm_A, n * n * sizeof(int), 0777);
    int * A = (int *)shmat(shm_A, 0, 0);

    key_t key_shm_T = ftok("graph.txt", 'T');
    int shm_T = shmget(key_shm_T, n * sizeof(int), 0777);
    int * T = (int *)shmat(shm_T, 0, 0);

    key_t key_shm_idx = ftok("graph.txt", 'I');
    int shm_idx = shmget(key_shm_idx, sizeof(int), 0777);
    int * idx = (int *)shmat(shm_idx, 0, 0);

    // Get semaphores. Get key from ftok, get semaphore id from semget.
    key_t key_sem_mtx = ftok("graph.txt", 'M');
    int sem_mtx = semget(key_sem_mtx, 1, 0777);

    key_t key_sem_sync = ftok("graph.txt", 'S');
    int sem_sync = semget(key_sem_sync, n, 0777);

    key_t key_sem_ntfy = ftok("graph.txt", 'N');
    int sem_ntfy = semget(key_sem_ntfy, 1, 0777);

    struct sembuf pop, vop;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1;
    vop.sem_op = 1;

    // Get dependecy count.
    int cnt = 0;
    for (int i = 0; i < n; i++) {
        if (A[i * n + id] == 1) {
            cnt++;
        }
    }
    // Wait for sem_sync to become equal to dependency count.
    pop.sem_num = id;
    pop.sem_op = -cnt;
    semop(sem_sync, &pop, 1);

    // Wait for sem_mtx to become 1
    pop.sem_num = 0;
    pop.sem_op = -1;
    semop(sem_mtx, &pop, 1);
    T[*idx] = id;
    (*idx)++;
    semop(sem_mtx, &vop, 1);        // Unlock sem_mtx

    // Signal all semaphores in sem_sync that depend.
    for (int j = 0; j < n; j++) {
        if (A[id * n + j] == 1) {
            vop.sem_num = j;
            semop(sem_sync, &vop, 1);
        }
    }

    // Increment sem_ntfy.
    vop.sem_num = 0;
    semop(sem_ntfy, &vop, 1);

    // Detach shared memory.
    shmdt(A);
    shmdt(T);
    shmdt(idx);
}