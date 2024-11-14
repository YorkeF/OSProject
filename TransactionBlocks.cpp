#include "TransactionBlocks.h"
#include "Monitor.h"

// Function to handle account creation
void createAccount(Monitor *monitor, const char *accountId, const char *name, double initialBalance) {
    monitorCreateAccount(monitor, accountId, name, initialBalance);
}

// Function to handle deposits
void deposit(Monitor *monitor, const char *accountId, double amount) {
    monitorDeposit(monitor, accountId, amount);
}

// Function to handle withdrawals
void withdraw(Monitor *monitor, const char *accountId, double amount) {
    monitorWithdraw(monitor, accountId, amount);
}

// Function to handle balance inquiries
void inquiry(Monitor *monitor, const char *accountId) {
    monitorInquiry(monitor, accountId);
}

// Function to handle transfers
void transfer(Monitor *monitor, const char *fromAccountId, double amount, const char *toAccountId) {
    monitorTransfer(monitor, fromAccountId, amount, toAccountId);
}

// Function to handle account closure
void closeAccount(Monitor *monitor, const char *accountId) {
    monitorCloseAccount(monitor, accountId);
}

