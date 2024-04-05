#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

struct msg1_buffer {
    long msg_type;
    struct msg {
        int pid;
    } msg;
};

struct msg3_buffer {
    long msg_type;
    struct msg{
        int pg_num;
    }msg;
};

int main(int argc, char * argv[]) {
    int mq1 = atoi(argv[1]);
    int mq3 = atoi(argv[2]);
    char * refstr = argv[3];
    int p_ind = atoi(argv[4]);

    // Attach in ready queue
    struct msg1_buffer msg1;
    msg1.msg_type = 1;
    msg1.msg.pid = getpid();
    msgsnd(mq1, &msg1, sizeof(msg1.msg), 0);

    // Pause until scheduler wakes up
    kill(getpid(), SIGSTOP);

    struct msg3_buffer msg3;

    char * ref = strtok(refstr, " ");
    while (ref != NULL) {
        msg3.msg_type = 2*p_ind+1;
        msg3.msg.pg_num = atoi(ref);
        msgsnd(mq3, &msg3, sizeof(msg3.msg), 0);

        if (atoi(ref) == -9) break;

        msgrcv(mq3, &msg3, sizeof(msg3.msg), 2*p_ind+2, 0);
        if (msg3.msg.pg_num == -1) {
            // Page fault
            kill(getpid(), SIGSTOP);
        }
        else if (msg3.msg.pg_num == -2) {
            exit(0);
        }
        else {
            // Got valid page, go to next.
            ref = strtok(NULL, " ");
        }
    }
}