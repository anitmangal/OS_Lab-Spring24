/* 
    Operating Systems LA - 6
    Anit Mangal
    21CS10005
*/

#include <stdio.h>
#include <stdlib.h>
#include <event.h>
#include <pthread.h>
#include <string.h>

int patientCount = 0, salesRepCount = 0, reporterCount = 0;     // Count of visitors (token system to check if quota is full or not)
int reportersWait = 0, patientsWait = 0, salesRepsWait = 0;     // Count of visitors waiting
int sessionOver = 0;                                            // Flag to check if session is over
int currTime = 0;                                               // Current time
int currEventDuration = 0;                                      // Duration of event just serviced
int pServiced = 0, sServiced = 0;                               // Count of patients and sales reps serviced
int doctorFree = 0, visitorFree = 0;                            // Flags to check if doctor and visitor are free

// Condition variables for each type of visitor and doctor
pthread_cond_t reporterCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t patientCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t salesRepCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t doctorCond = PTHREAD_COND_INITIALIZER;

// Mutexes for a visitor and doctor to start their service
pthread_mutex_t doctorMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t visitorMutex = PTHREAD_MUTEX_INITIALIZER;

// Barriers for doctor and visitor, to let assistant know that they are done
pthread_barrier_t doctorDoneBarrier;
pthread_barrier_t visitorDoneBarrier;

// Structure to pass information to visitor thread
typedef struct {
    char type;
    int count;
    int duration;
} threadType;

// Function to print current time
void printCurrentTime(int t) {
    if (t < 0) {
        printf("%02d:%02d", 8-(-t)/60, 60+(t)%60);
        printf("am");
        return;
    }
    if (t / 60 + 9 <= 12) {
        printf("%02d:%02d", t / 60 + 9, t % 60);
        if (t/60 + 9 < 12) printf("am");
        else printf("pm");
    } else {
        printf("%02d:%02d", t / 60 - 3, t % 60);
        printf("pm");
    }
}


// Visitor thread
void * visitor(void * theadInfo) {
    threadType * info = (threadType *)theadInfo;

    pthread_mutex_lock(&visitorMutex);
    // Wait for assistant to signal
    if (info->type == 'R') while (visitorFree == 0) pthread_cond_wait(&reporterCond, &visitorMutex);
    else if (info->type == 'P') while (visitorFree == 0) pthread_cond_wait(&patientCond, &visitorMutex);
    else if (info->type == 'S') while (visitorFree == 0) pthread_cond_wait(&salesRepCond, &visitorMutex);
    visitorFree = 0;

    currEventDuration = info->duration;
    printf("[");
    printCurrentTime(currTime);
    printf(" - ");
    printCurrentTime(currTime + currEventDuration);
    if (info->type == 'R') printf("] Reporter %d is in doctor's chamber\n", info->count);
    else if (info->type == 'P') printf("] Patient %d is in doctor's chamber\n", info->count);
    else if (info->type == 'S') printf("] Sales representative %d is in doctor's chamber\n", info->count);

    if (info->type == 'R') {
        reportersWait--;
    }
    else if (info->type == 'P') {
        patientsWait--;
        pServiced = info->count;
    }
    else if (info->type == 'S') {
        salesRepsWait--;
        sServiced = info->count;
    }

    free(info);
    pthread_mutex_unlock(&visitorMutex);
    pthread_barrier_wait(&visitorDoneBarrier);          // Let assistant know that visitor is done

    pthread_exit(NULL);
}

void * doctor() {
    while(1) {
        pthread_mutex_lock(&doctorMutex);
        // Wait for assistant to signal
        while (doctorFree == 0) pthread_cond_wait(&doctorCond, &doctorMutex);
        doctorFree = 0;
        if (sessionOver) {
            printf("[");
            printCurrentTime(currTime);
            printf("] Doctor leaves\n");
            pthread_mutex_unlock(&doctorMutex);
            pthread_barrier_wait(&doctorDoneBarrier);
            pthread_exit(NULL);
        }
        else {
            printf("[");
            printCurrentTime(currTime);
            printf("] Doctor has next visitor\n");
        }
        pthread_mutex_unlock(&doctorMutex);
        pthread_barrier_wait(&doctorDoneBarrier);       // Let assistant know that doctor is done
    }
    return NULL;
}

int main() {

    // Initialize event queue, add doctor's arrival
    eventQ E = initEQ("arrival.txt");
    event e = {'D', 0, 0};
    E = addevent(E, e);

    // Thread attribute
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    // Create doctor thread
    pthread_t doctorThread;
    pthread_create(&doctorThread, &attr, doctor, NULL);

    // Initialize barriers
    pthread_barrier_init(&doctorDoneBarrier, NULL, 2);
    pthread_barrier_init(&visitorDoneBarrier, NULL, 2);

    // Service each event until queue is empty
    while (!emptyQ(E)) {
        e = nextevent(E);
        E = delevent(E);

        if (e.type == 'R') {
            // Reporter arrives
            ++reportersWait;
            ++reporterCount;
            printf("\t\t[");
            printCurrentTime(e.time);
            printf("] Reporter %d arrives\n", reporterCount);

            if (sessionOver) {
                printf("\t\t[");
                printCurrentTime(e.time);
                printf("] Reporter %d leaves (session over)\n", reporterCount);
                --reportersWait;
            }
            else {
                // Create reporter thread
                pthread_t reporterThread;
                threadType * threadInfo = (threadType *)malloc(sizeof(threadType));
                threadInfo->type = 'R';
                threadInfo->count = reporterCount;
                threadInfo->duration = e.duration;
                pthread_create(&reporterThread, &attr, visitor, (void *)threadInfo);
            }
        }
        else if (e.type == 'P') {
            // Patient arrives
            ++patientsWait;
            ++patientCount;
            printf("\t\t[");
            printCurrentTime(e.time);
            printf("] Patient %d arrives\n", patientCount);

            if (sessionOver) {
                printf("\t\t[");
                printCurrentTime(e.time);
                printf("] Patient %d leaves (session over)\n", patientCount);
                --patientsWait;
            }
            else if (patientCount > 25) {
                printf("\t\t[");
                printCurrentTime(e.time);
                printf("] Patient %d leaves (quota full)\n", patientCount);
                --patientsWait;
            }
            else {
                // Create patient thread
                pthread_t patientThread;
                threadType * threadInfo = (threadType *)malloc(sizeof(threadType));
                threadInfo->type = 'P';
                threadInfo->count = patientCount;
                threadInfo->duration = e.duration;
                pthread_create(&patientThread, &attr, visitor, (void *)threadInfo);
            }
        }
        else if (e.type == 'S') {
            // Sales representative arrives
            ++salesRepsWait;
            ++salesRepCount;
            printf("\t\t[");
            printCurrentTime(e.time);
            printf("] Sales representative %d arrives\n", salesRepCount);

            if (sessionOver) {
                printf("\t\t[");
                printCurrentTime(e.time);
                printf("] Sales representative %d leaves (session over)\n", salesRepCount);
                --salesRepsWait;
            }
            else if (salesRepCount > 3) {
                printf("\t\t[");
                printCurrentTime(e.time);
                printf("] Sales representative %d leaves (quota full)\n", salesRepCount);
                --salesRepsWait;
            }
            else {
                // Create sales representative thread
                pthread_t salesRepThread;
                threadType * threadInfo = (threadType *)malloc(sizeof(threadType));
                threadInfo->type = 'S';
                threadInfo->count = salesRepCount;
                threadInfo->duration = e.duration;
                pthread_create(&salesRepThread, &attr, visitor, (void *)threadInfo);
            }
        }
        
        if ((e.type == 'D' || e.time > currTime) && !sessionOver) {
            // Doctor is available

            // Check if session is over
            pthread_mutex_lock(&doctorMutex);
            if (pServiced == 25 && sServiced == 3) {
                sessionOver = 1;
            }
            pthread_mutex_unlock(&doctorMutex);

            if (!sessionOver && (reportersWait || patientsWait || salesRepsWait)) {
                // Doctor is free and there are visitors waiting

                // Signal doctor
                pthread_mutex_lock(&doctorMutex);
                currTime = e.time;
                doctorFree = 1;
                pthread_cond_signal(&doctorCond);
                pthread_mutex_unlock(&doctorMutex);
                pthread_barrier_wait(&doctorDoneBarrier);       // Wait for doctor to finish

                // Signal visitor
                pthread_mutex_lock(&visitorMutex);
                visitorFree = 1;
                if (reportersWait) pthread_cond_signal(&reporterCond);
                else if (patientsWait) pthread_cond_signal(&patientCond);
                else if (salesRepsWait) pthread_cond_signal(&salesRepCond);
                pthread_mutex_unlock(&visitorMutex);
                pthread_barrier_wait(&visitorDoneBarrier);    // Wait for visitor to finish
                
                // Add next doctor available event
                e.type = 'D';
                e.time = currTime + currEventDuration;
                e.duration = 0;
                E = addevent(E, e);
                currTime = e.time;          // Update time to next doctor available time
            }
            else if (sessionOver) {
                // Session is over, signal doctor last time to leave
                pthread_mutex_lock(&doctorMutex);
                doctorFree = 1;
                pthread_cond_signal(&doctorCond);
                pthread_mutex_unlock(&doctorMutex);
                pthread_barrier_wait(&doctorDoneBarrier);
            }
        }
    }
}