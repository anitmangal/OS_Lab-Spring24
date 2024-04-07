#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/sem.h>

struct msg1_buffer {
    long msg_type;
    struct msg1 {
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
            msgrcv(mq1, &msg1, sizeof(msg1.msg), 0, 0);    // Current process in the ready queue

            int index = msg1.msg.index;
            sem_op.sem_num = index;
            semop(sem, &sem_op, 1);     // Signal the process to start
            

            msgrcv(mq2, &msg2, sizeof(msg2.msg), 0, 0);     // Status of the current process
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
