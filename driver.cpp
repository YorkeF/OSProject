#include <iostream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <time.h>

#include "TransactionBlocks.cpp" // Include transaction functions
#include "SharedMemory.h"








using namespace std;

// Structure to represent a User
struct User {
    int userId;
    string name;
    double balance;
};

void create(int userId, const string& name, double initialBalance, SharedMemorySegment *shm_ptr, int shm_id) {
    pid_t pid = fork();

    if (pid == 0) {
        // Child process
        // Attach to shared memory
        SharedMemorySegment *child_shm_ptr = (SharedMemorySegment *)shmat(shm_id, NULL, 0);
        if (child_shm_ptr == (SharedMemorySegment *)-1) {
            perror("Child shmat");
            exit(1);
        }

        // Perform account creation
        string filename = to_string(userId) + ".txt";
        ofstream accountFile(filename);
        if (!accountFile) {
            cerr << "Error creating file for user ID " << userId << ".\n";
            exit(1);
        }

        accountFile << initialBalance;
        accountFile.close();

        cout << "User " << name << " created with ID " << userId << " and file " << filename << ".\n";

        // Prepare transaction record
        TransactionRecord record;
        strcpy(record.transaction_type, "CREATE");
        strcpy(record.account_id, to_string(userId).c_str());
        record.amount = initialBalance;
        strcpy(record.status, "SUCCESS");
        strcpy(record.reason, "N/A");

        // Get current timestamp
        time_t now = time(NULL);
        strftime(record.timestamp, sizeof(record.timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

        // Critical Section Start
        pthread_mutex_lock(&(child_shm_ptr->mutex));

        // Write to shared memory
        int idx = child_shm_ptr->transaction_count;
        child_shm_ptr->records[idx] = record;
        child_shm_ptr->transaction_count++;

        pthread_mutex_unlock(&(child_shm_ptr->mutex));
        // Critical Section End

        // Detach and exit child process
        shmdt(child_shm_ptr);
        exit(0);
    } else if (pid > 0) {
        // Parent process waits for child
        wait(NULL);
    } else {
        cerr << "Fork failed!\n";
    }
}


// Function to check account balance (for testing purposes)
/*
void inquiry(int userId) {
    string filename = to_string(userId) + ".txt";

    ifstream accountFile(filename);
    if (accountFile) {
        double balance;
        accountFile >> balance;
        cout << "User ID: " << userId << ", Balance: $" << balance << "\n";
        accountFile.close();
    } else {
        cerr << "Error reading file for user ID " << userId << ".\n";
    }
}
*/

int main() {

    // Generate a key for shared memory
    key_t shm_key = ftok(".", 'x');
    int shm_id = shmget(shm_key, sizeof(SharedMemorySegment), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("Parent shmget");
        exit(1);
    }

    // Attach to shared memory
    SharedMemorySegment *shm_ptr = (SharedMemorySegment *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (SharedMemorySegment *)-1) {
        perror("Parent shmat");
        exit(1);
    }

    // Initialize shared memory
    shm_ptr->transaction_count = 0;

    // Initialize mutex attributes
    pthread_mutexattr_t mutexAttr;
    pthread_mutexattr_init(&mutexAttr);
    pthread_mutexattr_setpshared(&mutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(shm_ptr->mutex), &mutexAttr);


    // Tests to check if it works
    create(1, "Alice", 1000.0, shm_ptr, shm_id);
    deposit("1", 500.0, shm_ptr);
    withdraw("1", 200.0, shm_ptr);



    // Read and display the transaction records
    cout << "Transaction Records:\n";
    for (int i = 0; i < shm_ptr->transaction_count; i++) {
        TransactionRecord rec = shm_ptr->records[i];
        cout << "Transaction Type: " << rec.transaction_type << endl;
        cout << "Account ID: " << rec.account_id << endl;
        if (strcmp(rec.transaction_type, "TRANSFER") == 0) {
            cout << "Recipient Account ID: " << rec.recipient_account_id << endl;
        }
        cout << "Amount: " << rec.amount << endl;
        cout << "Status: " << rec.status << endl;
        cout << "Timestamp: " << rec.timestamp << endl;
        cout << "Reason: " << rec.reason << endl;
        cout << "--------------------------\n";
    }

    // Cleanup
    shmdt(shm_ptr);
    shmctl(shm_id, IPC_RMID, NULL);




    return 0;
}
