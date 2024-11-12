#include "Monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

// Initialize the monitor
void initializeMonitor(Monitor *monitor, SharedMemorySegment *shm_ptr) 
{
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    // Set the mutex to be shared between processes
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(monitor->mutex), &mutexAttr);
    monitor->shm_ptr = shm_ptr;
}

// Destroy the monitor
void destroyMonitor(Monitor *monitor) 
{
    pthread_mutex_destroy(&(monitor->mutex));
}

// Helper function to get balance
double monitorGetBalance(Monitor *monitor, const char *accountId) 
{
    char filename[30];
    snprintf(filename, sizeof(filename), "%s.txt", accountId);

    int fd = open(filename, O_RDONLY);
    if (fd == -1) 
    {
        printf("Error reading account file: %s\n", filename);
        return -1;
    }

    // Lock the file for reading
    if (flock(fd, LOCK_SH) == -1) 
    {
        printf("Error locking the file for reading.\n");
        close(fd);
        return -1;
    }

    char buffer[50];
    ssize_t bytesRead = read(fd, buffer, sizeof(buffer) - 1);

    if (bytesRead <= 0) 
    {
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
void monitorUpdateBalance(Monitor *monitor, const char *accountId, double newBalance) 
{
    char filename[30];
    snprintf(filename, sizeof(filename), "%s.txt", accountId);

    int fd = open(filename, O_WRONLY);

    if (fd == -1) {
        printf("Error updating the file.\n");
        return;
    }

    // Lock the file for writing
    if (flock(fd, LOCK_EX) == -1) 
    {
        printf("Error locking the file for writing.\n");
        close(fd);
        return;
    }

    // Truncate the file and write new balance
    if (ftruncate(fd, 0) == -1) 
    {
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
void monitorRecordTransaction(Monitor *monitor, const char *type, const char *accountId, double amount, const char *status, const char *reason, const char *recipientAccountId = NULL) 
{
    TransactionRecord record;
    strcpy(record.transaction_type, type);
    strcpy(record.account_id, accountId);

    if (recipientAccountId != NULL) 
    {
        strcpy(record.recipient_account_id, recipientAccountId);
    } 
    else 
    {
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
    monitor->shm_ptr->records[idx] = record;
    monitor->shm_ptr->transaction_count++;

    pthread_mutex_unlock(&(monitor->shm_ptr->mutex));
    // Critical Section End
}

// Monitor function implementations
void monitorCreateAccount(Monitor *monitor, const char *accountId, const char *name, double initialBalance) 
{
    pthread_mutex_lock(&(monitor->mutex));

    // Check if account already exists
    double existingBalance = monitorGetBalance(monitor, accountId);

    if (existingBalance >= 0) 
    {
        // Account already exists
        printf("Error: Account %s already exists.\n", accountId);
        monitorRecordTransaction(monitor, "CREATE", accountId, initialBalance, "FAILED", "Account already exists");
        pthread_mutex_unlock(&(monitor->mutex));
        return;
    }

    // Create account file
    char filename[30];
    snprintf(filename, sizeof(filename), "%s.txt", accountId);

    int fd = open(filename, O_WRONLY | O_CREAT, 0666);

    if (fd == -1) 
    {
        printf("Error creating account file: %s\n", filename);
        monitorRecordTransaction(monitor, "CREATE", accountId, initialBalance, "FAILED", "File creation error");
        pthread_mutex_unlock(&(monitor->mutex));
        return;
    }

    // Write initial balance
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%.2lf", initialBalance);
    write(fd, buffer, strlen(buffer));
    close(fd);

    printf("User %s created with account ID %s and initial balance %.2lf.\n", name, accountId, initialBalance);

    // Record in shared memory
    monitorRecordTransaction(monitor, "CREATE", accountId, initialBalance, "SUCCESS", "N/A");

    pthread_mutex_unlock(&(monitor->mutex));
}

void monitorDeposit(Monitor *monitor, const char *accountId, double amount) 
{
    pthread_mutex_lock(&(monitor->mutex));

    double balance = monitorGetBalance(monitor, accountId);

    if (balance < 0) 
    {
        // Account does not exist
        printf("Error: Account %s not found.\n", accountId);
        monitorRecordTransaction(monitor, "DEPOSIT", accountId, amount, "FAILED", "Account not found");
    } 
    else 
    {
        balance += amount;
        monitorUpdateBalance(monitor, accountId, balance);
        printf("Deposit successful. New balance: %.2lf\n", balance);
        monitorRecordTransaction(monitor, "DEPOSIT", accountId, amount, "SUCCESS", "N/A");
    }

    pthread_mutex_unlock(&(monitor->mutex));
}

void monitorWithdraw(Monitor *monitor, const char *accountId, double amount) 
{
    pthread_mutex_lock(&(monitor->mutex));

    double balance = monitorGetBalance(monitor, accountId);

    if (balance < 0) 
    {
        // Account does not exist
        printf("Error: Account %s not found.\n", accountId);
        monitorRecordTransaction(monitor, "WITHDRAW", accountId, amount, "FAILED", "Account not found");
    } 
    else if (amount > balance) 
    {
        // Insufficient funds
        printf("Insufficient funds in account %s. Current balance: %.2lf\n", accountId, balance);
        monitorRecordTransaction(monitor, "WITHDRAW", accountId, amount, "FAILED", "Insufficient funds");
    } 
    else 
    {
        balance -= amount;
        monitorUpdateBalance(monitor, accountId, balance);
        printf("Withdrawal successful. New balance: %.2lf\n", balance);
        monitorRecordTransaction(monitor, "WITHDRAW", accountId, amount, "SUCCESS", "N/A");
    }

    pthread_mutex_unlock(&(monitor->mutex));
}

void monitorInquiry(Monitor *monitor, const char *accountId) 
{
    pthread_mutex_lock(&(monitor->mutex));

    double balance = monitorGetBalance(monitor, accountId);

    if (balance < 0) 
    {
        // Account does not exist
        printf("Error: Account %s not found.\n", accountId);
        monitorRecordTransaction(monitor, "INQUIRY", accountId, 0.0, "FAILED", "Account not found");
    } 
    else 
    {
        printf("Account %s balance: %.2lf\n", accountId, balance);
        monitorRecordTransaction(monitor, "INQUIRY", accountId, 0.0, "SUCCESS", "N/A");
    }

    pthread_mutex_unlock(&(monitor->mutex));
}

void monitorTransfer(Monitor *monitor, const char *fromAccountId, double amount, const char *toAccountId) 
{
    pthread_mutex_lock(&(monitor->mutex));

    double fromBalance = monitorGetBalance(monitor, fromAccountId);
    double toBalance = monitorGetBalance(monitor, toAccountId);

    if (fromBalance < 0) 
    {
        printf("Error: From account %s not found.\n", fromAccountId);
        monitorRecordTransaction(monitor, "TRANSFER", fromAccountId, amount, "FAILED", "From account not found", toAccountId);
    } 
    else if (toBalance < 0) 
    {
        printf("Error: To account %s not found.\n", toAccountId);
        monitorRecordTransaction(monitor, "TRANSFER", fromAccountId, amount, "FAILED", "To account not found", toAccountId);
    } 
    else if (amount > fromBalance) 
    {
        printf("Insufficient funds in account %s to transfer %.2lf\n", fromAccountId, amount);
        monitorRecordTransaction(monitor, "TRANSFER", fromAccountId, amount, "FAILED", "Insufficient funds", toAccountId);
    } 
    else 
    {
        fromBalance -= amount;
        toBalance += amount;

        monitorUpdateBalance(monitor, fromAccountId, fromBalance);
        monitorUpdateBalance(monitor, toAccountId, toBalance);

        printf("Transfer successful. %.2lf transferred from %s to %s\n", amount, fromAccountId, toAccountId);
        printf("New balance for %s: %.2lf\n", fromAccountId, fromBalance);
        printf("New balance for %s: %.2lf\n", toAccountId, toBalance);

        // Record in shared memory
        monitorRecordTransaction(monitor, "TRANSFER", fromAccountId, amount, "SUCCESS", "N/A", toAccountId);
    }

    pthread_mutex_unlock(&(monitor->mutex));
}

void monitorCloseAccount(Monitor *monitor, const char *accountId) 
{
    pthread_mutex_lock(&(monitor->mutex));

    double balance = monitorGetBalance(monitor, accountId);

    if (balance < 0) 
    {
        // Account does not exist
        printf("Error: Account %s not found.\n", accountId);
        monitorRecordTransaction(monitor, "CLOSE", accountId, 0.0, "FAILED", "Account not found");
    } 
    else if (balance != 0.0) 
    {
        // Account balance is not zero
        printf("Cannot close account %s. Balance is not zero: %.2lf\n", accountId, balance);
        monitorRecordTransaction(monitor, "CLOSE", accountId, 0.0, "FAILED", "Balance not zero");
    } 
    else 
    {
        // Delete the account file
        char filename[30];
        snprintf(filename, sizeof(filename), "%s.txt", accountId);

        if (remove(filename) == 0) 
        {
            printf("Account %s closed successfully.\n", accountId);
            monitorRecordTransaction(monitor, "CLOSE", accountId, 0.0, "SUCCESS", "N/A");
        } 
        else 
        {
            printf("Error closing account %s.\n", accountId);
            monitorRecordTransaction(monitor, "CLOSE", accountId, 0.0, "FAILED", "Error deleting file");
        }
    }

    pthread_mutex_unlock(&(monitor->mutex));
}

