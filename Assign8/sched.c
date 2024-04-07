#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/sem.h>

#include <stdio.h>
#include <unistd.h>

struct msg1_buffer {
    long msg_type;
    struct msg1 {
        // int pid;
        int index;
    } msg;
};

struct msg2_buffer {
    long msg_type;
    struct msg2 {
        int type_of_msg;
    } msg;
};

int main(int argc, char *argv[]) {
    printf("Scheduler started\n");
    int mq1 = atoi(argv[1]);
    int mq2 = atoi(argv[2]);
    int k = atoi(argv[3]);
    int sem = atoi(argv[4]);
    int notify_sem = atoi(argv[5]);

    struct msg1_buffer msg1;
    struct msg2_buffer msg2;

    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;

    while(1) {
        if (k == 0) {
            // msg1.msg_type = 2;
            // msgsnd(mq1, &msg1, sizeof(msg1.msg), 0);
            struct sembuf sem_op_notif;
            sem_op_notif.sem_num = 0;
            sem_op_notif.sem_op = 1;
            sem_op_notif.sem_flg = 0;
            semop(notify_sem, &sem_op_notif, 1);
        }
        else {
            printf("SCHEDULER: Waiting for rq\n");
            msgrcv(mq1, &msg1, sizeof(msg1.msg), 0, 0);    // Current process in the ready queue

            printf("SCHEDULER: Process %d\n", msg1.msg.index);    // Process to be scheduled (pid
            // int pid = msg1.msg.pid;                         // pid of the current process
            // sleep(1);
            // kill(pid, SIGCONT);
            // sleep(1);
            int index = msg1.msg.index;
            sem_op.sem_num = index;
            semop(sem, &sem_op, 1);     // Signal the process to start
            

            msgrcv(mq2, &msg2, sizeof(msg2.msg), 0, 0);     // Status of the current process
            printf("SCHEDULER: Process %d status %d\n", index, msg2.msg.type_of_msg);    // Status of the process (0: Terminated, 1: Page fault handled
            if (msg2.msg.type_of_msg == 1) {
                // PAGE FAULT HANDLED
                msg1.msg_type = 1;
                msg1.msg.index = index;
                msgsnd(mq1, &msg1, sizeof(msg1.msg), 0);
            }
            else {
                // PROCESS TERMINATED
                k--;
            }
        }
    }
    return 0;
}
