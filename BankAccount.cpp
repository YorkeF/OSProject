#include "BankAccount.h"

using namespace std;

BankAccount::BankAccount(int startBalance) : balance(startBalance){}

// Withdraw function that takes parameters of amount wishing to withdraw and account number
void BankAccount::withdraw(int amount, int accountNumber)
{
    {
        // Get the queue lock and lock it
        lock_guard<mutex> queueLock(queueLock);

        // Push the account number of the account trying to access the account into the queue
        entryQueue.push(accountNumber);
    }

    // Lock access to the account while withdraw happens
    unique_lock<mutex> accountLock(accountLock);

    // Activate a wait on the process until it is the front of the queue
    balanceCV.wait(accountLock, [this, accountNumber]
    {
        // Lock access to queue while checking order
        lock_guard<mutex> queueLock(queueLock);
        return (entryQueue.front() == accountNumber);
    });

    // Take the amount out of the account
    balance -= amount;

    {
        // Lock access to queue while removing account
        lock_guard<mutex> queueLock(queueLock);

        // Remove the account from the queue
        entryQueue.pop();
    }

    // Notify all of the waiting processes it recheck the conditions
    balanceCV.notify_all();

    // Unlock the lock
    accountLock.unlock();
}

// Deposit function that takes parameters of amount wishing to deposit and account number
void BankAccount::deposit(int amount, int accountNumber)
{
    {
        // Get the queue lock and lock it
        lock_guard<mutex> queueLock(queueLock);

        // Push the account number of the account trying to access the account into the queue
        entryQueue.push(accountNumber);
    }

    // Lock access to the account while deposit happens
    unique_lock<mutex> accountLock(accountLock);

    // Activate a wait on the process until it is the front of the queue
    balanceCV.wait(accountLock, [this, accountNumber]
    {
        // Lock access to queue while checking order
        lock_guard<mutex> queueLock(queueLock);
        return (entryQueue.front() == accountNumber);
    });

    // Add amount to the account balance
    balance += amount;

    {
        // Lock access to queue while removing account
        lock_guard<mutex> queueLock(queueLock);

        // Remove the account from the queue
        entryQueue.pop();
    }

    // Notify all of the waiting processes it recheck the conditions
    balanceCV.notify_all();

    // Unlock the lock
    accountLock.unlock();
}
