/*
    OS Laboratory Assignment 4
    Anit Mangal
    21CS10005
*/
#include <stdlib.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    srand(time(NULL));
    int shmid = shmget(IPC_PRIVATE, 2*sizeof(int), 0777|IPC_CREAT);             // allocate shared memory
    if (shmid == -1) {
        printf("Error creating shared memory segment\n");
        exit(1);
    }
    int * M = shmat(shmid, NULL, 0);            // attach shared memory
    M[0] = 0;
    int n, t;
    printf("n = ");
    scanf("%d", &n);
    printf("t = ");
    scanf("%d", &t);
    int childcnt[n+1], childitemsum[n+1], childpid[n+1];        // childcnt keeps track of number of items produced for each child, childitemsum keeps track of sum of items produced for each child, childpid keeps track of child pids
    int childnum;
    for (childnum = 1; childnum <= n; childnum++) {
        if ((childpid[childnum] = fork()) == 0) {
            // CONSUMER
            printf("\t\t\t\tConsumer %d is alive\n", childnum);
            int numitems = 0;
            int sumitems = 0;
            while (1) {
                while (M[0] == 0);
                if (M[0] == -1) {
                    // Child exits
                    printf("\t\t\t\tConsumer %d has read %d items: Checksum = %d\n", childnum, numitems, sumitems);
                    shmdt(M);           // detach shared memory
                    exit(0);
                }
                else if (M[0] == childnum) {
                    // Child reads
                    int item = M[1];
                    numitems++;
                    sumitems += item;
                    #ifdef VERBOSE
                        printf("\t\t\t\tConsumer %d reads %d\n", childnum, item);
                    #endif
                    M[0] = 0;
                }
            }
        }
        else {
            childcnt[childnum] = 0;
            childitemsum[childnum] = 0;
        }
    }
    // PRODUCER
    printf("Producer is alive\n");
    for (int i = 0; i < t; i++) {
        int item = 100 + rand()%900;
        int c = 1 + rand()%n;
        while (M[0] != 0);
        M[0] = c;
        #ifdef SLEEP
            usleep(1 + rand()%10);
        #endif
        M[1] = item;
        #ifdef VERBOSE
            printf("Producer produces %d for Consumer %d\n", item, c);
        #endif
        childcnt[c]++;
        childitemsum[c] += item;
    }
    while (M[0] != 0);
    M[0] = -1;
    for (childnum = 1; childnum <= n; childnum++) waitpid(childpid[childnum], NULL, 0);
    printf("Producer has produced %d items\n", t);
    for (childnum = 1; childnum <= n; childnum++) printf("%d items for Consumer %d: Checksum = %d\n", childcnt[childnum], childnum, childitemsum[childnum]);
    shmdt(M);           // detach shared memory
    shmctl(shmid, IPC_RMID, NULL);          // deallocate shared memory
}