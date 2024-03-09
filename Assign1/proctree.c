/*
    OS Lab Assignment 1
    Anit Mangal
    21CS10005
*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#define LINE_LIMIT 100

int main(int argc, char * argv[]) {
    // If run with no city name
    if (argc <= 1) {
        printf("Run with a node name.\n");
        exit(0);
    }
    // Open treeinfo.txt
    FILE * f = fopen("treeinfo.txt", "r");
    if (f == NULL) {
        printf("File treeinfo.txt does not exist.\n");
        exit(0);
    }
    // Have default indentation as 0. If argument specifies indentation, overwrite.
    int indent = 0;
    if (argc == 3) indent = atoi(argv[2]);
    // Read lines until EOF
    char currline[LINE_LIMIT];
    while (fgets(currline, 100, f) != NULL) {
        // Get first word in currline
        char * str = strtok(currline, " \n");
        // Word matches city name
        if (strcmp(argv[1], str) == 0) {
            // Print city name with pid and indent properly
            for (int i = 0; i < indent; i++) printf("\t");
            int pid = getpid();
            printf("%s(%d)\n", str, pid);
            // Get number of children
            str = strtok(NULL, " \n");
            int numchilds = atoi(str);
            for (int i = 0; i < numchilds; i++) {
                str = strtok(NULL, " \n");
                if ((pid = fork()) == 0) {
                    // At child, execute program with child city and increment indentation.
                    char newind[100];
                    sprintf(newind, "%d", indent+1);
                    execl("./proctree", "blank", str, newind, NULL);
                }
                else waitpid(pid, NULL, 0); // Wait for subtree to finish
            }
            exit(0);
        }
    }
    // No word matched city name
    printf("City %s not found\n", argv[1]);
}