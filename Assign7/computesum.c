/*
    Operating Systems LA7: computesum.c
    Anit Mangal
    21CS10005
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <foothread.h>

// Global variables
int *sum, *P, *children;            // sum: partial sum at each node, P: parent of each node, children: number of children of each node
foothread_barrier_t * barriers;     // Barrier for each node
int n, root;
foothread_mutex_t sum_mutex;        // Mutex for sum
foothread_mutex_t io_mutex;         // Mutex for I/O

// Function to be executed by each thread
int child(void * args) {
    int i = *((int *)args);             // Get the node number
    int x;
    foothread_barrier_wait(&barriers[i]);       // Wait for all children to finish
    if (children[i] == 0) {
        // Leaf node
        foothread_mutex_lock(&io_mutex);
        printf("Leaf node %4d :: Enter a positive integer: ", i);
        scanf("%d", &x);
        foothread_mutex_unlock(&io_mutex);
        foothread_mutex_lock(&sum_mutex);
        sum[P[i]] += x;
        foothread_mutex_unlock(&sum_mutex);
    }
    else {
        // Internal node
        foothread_mutex_lock(&io_mutex);
        printf("Internal node %d gets the partial sum %d from its children\n", i, sum[i]);
        foothread_mutex_unlock(&io_mutex);
        if (i != root) {
            foothread_mutex_lock(&sum_mutex);
            sum[P[i]] += sum[i];
            foothread_mutex_unlock(&sum_mutex);
        }
    }
    if (i != root) foothread_barrier_wait(&barriers[P[i]]);
    free((int*)args);
    foothread_exit();       // Exit the thread
    return 0;
}

int main() {
    FILE * fp = fopen("tree.txt", "r");
    fscanf(fp, "%d", &n);

    // Create parent and children arrays and initialize them
    P = (int *)malloc(n * sizeof(int));
    children = (int *)malloc(n * sizeof(int));
    memset(P, -1, n * sizeof(int));
    memset(children, 0, n * sizeof(int));

    for (int i = 0; i < n; i++) {
        int ind;
        fscanf(fp, "%d", &ind);
        fscanf(fp, "%d", &P[ind]);
        if (P[ind] == ind) root = ind;
    }

    for (int i = 0; i < n; i++) children[P[i]]++;
    children[root]--;

    fclose(fp);

    // Initialise the shared resources
    sum = (int *)malloc(n * sizeof(int));
    memset(sum, 0, n * sizeof(int));

    barriers = (foothread_barrier_t *)malloc(n * sizeof(foothread_barrier_t));

    foothread_mutex_init(&sum_mutex);
    foothread_mutex_init(&io_mutex);

    foothread_t thread;

    foothread_attr_t attr;
    foothread_attr_setjointype(&attr, FOOTHREAD_JOINABLE);
    foothread_attr_setstacksize(&attr, FOOTHREAD_DEFAULT_STACK_SIZE);

    for (int i = 0; i < n; i++) {
        foothread_barrier_init(&barriers[i], children[i] + 1);

        // Create a thread for each node
        int * node = (int *)malloc(sizeof(int));
        *node = i;
        foothread_create(&thread, &attr, child, (void *)node);
    }

    foothread_exit();               // To synchronize the main thread with the children
    printf("Sum at root (node %d) = %d\n", root, sum[root]);

    // Clean up
    foothread_mutex_destroy(&sum_mutex);
    foothread_mutex_destroy(&io_mutex);
    for (int i = 0; i < n; i++) foothread_barrier_destroy(&barriers[i]);
    free(sum);
    free(barriers);
    free(children);
    free(P);
}