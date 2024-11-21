// monitor_init_and_queue.cpp

#include "monitor.h"
#include <iostream>
#include <unistd.h>
#include <sys/file.h>
#include <errno.h>
#include <time.h>
#include <string.h>

using namespace std;

// Initialize the monitor
void initializeMonitor(Monitor *monitor, SharedMemorySegment *shm_ptr) {
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    // Set the mutex to be shared between processes
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(monitor->mutex), &mutexAttr);

    pthread_condattr_t condAttr;
    pthread_condattr_init(&condAttr);
    pthread_condattr_setpshared(&condAttr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&(monitor->cond), &condAttr);

    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        pthread_mutex_init(&(monitor->account_mutexes[i]), &mutexAttr);
    }

    monitor->shm_ptr = shm_ptr;
}

// Destroy the monitor
void destroyMonitor(Monitor *monitor) {
    pthread_mutex_destroy(&(monitor->mutex));
    pthread_cond_destroy(&(monitor->cond));
    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        pthread_mutex_destroy(&(monitor->account_mutexes[i]));
    }
}

// Function to enter the monitor (enqueue)
void enterMonitor(Monitor *monitor) {
    pthread_mutex_lock(&(monitor->mutex));
    pid_t pid = getpid();
    monitor->process_queue.push(pid);
    cout << "Process " << pid << " added to queue.\n";
    while (monitor->process_queue.front() != pid) {
        pthread_cond_wait(&(monitor->cond), &(monitor->mutex));
    }
    pthread_mutex_unlock(&(monitor->mutex));
}

// Function to exit the monitor (dequeue)
void exitMonitor(Monitor *monitor) {
    pthread_mutex_lock(&(monitor->mutex));
    pid_t pid = getpid();
    if (!monitor->process_queue.empty() && monitor->process_queue.front() == pid) {
        monitor->process_queue.pop();
        pthread_cond_broadcast(&(monitor->cond));
    }
    pthread_mutex_unlock(&(monitor->mutex));
}

// Function to display the processes in queue
void displayProcessQueue(Monitor *monitor) {
    pthread_mutex_lock(&(monitor->mutex));
    queue<pid_t> tempQueue = monitor->process_queue;
    cout << "Processes in queue: ";
    while (!tempQueue.empty()) {
        cout << tempQueue.front() << " ";
        tempQueue.pop();
    }
    cout << endl;
    pthread_mutex_unlock(&(monitor->mutex));
}
