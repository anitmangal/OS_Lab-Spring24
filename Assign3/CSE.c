#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc <= 1) {
        // Parent
        int pfd1[2], pfd2[2];
        pipe(pfd1);
        pipe(pfd2);
        int orig_stdin = dup(0);
        int orig_stdout = dup(1);
        printf("+++ CSE in supervisor mode: Started\n");
        printf("+++ CSE in supervisor mode: pfd = [%d %d]\n", pfd1[0], pfd1[1]);
        int pid1, pid2;
        char cpfd1_1[3], cpfd1_2[3], cpfd2_1[3], cpfd2_2[3], cstdin[3], cstdout[3];
        sprintf(cpfd1_1, "%d", pfd1[0]);
        sprintf(cpfd1_2, "%d", pfd1[1]);
        sprintf(cpfd2_1, "%d", pfd2[0]);
        sprintf(cpfd2_2, "%d", pfd2[1]);
        sprintf(cstdin, "%d", orig_stdin);
        sprintf(cstdout, "%d", orig_stdout);
        // First child
        printf("+++ CSE in supervisor mode: Forking first child in command-input mode\n");
        if ((pid1 = fork()) == 0) {
            execlp("xterm", "xterm", "-T", "First Child", "-e", "./CSE", "C", cpfd1_1, cpfd1_2, cpfd2_1, cpfd2_2, cstdin, cstdout, NULL);
        }
        else {
            // Second child
            printf("+++ CSE in supervisor mode: Forking second child in execute mode\n");
            if ((pid2 = fork()) == 0) {
                execlp("xterm", "xterm", "-T", "Second Child", "-e", "./CSE", "E", cpfd1_1, cpfd1_2, cpfd2_1, cpfd2_2, cstdin, cstdout, NULL);
            }
            else {
                waitpid(pid1, NULL, 0);
                printf("Parent: First child terminated\n");
                waitpid(pid2, NULL, 0);
                printf("Parent: Second child terminated\n");
            }
        }
    }
    else {
        int pipefd1[2];
        int pipefd2[2];
        int orig[2];
        pipefd1[0] = atoi(argv[2]);
        pipefd1[1] = atoi(argv[3]);
        pipefd2[0] = atoi(argv[4]);
        pipefd2[1] = atoi(argv[5]);
        orig[0] = dup(0);   // Save stdin
        orig[1] = dup(1);   // Save stdout
        int mode;   // 1 : command-input mode, 0 : execute mode
        if (strcmp(argv[1], "C") == 0) {
            // C does not need read end of pipe1 and write end of pipe2
            close(pipefd1[0]);
            close(pipefd2[1]);

            // Redirect stdout to write end of pipe1
            orig[1] = dup(1);
            close(1);
            dup(pipefd1[1]);
            mode = 1;
        }
        else {
            // E does not need write end of pipe1 and read end of pipe2
            close(pipefd1[1]);
            close(pipefd2[0]);

            // Redirect stdin to read end of pipe1
            orig[0] = dup(0);
            close(0);
            dup(pipefd1[0]);
            mode = 0;
        }
        char command[100];
        while(1) {
            if (!mode) {
                // Execute mode
                printf("Waiting for command> ");
                fflush(stdout);
                fgets(command, 100, stdin);
                command[strlen(command) - 1] = '\0'; // remove \n
                printf("%s\n", command);
                fflush(stdout);

                if (strcmp(command, "exit") == 0) {
                    exit(0);
                }
                else if (strcmp(command, "swaprole") == 0) {
                    // Swap role: swap modes and swap pipes
                    mode = !mode;
                    if (strcmp(argv[1], "E") == 0) {
                        pipefd1[0] = dup(0);
                        close(0);
                        dup(orig[0]);
                        close(1);
                        dup(pipefd2[1]);
                        close(pipefd2[1]);
                    }
                    else {
                        pipefd2[0] = dup(0);
                        close(0);
                        dup(orig[0]);
                        close(1);
                        dup(pipefd1[1]);
                        close(pipefd1[1]);
                    }
                }
                else {
                    // Execute command
                    char *args = strtok(command, " ");
                    char *argv1[100];
                    int i = 0;
                    while (args != NULL) {
                        argv1[i++] = args;
                        args = strtok(NULL, " ");
                    }
                    argv1[i] = NULL;
                    int pid = fork();
                    if (pid == 0) {
                        close(0);
                        dup(orig[0]);
                        // dup2(orig[0], 0);
                        execvp(argv1[0], argv1);
                    }
                    else {
                        waitpid(pid, NULL, 0);
                    }
                }
            }
            else {
                // Command-input mode
                fprintf(stderr, "Enter command> ");
                fgets(command, 100, stdin);
                command[strlen(command) - 1] = '\0'; // remove \n
                printf("%s\n", command);
                fflush(stdout);
                if (strcmp(command, "exit") == 0) {
                    exit(0);
                }
                else if (strcmp(command, "swaprole") == 0) {
                    // Swap role: swap modes and swap pipes
                    mode = !mode;
                    if (strcmp(argv[1], "C") == 0) {
                        pipefd1[1] = dup(1);
                        close(1);
                        dup(orig[1]);
                        close(0);
                        dup(pipefd2[0]);
                        close(pipefd2[0]);
                    }
                    else {
                        pipefd2[1] = dup(1);
                        close(1);
                        dup(orig[1]);
                        close(0);
                        dup(pipefd1[0]);
                        close(pipefd1[0]);
                    }
                }
            }
        }
    }
}