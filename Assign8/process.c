#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/sem.h>

struct msg1_buffer {
    long msg_type;
    struct msg1 {
        int index;
    } msg;
};                                          // To push into the ready queue

struct msg3_buffer {
    long msg_type;
    struct msg3 {
        int pg_num;
        int index;
    }msg;
};                                          // To request for a page from mq3

int main(int argc, char * argv[]) {
    int mq1 = atoi(argv[1]);
    int mq3 = atoi(argv[2]);
    char * refstr = argv[3];
    int p_ind = atoi(argv[4]);
    int sem = atoi(argv[5]);

    struct sembuf sem_op;
    sem_op.sem_num = p_ind;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;

    // Attach in ready queue
    struct msg1_buffer msg1;
    msg1.msg_type = 1;
    msg1.msg.index = p_ind;
    msgsnd(mq1, &msg1, sizeof(msg1.msg), 0);

    // Pause until scheduler wakes up
    semop(sem, &sem_op, 1);

    struct msg3_buffer msg3;

    char * ref = strtok(refstr, " ");
    int page_fault_occurred = 0; // Flag to restart if page fault occurs
    while (ref != NULL) {
        msg3.msg_type = 1;          // Request for a page
        msg3.msg.pg_num = atoi(ref);
        msg3.msg.index = p_ind;     // Index of the process (0 based)
        msgsnd(mq3, &msg3, sizeof(msg3.msg), 0);

        if (atoi(ref) == -9) break;

        msgrcv(mq3, &msg3, sizeof(msg3.msg), 2, 0);     // Get the frame from mmu
        if (msg3.msg.pg_num == -1) {
            // Page fault
            page_fault_occurred = 1;     // Setting the flag to 1 if a page fault occurs
            semop(sem, &sem_op, 1);     // Pause until scheduler wakes up
            page_fault_occurred = 0;     // Reset the flag immediately after stopping the process
        }
        else if (msg3.msg.pg_num == -2) {
            exit(0);
        }
        else {
            // Got valid page, go to next.
            if (!page_fault_occurred) {     // Only advance to the next token if no page fault occurred
                ref = strtok(NULL, " ");
            }
        }
    }
    return 0;
}
