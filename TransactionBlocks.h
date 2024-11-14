#ifndef TRANSACTION_BLOCKS_H
#define TRANSACTION_BLOCKS_H

#include "Monitor.h"

// Transaction functions
void createAccount(Monitor *monitor, const char *accountId, const char *name, double initialBalance);
void deposit(Monitor *monitor, const char *accountId, double amount);
void withdraw(Monitor *monitor, const char *accountId, double amount);
void inquiry(Monitor *monitor, const char *accountId);
void transfer(Monitor *monitor, const char *fromAccountId, double amount, const char *toAccountId);
void closeAccount(Monitor *monitor, const char *accountId);

#endif
