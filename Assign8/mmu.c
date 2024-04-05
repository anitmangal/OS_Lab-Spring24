// For MQ3, remember that process sends pg request with type (index*2+1). mmu sends it a reply with type (index*2 + 2)

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>

// Define message structure for communication
struct message {
    long mtype; // Process ID for Type I messages, special values (-9, -2) for Type II
    int page_number;
};

// Prototype for the PageFaultHandler
void PageFaultHandler(int page_number, int process_id, int* page_table, int* free_frame_list);

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("Usage: ./mmu <page_table_shm_id> <free_frame_list_shm_id> <mq2_id> <mq3_id>\n");
        exit(1);
    }

    // Convert command line arguments
    int shm_page_table_id = atoi(argv[1]);
    int shm_free_frame_list_id = atoi(argv[2]);
    int mq2 = atoi(argv[3]);
    int mq3 = atoi(argv[4]);

    // Attach to shared memory
    int *page_table = (int*)shmat(shm_page_table_id, NULL, 0);
    int *free_frame_list = (int*)shmat(shm_free_frame_list_id, NULL, 0);

    // Global timestamp
    int global_timestamp = 0;

    struct message msg;
    while(1) {
        // Wait for a page number from any process
        if (msgrcv(mq3, &msg, sizeof(msg) - sizeof(long), 0, 0) < 0) {
            perror("msgrcv");
            break;
        }

        global_timestamp++; // Increment global timestamp

        int page_number = msg.page_number;
        int process_id = msg.mtype; // Using mtype to store process ID

        if (page_number == -9) {
            // Process completed execution
            // Update free frame list and release frames (not shown for brevity)
            // Send Type II message to Scheduler
        } else if (page_number == -2) {
            printf("TRYING TO ACCESS INVALID PAGE REFERENCE\n");
            // Send Type II message to Scheduler
        } else {
            // Check page table for the page
            // Assume page_table is an array where index represents the page number
            // and the value at each index is the frame number or -1 if not present
            if (page_table[page_number] != -1) {
                // Page found in table, send frame number back to process
            } else {
                // Page fault, call PageFaultHandler
                PageFaultHandler(page_number, process_id, page_table, free_frame_list);
                // Send Type I message to Scheduler
            }
        }
    }

    // Cleanup before exit
    shmdt(page_table);
    shmdt(free_frame_list);

    return 0;
}

void PageFaultHandler(int page_number, int process_id, int* page_table, int* free_frame_list) {
    // Implement page fault handling
    // 1. Check for a free frame
    // 2. If no free frame, select a victim page using LRU and replace it
    // Update page table and free frame list accordingly
}
