/*
    OS Laboratory Assignment 2
    Anit Mangal
    21CS10005
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <time.h>

// Compact substitutes to save space
#define SELF "mgr"
#define FINISHED "fin"
#define SUSPENDED "sus"
#define TERMINATED "ter"
#define KILLED "kil"

// Global variables
int currpr = 0;         // Process that was set to run
char pt[11][4][6];      // Process Table (pid, pgid, status, name)

// Signal Handler
void sigHan(int sig) {
    if (currpr) {
        if (sig == SIGINT) {
            kill(atoi(pt[currpr][0]), SIGKILL);
            strcpy(pt[currpr][2], TERMINATED);
            printf("\n");
            currpr = 0;
        }
        else if (sig == SIGTSTP) {
            kill(atoi(pt[currpr][0]), SIGTSTP);
            strcpy(pt[currpr][2], SUSPENDED);
            printf("\n");
            currpr = 0;
        }
    }
    else printf("mgr> ");   // For cases where ^C or ^Z is pressed in mgr
}

int main() {
    // Populate first index of mgr
    signal(SIGINT, sigHan);
    signal(SIGTSTP, sigHan);
    srand((unsigned int)time(NULL));
    sprintf(pt[0][0], "%d", getpid());
    sprintf(pt[0][1], "%d", getpgid(getpid()));
    strcpy(pt[0][2], SELF);
    strcpy(pt[0][3], "mgr");

    int numJobs = 0;
    int status;

    // Main loop
    while(1) {
        char inp;
        printf("mgr> ");
        scanf("%s", &inp);
        if (inp == 'p') {
            printf("NO\tPID\t\tPGID\t\tSTATUS\t\t\tNAME\n");
            for (int i = 0; i <= numJobs; i++) {
                printf("%d\t", i);
                printf("%s\t\t", pt[i][0]);
                printf("%s\t\t", pt[i][1]);
                if (strcmp(pt[i][2], SELF) == 0) printf("SELF\t\t\t");
                else if (strcmp(pt[i][2], FINISHED) == 0) printf("FINISHED\t\t");
                else if (strcmp(pt[i][2], SUSPENDED) == 0) printf("SUSPENDED\t\t");
                else if (strcmp(pt[i][2], TERMINATED) == 0) printf("TERMINATED\t\t");
                else if (strcmp(pt[i][2], KILLED) == 0) printf("KILLED\t\t\t");
                printf("%s\n", pt[i][3]);
            }
        }
        else if (inp == 'r') {
            if (numJobs == 10) {
                printf("Process table is full. Quiting...\n");
                exit(1);
            }
            int pid;
            char arg[2];
            arg[1] = '\0';
            arg[0] = 'A' + rand() % 26;
            if ((pid = fork()) == 0) {
                // Child
                execl("./job", "blank", arg, NULL);
            }
            else {
                // Parent
                numJobs++;
                currpr = numJobs;
                setpgid(pid, 0);    // Set group id to pid for child
                // Populate process table
                sprintf(pt[numJobs][0], "%d", pid);
                sprintf(pt[numJobs][1], "%d", getpgid(pid));
                char nm[6];
                strcpy(nm, "job ");
                nm[4] = arg[0];
                nm[5] = '\0';
                strcpy(pt[numJobs][3], nm);

                waitpid(pid, &status, WUNTRACED);       // Wait for child to terminate or stop (WUNTRACED helps with stop)
                // Exited normally
                if (WEXITSTATUS(status) == 0 && currpr) {
                    strcpy(pt[currpr][2], FINISHED);
                    currpr = 0;
                }
            }
        }
        else if (inp == 'c') {
            int cnt = 0;
            int candidates[10];
            for (int i = 1; i <= numJobs; i++) {
                if (strcmp(pt[i][2], SUSPENDED) == 0) candidates[cnt++] = i;
            }
            if (cnt) {
                printf("Suspended jobs: ");
                for (int i = 0; i < cnt; i++) {
                    printf("%d", candidates[i]);
                    if (i < cnt-1) printf(", ");
                    else printf(" (Pick one): ");
                }
                int num;
                scanf("%d", &num);
                int check = 0;
                for (int i = 0; i < cnt; i++) if (candidates[i] == num) check = 1;
                if (check) {
                    currpr = num;
                    kill(atoi(pt[num][0]), SIGCONT);    // Continue signal
                    waitpid(atoi(pt[num][0]), &status, WUNTRACED);      // Wait for child to terminate or stop (WUNTRACED helps with stop)
                    // Exited normally
                    if (WEXITSTATUS(status) == 0 && currpr) {
                        strcpy(pt[currpr][2], FINISHED);
                        currpr = 0;
                    }
                }
                else printf("Invalid job number.\n");
            }
        }
        else if (inp == 'k') {
            int cnt = 0;
            int candidates[10];
            for (int i = 1; i <= numJobs; i++) {
                if (strcmp(pt[i][2], SUSPENDED) == 0) candidates[cnt++] = i;
            }
            if (cnt) {
                printf("Suspended jobs: ");
                for (int i = 0; i < cnt; i++) {
                    printf("%d", candidates[i]);
                    if (i < cnt-1) printf(", ");
                    else printf(" (Pick one): ");
                }
                int num;
                scanf("%d", &num);
                int check = 0;
                for (int i = 0; i < cnt; i++) if (candidates[i] == num) check = 1;
                if (check) {
                    kill(atoi(pt[num][0]), SIGKILL);            // Kill signal
                    strcpy(pt[num][2], KILLED);
                    currpr = 0;
                }
                else printf("Invalid job number.\n");
            }
        }
        else if (inp == 'h') {
            printf("\tCommand\t: Action\n\t   c\t: Continue a suspended job\n\t   h\t: Print this help message\n\t   k\t: Kill a suspended job\n\t   p\t: Print the process table\n\t   q\t: Quit\n\t   r\t: Run a new job\n");
        }
        else if (inp == 'q') {
            for (int i = 1; i <= numJobs; i++) {
                if (strcmp(pt[i][2], SUSPENDED) == 0) kill(atoi(pt[i][0]), SIGKILL);
            }
            exit(0);
        }
    }
}