#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <signal.h>

struct msg1_buffer {
    long msg_type;
    struct msg {
        int pid;
    } msg;
};

struct msg2_buffer {
    long msg_type;
    struct msg {
        int pid;
    } msg;
};

int main(int argc, char *argv[]) {
    int mq1 = atoi(argv[1]);
    int mq2 = atoi(argv[2]);
    int k = atoi(argv[3]);

    struct msg1_buffer msg1;
    struct msg2_buffer msg2;
    while(1) {
        if (k == 0) {
            msg1.msg_type = 2;
            msgsnd(mq1, &msg1, sizeof(msg1.msg), 0);
        }
        msgrcv(mq1, &msg1, sizeof(msg1.msg), 1, 0);     // Type 1 message from ready queue

        int pid = msg1.msg.pid;
        kill(pid, SIGCONT);

        msgrcv(mq2, &msg2, sizeof(msg2.msg), 0, 0);     // Any message
        if (msg2.msg_type == 1) {
            // PAGE FAULT HANDLED
            msg1.msg_type = 1;
            msg1.msg.pid = msg2.msg.pid;
            msgsnd(mq1, &msg1, sizeof(msg1.msg), 0);
        }
        else {
            // PROCESS TERMINATED
            k--;
        }
    }
}