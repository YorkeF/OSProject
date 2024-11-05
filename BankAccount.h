#ifndef BANKACCOUNT_H
#define BANKACCOUNT_H

#include <iostream>
#include <mutex>
#include <condition_variable>
#include <queue>

using namespace std;

class BankAccount 
{
    private:
        int balance;
        mutex accountLock;
        mutex queueLock;
        condition_variable balanceCV;
        queue<int> entryQueue;

    public:
        BankAccount(int startBalance);

        void withdraw(int amount, int accountNumber);
        void deposit(int amount, int accountNumber);
        void transfer(int amount, int senderAccountNumber, int recieverAccountNumber);
        void balanceInquiry(int accountNumber);
}

#endif
