// monitor_helpers.cpp

#include "monitor.h"
#include <iostream>
#include <unistd.h>
#include <sys/file.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

using namespace std;

// Helper function to get mutex index for an account
int getAccountMutexIndex(const char *accountId) {
    // Simple hash function to map account ID to mutex index
    int index = 0;
    for (int i = 0; accountId[i] != '\0'; i++) {
        index += accountId[i];
    }
    return index % MAX_ACCOUNTS;
}

// Helper function to get balance
double monitorGetBalance(Monitor *monitor, const char *accountId) {
    char filename[30];
    snprintf(filename, sizeof(filename), "%s.txt", accountId);

    int fd = open(filename, O_RDONLY);

    if (fd == -1) {
        if (errno != ENOENT) // Check if the error is not 'File Not Found'
        {
            printf("Error reading account file: %s\n", filename);
        }
        return -1;
    }

    // Lock the file for reading
    if (flock(fd, LOCK_SH) == -1) {
        printf("Error locking the file for reading.\n");
        close(fd);
        return -1;
    }

    char buffer[50];
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);

    if (bytesRead <= 0) {
        printf("Error reading balance from file.\n");
        flock(fd, LOCK_UN);
        close(fd);
        return -1;
    }
    buffer[bytesRead] = '\0';

    double balance = atof(buffer);

    // Unlock and close the file
    flock(fd, LOCK_UN);
    close(fd);

    return balance;
}

// Helper function to update balance
void monitorUpdateBalance(Monitor *monitor, const char *accountId, double newBalance) {
    char filename[30];
    snprintf(filename, sizeof(filename), "%s.txt", accountId);

    int fd = open(filename, O_WRONLY);

    if (fd == -1) {
        printf("Error updating the file.\n");
        return;
    }

    // Lock the file for writing
    if (flock(fd, LOCK_EX) == -1) {
        printf("Error locking the file for writing.\n");
        close(fd);
        return;
    }

    // Truncate the file and write new balance
    if (ftruncate(fd, 0) == -1) {
        printf("Error truncating the file.\n");
        flock(fd, LOCK_UN);
        close(fd);
        return;
    }

    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%.2lf", newBalance);
    write(fd, buffer, strlen(buffer));

    // Unlock and close the file
    flock(fd, LOCK_UN);
    close(fd);
}

// Helper function to record transactions
void monitorRecordTransaction(Monitor *monitor, const char *type, const char *accountId, double amount, const char *status, const char *reason, const char *recipientAccountId) {
    TransactionRecord record;
    strcpy(record.transaction_type, type);
    strcpy(record.account_id, accountId);

    if (recipientAccountId != NULL) {
        strcpy(record.recipient_account_id, recipientAccountId);
    } else {
        record.recipient_account_id[0] = '\0';
    }

    record.amount = amount;
    strcpy(record.status, status);
    strcpy(record.reason, reason);

    // Get current timestamp
    time_t now = time(NULL);
    strftime(record.timestamp, sizeof(record.timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Critical Section Start
    pthread_mutex_lock(&(monitor->shm_ptr->mutex));

    // Write to shared memory
    int idx = monitor->shm_ptr->transaction_count;

    if (idx < MAX_TRANSACTIONS) {
        monitor->shm_ptr->records[idx] = record;
        monitor->shm_ptr->transaction_count++;
    } else {
        printf("Transaction record limit reached.\n");
    }

    pthread_mutex_unlock(&(monitor->shm_ptr->mutex));
    // Critical Section End
}