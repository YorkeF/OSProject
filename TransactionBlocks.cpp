#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "Monitor.h"


void deposit(Monitor *monitor, const char *accountId, double amount);
void withdraw(Monitor *monitor, const char *accountId, double amount);
void inquiry(Monitor *monitor, const char *accountId);
void transfer(Monitor *monitor, const char *fromAccountId, double amount, const char *toAccountId);
void closeAccount(Monitor *monitor, const char *accountId);
void createAccount(Monitor *monitor, const char *accountId);

void createAccount(Monitor *monitor, const char *accountId, double initialBalance)
{
    monitorCreateAccount(monitor, accountId, initialBalance);
}

void closeAccount(Monitor *monitor, const char *accountId)
{
    monitorCloseAccount(monitor, accountId);
}

void inquiry(Monitor *monitor, const char *accountId)
{
    monitorInquiry(monitor, accountId);
}

// Function to handle deposits
void deposit(Monitor *monitor, const char *accountId, double amount) {
   monitorDeposit(monitor, accountId, amount);
}



// Function to handle withdrawals
void withdraw(Monitor *monitor, const char *accountId, double amount) {
    monitorWithdraw(monitor, accountId, amount);
}


// Function to transfer funds between accounts
void transfer(Monitor *monitor, const char *fromAccountId, double amount, const char *toAccountId) {
    monitorTransfer(monitor, fromAccountId, amount, toAccountId);
}


