/**
* Group I
 * Yorke Ferrell
 * yorke.ferrell@okstate.edu
 * 11/20/2024
 */

#ifndef SHAREDMEMORY_H
#define SHAREDMEMORY_H

#include <pthread.h>
#include <time.h>

#define MAX_TRANSACTIONS 100
#define ACCOUNT_ID_LENGTH 20
#define STATUS_LENGTH 10
#define TRANSACTION_TYPE_LENGTH 10
#define REASON_LENGTH 100

struct TransactionRecord {
    char transaction_type[TRANSACTION_TYPE_LENGTH]; // CREATE, DEPOSIT, etc.
    char account_id[ACCOUNT_ID_LENGTH];
    char recipient_account_id[ACCOUNT_ID_LENGTH]; // For TRANSFER transactions
    double amount; // Transaction amount
    char status[STATUS_LENGTH]; // SUCCESS or FAILED
    char reason[REASON_LENGTH]; // Reason for failure, if any
    char timestamp[30]; // Date and time of the transaction
};

struct SharedMemorySegment {
    TransactionRecord records[MAX_TRANSACTIONS];
    int transaction_count;
    pthread_mutex_t mutex; // Mutex for synchronization
};

#endif // SHAREDMEMORY_H
