#include "TransactionBlocks.cpp"
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <algorithm>
#include <cstring> 

using namespace std;

// Forward declarations of functions from TransactionBlocks.cpp
void deposit(const char *filename, double amount);
void withdraw(const char *filename, double amount);
void transfer(const char *fromAccount, double amount, const char *toAccount);
void inquiry(const char *filename);
double getBalance(const char *accountName);

class Monitor 
{
    public:
        Monitor() {}  // Constructor
        ~Monitor() {} // Destructor

        void withdraw(const char *accountNumber, double amount);
        void deposit(const char *accountNumber, double amount);
        void transfer(const char *fromAccount, const char *toAccount, double amount);
        double balanceInquiry(const char *accountNumber);

    private:
        void acquireLock(const char *accountNumber);
        void releaseLock(const char *accountNumber);
};

void Monitor::acquireLock(const char *accountNumber) 
{
    string semName = "/sem_account_" + string(accountNumber);
    sem_t *sem = sem_open(semName.c_str(), O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) 
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_wait(sem);
    sem_close(sem);
}

void Monitor::releaseLock(const char *accountNumber) 
{
    string semName = "/sem_account_" + string(accountNumber);
    sem_t *sem = sem_open(semName.c_str(), 0);
    if (sem == SEM_FAILED) 
    {
        perror("sem_open");
        exit(EXIT_FAILURE);
    }
    sem_post(sem);
    sem_close(sem);
}

void Monitor::withdraw(const char *accountNumber, double amount) 
{
    acquireLock(accountNumber);
    ::withdraw(accountNumber, amount);
    releaseLock(accountNumber);
}

void Monitor::deposit(const char *accountNumber, double amount) 
{
    acquireLock(accountNumber);
    ::deposit(accountNumber, amount);
    releaseLock(accountNumber);
}

void Monitor::transfer(const char *fromAccount, const char *toAccount, double amount) 
{
    // Determine the order to acquire locks to prevent deadlocks
    const char *firstAccount;
    const char *secondAccount;

    if (strcmp(fromAccount, toAccount) < 0) 
    {
        firstAccount = fromAccount;
        secondAccount = toAccount;
    } 
    else 
    {
        firstAccount = toAccount;
        secondAccount = fromAccount;
    }

    // Acquire locks in a consistent order
    acquireLock(firstAccount);
    acquireLock(secondAccount);

    ::transfer(fromAccount, amount, toAccount);

    releaseLock(secondAccount);
    releaseLock(firstAccount);
}

double Monitor::balanceInquiry(const char *accountNumber) 
{
    acquireLock(accountNumber);
    double balance = getBalance(accountNumber); // Use getBalance to retrieve the balance
    releaseLock(accountNumber);
    return balance;
}
