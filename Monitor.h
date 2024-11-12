#ifndef MONITOR_H
#define MONITOR_H

#include <pthread.h>
#include "SharedMemory.h"

// Monitor structure
typedef struct 
{
    pthread_mutex_t mutex;             // Mutex for synchronization
    SharedMemorySegment *shm_ptr;      // Pointer to shared memory segment
} Monitor;

// Monitor initialization and destruction
void initializeMonitor(Monitor *monitor, SharedMemorySegment *shm_ptr);
void destroyMonitor(Monitor *monitor);

// Monitor operations
void monitorCreateAccount(Monitor *monitor, const char *accountId, const char *name, double initialBalance);
void monitorDeposit(Monitor *monitor, const char *accountId, double amount);
void monitorWithdraw(Monitor *monitor, const char *accountId, double amount);
void monitorInquiry(Monitor *monitor, const char *accountId);
void monitorTransfer(Monitor *monitor, const char *fromAccountId, double amount, const char *toAccountId);
void monitorCloseAccount(Monitor *monitor, const char *accountId);

// Helper functions
double monitorGetBalance(Monitor *monitor, const char *accountId);
void monitorUpdateBalance(Monitor *monitor, const char *accountId, double newBalance);
void monitorRecordTransaction(Monitor *monitor, const char *type, const char *accountId, double amount, const char *status, const char *reason, const char *recipientAccountId);

#endif 
