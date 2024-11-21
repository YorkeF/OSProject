// monitor_transactions.cpp

#include "monitor.h"
#include <iostream>
#include <unistd.h>
#include <string.h>

using namespace std;

// Transaction functions
void createAccount(Monitor *monitor, const char *accountId, const char *name, double initialBalance) {
    enterMonitor(monitor);

    int accountIndex = getAccountMutexIndex(accountId);
    pthread_mutex_lock(&(monitor->account_mutexes[accountIndex]));

    // Check if account already exists
    double existingBalance = monitorGetBalance(monitor, accountId);

    if (existingBalance >= 0) {
        // Account already exists
        printf("Error: Account %s already exists.\n", accountId);
        monitorRecordTransaction(monitor, "CREATE", accountId, initialBalance, "FAILED", "Account already exists", NULL);
        pthread_mutex_unlock(&(monitor->account_mutexes[accountIndex]));
        exitMonitor(monitor);
        return;
    }

    // Create account file
    char filename[30];
    snprintf(filename, sizeof(filename), "%s.txt", accountId);

    int fd = open(filename, O_WRONLY | O_CREAT, 0666);

    if (fd == -1) {
        printf("Error creating account file: %s\n", filename);
        monitorRecordTransaction(monitor, "CREATE", accountId, initialBalance, "FAILED", "File creation error", NULL);
        pthread_mutex_unlock(&(monitor->account_mutexes[accountIndex]));
        exitMonitor(monitor);
        return;
    }

    // Write initial balance
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "%.2lf", initialBalance);
    write(fd, buffer, strlen(buffer));
    close(fd);

    printf("User %s created with account ID %s and initial balance %.2lf.\n", name, accountId, initialBalance);

    // Record success in shared memory
    monitorRecordTransaction(monitor, "CREATE", accountId, initialBalance, "SUCCESS", "N/A", NULL);

    pthread_mutex_unlock(&(monitor->account_mutexes[accountIndex]));
    exitMonitor(monitor);
}

void deposit(Monitor *monitor, const char *accountId, double amount) {
    enterMonitor(monitor);

    int accountIndex = getAccountMutexIndex(accountId);
    pthread_mutex_lock(&(monitor->account_mutexes[accountIndex]));

    double balance = monitorGetBalance(monitor, accountId);

    if (balance < 0) {
        // Account does not exist
        printf("Error: Account %s not found.\n", accountId);
        monitorRecordTransaction(monitor, "DEPOSIT", accountId, amount, "FAILED", "Account not found", NULL);
    } else {
        balance += amount;
        monitorUpdateBalance(monitor, accountId, balance);
        printf("Deposit successful. New balance: %.2lf\n", balance);
        monitorRecordTransaction(monitor, "DEPOSIT", accountId, amount, "SUCCESS", "N/A", NULL);
    }

    pthread_mutex_unlock(&(monitor->account_mutexes[accountIndex]));
    exitMonitor(monitor);
}

void withdraw(Monitor *monitor, const char *accountId, double amount) {
    enterMonitor(monitor);

    int accountIndex = getAccountMutexIndex(accountId);
    pthread_mutex_lock(&(monitor->account_mutexes[accountIndex]));

    double balance = monitorGetBalance(monitor, accountId);

    if (balance < 0) {
        // Account does not exist
        printf("Error: Account %s not found.\n", accountId);
        monitorRecordTransaction(monitor, "WITHDRAW", accountId, amount, "FAILED", "Account not found", NULL);
    } else if (amount > balance) {
        // Insufficient funds
        printf("Insufficient funds in account %s. Current balance: %.2lf\n", accountId, balance);
        monitorRecordTransaction(monitor, "WITHDRAW", accountId, amount, "FAILED", "Insufficient funds", NULL);
    } else {
        balance -= amount;
        monitorUpdateBalance(monitor, accountId, balance);
        printf("Withdrawal successful. New balance: %.2lf\n", balance);
        monitorRecordTransaction(monitor, "WITHDRAW", accountId, amount, "SUCCESS", "N/A", NULL);
    }

    pthread_mutex_unlock(&(monitor->account_mutexes[accountIndex]));
    exitMonitor(monitor);
}

void inquiry(Monitor *monitor, const char *accountId) {
    enterMonitor(monitor);

    int accountIndex = getAccountMutexIndex(accountId);
    pthread_mutex_lock(&(monitor->account_mutexes[accountIndex]));

    double balance = monitorGetBalance(monitor, accountId);

    if (balance < 0) {
        // Account does not exist
        printf("Error: Account %s not found.\n", accountId);
        monitorRecordTransaction(monitor, "INQUIRY", accountId, 0.0, "FAILED", "Account not found", NULL);
    } else {
        printf("Account %s balance: %.2lf\n", accountId, balance);
        monitorRecordTransaction(monitor, "INQUIRY", accountId, 0.0, "SUCCESS", "N/A", NULL);
    }

    pthread_mutex_unlock(&(monitor->account_mutexes[accountIndex]));
    exitMonitor(monitor);
}

void transfer(Monitor *monitor, const char *fromAccountId, double amount, const char *toAccountId) {
    enterMonitor(monitor);

    int fromIndex = getAccountMutexIndex(fromAccountId);
    int toIndex = getAccountMutexIndex(toAccountId);

    // Ensure locks are always acquired in the same order
    if (fromIndex < toIndex) {
        pthread_mutex_lock(&(monitor->account_mutexes[fromIndex]));
        pthread_mutex_lock(&(monitor->account_mutexes[toIndex]));
    } else if (fromIndex > toIndex) {
        pthread_mutex_lock(&(monitor->account_mutexes[toIndex]));
        pthread_mutex_lock(&(monitor->account_mutexes[fromIndex]));
    } else {
        // Same account
        pthread_mutex_lock(&(monitor->account_mutexes[fromIndex]));
    }

    // Perform transfer operation
    double fromBalance = monitorGetBalance(monitor, fromAccountId);
    double toBalance = monitorGetBalance(monitor, toAccountId);

    if (fromBalance < 0) {
        printf("Error: From account %s not found.\n", fromAccountId);
        monitorRecordTransaction(monitor, "TRANSFER", fromAccountId, amount, "FAILED", "From account not found", toAccountId);
    } else if (toBalance < 0) {
        printf("Error: To account %s not found.\n", toAccountId);
        monitorRecordTransaction(monitor, "TRANSFER", fromAccountId, amount, "FAILED", "To account not found", toAccountId);
    } else if (amount > fromBalance) {
        printf("Insufficient funds in account %s to transfer %.2lf\n", fromAccountId, amount);
        monitorRecordTransaction(monitor, "TRANSFER", fromAccountId, amount, "FAILED", "Insufficient funds", toAccountId);
    } else {
        fromBalance -= amount;
        toBalance += amount;

        monitorUpdateBalance(monitor, fromAccountId, fromBalance);
        monitorUpdateBalance(monitor, toAccountId, toBalance);

        printf("Transfer successful. %.2lf transferred from %s to %s\n", amount, fromAccountId, toAccountId);
        printf("New balance for %s: %.2lf\n", fromAccountId, fromBalance);
        printf("New balance for %s: %.2lf\n", toAccountId, toBalance);

        // Record success in shared memory
        monitorRecordTransaction(monitor, "TRANSFER", fromAccountId, amount, "SUCCESS", "N/A", toAccountId);
    }

    // Release locks in reverse order
    pthread_mutex_unlock(&(monitor->account_mutexes[fromIndex]));
    if (fromIndex != toIndex) {
        pthread_mutex_unlock(&(monitor->account_mutexes[toIndex]));
    }

    exitMonitor(monitor);
}

void closeAccount(Monitor *monitor, const char *accountId) {
    enterMonitor(monitor);

    int accountIndex = getAccountMutexIndex(accountId);
    pthread_mutex_lock(&(monitor->account_mutexes[accountIndex]));

    double balance = monitorGetBalance(monitor, accountId);

    if (balance < 0) {
        // Account does not exist
        printf("Error: Account %s not found.\n", accountId);
        monitorRecordTransaction(monitor, "CLOSE", accountId, 0.0, "FAILED", "Account not found", NULL);
    } else if (balance != 0.0) {
        // Account balance is not zero
        printf("Cannot close account %s. Balance is not zero: %.2lf\n", accountId, balance);
        monitorRecordTransaction(monitor, "CLOSE", accountId, 0.0, "FAILED", "Balance not zero", NULL);
    } else {
        // Delete the account file
        char filename[30];
        snprintf(filename, sizeof(filename), "%s.txt", accountId);

        if (remove(filename) == 0) {
            printf("Account %s closed successfully.\n", accountId);
            monitorRecordTransaction(monitor, "CLOSE", accountId, 0.0, "SUCCESS", "N/A", NULL);
        } else {
            printf("Error closing account %s.\n", accountId);
            monitorRecordTransaction(monitor, "CLOSE", accountId, 0.0, "FAILED", "Error deleting file", NULL);
        }
    }

    pthread_mutex_unlock(&(monitor->account_mutexes[accountIndex]));
    exitMonitor(monitor);
}
