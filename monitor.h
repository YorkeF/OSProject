// monitor.h

#ifndef MONITOR_H
#define MONITOR_H

#include <pthread.h>
#include <queue>
#include "sharedmemory.h"

#define MAX_ACCOUNTS 100  // Define maximum number of accounts

// Monitor structure
struct Monitor {
    pthread_mutex_t mutex;             // Mutex for synchronization
    pthread_cond_t cond;               // Condition variable for queue
    std::queue<pid_t> process_queue;   // Queue to manage process access
    pthread_mutex_t account_mutexes[MAX_ACCOUNTS]; // Mutexes for accounts (deadlock prevention)
    SharedMemorySegment *shm_ptr;      // Pointer to shared memory segment
};

// Monitor initialization and destruction
void initializeMonitor(Monitor *monitor, SharedMemorySegment *shm_ptr);
void destroyMonitor(Monitor *monitor);

// Monitor queue functions
void enterMonitor(Monitor *monitor);
void exitMonitor(Monitor *monitor);
void displayProcessQueue(Monitor *monitor);

// Helper functions
int getAccountMutexIndex(const char *accountId);
double monitorGetBalance(Monitor *monitor, const char *accountId);
void monitorUpdateBalance(Monitor *monitor, const char *accountId, double newBalance);
void monitorRecordTransaction(Monitor *monitor, const char *type, const char *accountId, double amount, const char *status, const char *reason, const char *recipientAccountId = NULL);

// Transaction functions
void createAccount(Monitor *monitor, const char *accountId, const char *name, double initialBalance);
void deposit(Monitor *monitor, const char *accountId, double amount);
void withdraw(Monitor *monitor, const char *accountId, double amount);
void inquiry(Monitor *monitor, const char *accountId);
void transfer(Monitor *monitor, const char *fromAccountId, double amount, const char *toAccountId);
void closeAccount(Monitor *monitor, const char *accountId);

#endif // MONITOR_H
