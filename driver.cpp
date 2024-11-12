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
#include "Monitor.h"

using namespace std;

// Structure to represent a User
struct User {
    int userId;
    string name;
    double balance;
};

void create(const char *accountId, const string& name, double initialBalance, SharedMemorySegment *shm_ptr) {
    // Check if account already exists
    double existingBalance = getBalance(accountId, shm_ptr);
    if (existingBalance >= 0) {
        // Account already exists
        printf("Error: Account %s already exists.\n", accountId);
        recordTransaction("CREATE", accountId, initialBalance, "FAILED", "Account already exists", shm_ptr);
        return;
    }

    // Create account file
    char filename[30];
    snprintf(filename, sizeof(filename), "%s.txt", accountId);

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        printf("Error creating account file: %s\n", filename);
        recordTransaction("CREATE", accountId, initialBalance, "FAILED", "File creation error", shm_ptr);
        return;
    }

    fprintf(file, "%.2lf", initialBalance);
    fclose(file);

    printf("User %s created with account ID %s and initial balance %.2lf.\n", name.c_str(), accountId, initialBalance);

    // Record success in shared memory
    recordTransaction("CREATE", accountId, initialBalance, "SUCCESS", "N/A", shm_ptr);
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

    // Initialize Monitor
    Monitor monitor;
    initializeMonitor(&monitor, shm_ptr);

    // Create accounts
    createAccount(&monitor, "1", "Alice", 1000.0);
    createAccount(&monitor, "2", "Bob", 500.0);

    // Perform transactions
    deposit(&monitor, "1", 200.0);
    withdraw(&monitor, "1", 150.0);
    inquiry(&monitor, "1");
    transfer(&monitor, "1", 300.0, "2");
    inquiry(&monitor, "2");
    closeAccount(&monitor, "1"); // Should fail if balance is not zero
    withdraw(&monitor, "1", 750.0); // Withdraw remaining balance
    closeAccount(&monitor, "1"); // Should succeed now



    // Read and display the transaction records
    cout << "\n--------------------------\nTransaction Records:\n";
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

    // Destroy Monitor
    destroyMonitor(&monitor); 
    
    // Cleanup
    shmdt(shm_ptr);
    shmctl(shm_id, IPC_RMID, NULL);

    return 0;
}
