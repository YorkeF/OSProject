// driver.cpp

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <pthread.h>
#include "monitor.h"
#include "sharedmemory.h"
#include "monitor_init_and_queue.cpp"    // Include the monitor initialization and queue functions
#include "monitor_helpers.cpp"           // Include the monitor helper functions
#include "monitor_transactions.cpp"      // Include the transaction functions

using namespace std;

// String to uppercase function
string toUpperCase(const string &str) {
    string result = str;
    transform(result.begin(), result.end(), result.begin(),
              [](unsigned char c) { return toupper(c); });
    return result;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <input_file>" << endl;
        return 1;
    }

    // Open the input file
    ifstream inputFile(argv[1]);
    if (!inputFile.is_open()) {
        cerr << "Error: Could not open input file " << argv[1] << endl;
        return 1;
    }

    // Generate a key for shared memory
    key_t shm_key = ftok(".", 'x');
    int shm_id = shmget(shm_key, sizeof(SharedMemorySegment), IPC_CREAT | 0666);
    if (shm_id < 0) {
        perror("Parent shmget");
        return 1;
    }

    // Attach to shared memory
    SharedMemorySegment *shm_ptr = (SharedMemorySegment *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (SharedMemorySegment *)-1) {
        perror("Parent shmat");
        return 1;
    }

    // Initialize shared memory
    shm_ptr->transaction_count = 0;

    // Initialize shared memory mutex
    pthread_mutexattr_t shmMutexAttr;
    pthread_mutexattr_init(&shmMutexAttr);
    pthread_mutexattr_setpshared(&shmMutexAttr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(shm_ptr->mutex), &shmMutexAttr);

    // Initialize Monitor
    Monitor monitor;
    initializeMonitor(&monitor, shm_ptr);

    string line;
    while (getline(inputFile, line)) {
        stringstream ss(line);
        string accountId, command, recipientId;
        double amount;

        ss >> accountId >> command;
        command = toUpperCase(command); // Normalize the command

        pid_t pid = fork();
        if (pid == 0) { // Child process
            if (command == "WITHDRAW") {
                ss >> amount;
                withdraw(&monitor, accountId.c_str(), amount);
            } else if (command == "CREATE") {
                ss >> amount;
                createAccount(&monitor, accountId.c_str(), ("User_" + accountId).c_str(), amount);
            } else if (command == "INQUIRY") {
                inquiry(&monitor, accountId.c_str());
            } else if (command == "DEPOSIT") {
                ss >> amount;
                deposit(&monitor, accountId.c_str(), amount);
            } else if (command == "TRANSFER") {
                ss >> amount >> recipientId;
                transfer(&monitor, accountId.c_str(), amount, recipientId.c_str());
            } else if (command == "CLOSE") {
                closeAccount(&monitor, accountId.c_str());
            } else {
                cerr << "Unknown command: " << command << endl;
            }
            exit(0);
        } else if (pid > 0) { // Parent process
            wait(NULL); // Wait for child process to finish
        } else {
            perror("Fork failed");
        }
    }

    // Display transactions from shared memory
    cout << "\nTransactions recorded in shared memory:\n";
    for (int i = 0; i < shm_ptr->transaction_count; i++) {
        TransactionRecord &record = shm_ptr->records[i];
        cout << "Transaction Type: " << record.transaction_type
             << ", Account ID: " << record.account_id
             << ", Amount: " << record.amount
             << ", Status: " << record.status
             << ", Reason: " << record.reason
             << ", Timestamp: " << record.timestamp << endl;
    }

    // Destroy Monitor
    destroyMonitor(&monitor);

    // Cleanup shared memory
    shmdt(shm_ptr);
    shmctl(shm_id, IPC_RMID, NULL);

    inputFile.close();
    return 0;
}
