/*
    OS Laboratory Assignment 5: boss.c
    Anit Mangal
    21CS10005
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

int main() {
    FILE * f = fopen("graph.txt", "r");
    int n;
    fscanf(f, "%d", &n);

    // Create shared memory. Get key from ftok, get shared memory id from shmget, attach shared memory to pointer from shmat.
    key_t key_shm_idx = ftok("graph.txt", 'I');
    int shm_idx = shmget(key_shm_idx, sizeof(int), 0777|IPC_CREAT);
    int * idx = (int *)shmat(shm_idx, 0, 0);
    *idx = 0;

    key_t key_shm_A = ftok("graph.txt", 'A');
    int shm_A = shmget(key_shm_A, n * n * sizeof(int), 0777|IPC_CREAT);
    int * A = (int *)shmat(shm_A, 0, 0);

    key_t key_shm_T = ftok("graph.txt", 'T');
    int shm_T = shmget(key_shm_T, n * sizeof(int), 0777|IPC_CREAT);
    int * T = (int *)shmat(shm_T, 0, 0);

    if (shm_A == -1 || shm_T == -1 || shm_idx == -1) {
        printf("+++Boss: Error in creating shared memory.\n");
        return 1;
    }

    // Build A from file.
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            fscanf(f, "%d", &A[i * n + j]);
        }
    }
    fclose(f);

    // Create semaphores. Get key from ftok, get semaphore id from semget, set semaphore value from
    struct sembuf pop, vop;
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = 0;
    pop.sem_op = -1;
    vop.sem_op = 1;

    key_t key_sem_mtx = ftok("graph.txt", 'M');
    int sem_mtx = semget(key_sem_mtx, 1, 0777|IPC_CREAT);
    semctl(sem_mtx, 0, SETVAL, 1);

    key_t key_sem_sync = ftok("graph.txt", 'S');
    int sem_sync = semget(key_sem_sync, n, 0777|IPC_CREAT);
    for (int i = 0; i < n; i++) {
        semctl(sem_sync, i, SETVAL, 0);
    }

    key_t key_sem_ntfy = ftok("graph.txt", 'N');
    int sem_ntfy = semget(key_sem_ntfy, 1, 0777|IPC_CREAT);
    semctl(sem_ntfy, 0, SETVAL, 0);

    if (sem_mtx == -1 || sem_sync == -1 || sem_ntfy == -1) {
        printf("+++Boss: Error in creating semaphores.\n");
        return 1;
    }

    printf("+++Boss: Setup done.\n");

    // Wait for all workers to finish. This is when value of sem_ntfy reaches n.
    pop.sem_op = -n;
    semop(sem_ntfy, &pop, 1);
    // Print topological sorting of the vertices.
    printf("+++ Topological sorting of the vertices:\n");
    for (int i = 0; i < n; i++) printf("%d ", T[i]);
    printf("\n");

    // Check for precedence constraints.
    int done[n];
    int err_cnt = 0;
    for (int i = 0; i < n; i++) done[i] = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (A[j*n + T[i]] == 1) {
                if (done[j] == 0) {
                    err_cnt++;
                }
            }
        }
        done[T[i]] = 1;
    }
    if (err_cnt == 0) printf("+++Boss: Well done, my team.\n");
    else printf("+++Boss: %d many precedence constraints have been violated.\n", err_cnt);

    // Detach and remove shared memory and semaphores.
    shmctl(shm_idx, IPC_RMID, 0);
    shmctl(shm_A, IPC_RMID, 0);
    shmctl(shm_T, IPC_RMID, 0);

    semctl(sem_mtx, 0, IPC_RMID, 0);
    semctl(sem_sync, 0, IPC_RMID, 0);
    semctl(sem_ntfy, 0, IPC_RMID, 0);
}