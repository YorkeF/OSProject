/**
* Group I
 * Cole Dunn
 * cole.dunn@okstate.edu
 * 11/20/2024
 */


#include "monitor.h"
#include <iostream>
#include <unistd.h>
#include <sys/file.h>
#include <errno.h>
#include <time.h>
#include <string.h>

using namespace std;



/**
 * @brief Initializes the monitor and its synchronization primitives.
 *
 * @param monitor Pointer to the monitor structure to initialize.
 * @param shm_ptr Pointer to the shared memory segment.
 */
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

/**
 * @brief Destroys the monitor and cleans up synchronization primitives.
 *
 * @param monitor Pointer to the monitor structure to destroy.
 */
void destroyMonitor(Monitor *monitor) {
    pthread_mutex_destroy(&(monitor->mutex));
    pthread_cond_destroy(&(monitor->cond));
    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        pthread_mutex_destroy(&(monitor->account_mutexes[i]));
    }
}

/**
 * @brief Enters the monitor by adding the process to the queue.
 *
 * The process will block until it is the first in the queue.
 *
 * @param monitor Pointer to the monitor structure.
 */
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

/**
 * @brief Exits the monitor by removing the process from the queue.
 *
 * @param monitor Pointer to the monitor structure.
 */
void exitMonitor(Monitor *monitor) {
    pthread_mutex_lock(&(monitor->mutex));
    pid_t pid = getpid();
    if (!monitor->process_queue.empty() && monitor->process_queue.front() == pid) {
        monitor->process_queue.pop();
        pthread_cond_broadcast(&(monitor->cond));
    }
    pthread_mutex_unlock(&(monitor->mutex));
}

/**
 * @brief Displays the current queue of processes in the monitor.
 *
 * @param monitor Pointer to the monitor structure.
 */
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
