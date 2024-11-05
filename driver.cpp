#include <iostream>
#include <fstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

// Structure to represent a User
struct User {
    int userId;
    string name;
    double balance;
};

// Function to create a new user and save to a file
void create(int userId, const string& name, double initialBalance) {
    pid_t pid = fork();

    if (pid == 0) {
        // Filename based on the user ID
        string filename = to_string(userId) + ".txt";

        // Open account file
        ofstream accountFile(filename);
        if (!accountFile) {
            cerr << "Error creating file for user ID " << userId << ".\n";
            exit(1);
        }

        // Setting initial balance
        accountFile << initialBalance;
        accountFile.close();

        cout << "User " << name << " created with ID " << userId << " and file " << filename << ".\n";
        exit(0);
    } else if (pid > 0) {
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
    
    // Tests to check if it works
    /*
    create(1, "Alice", 1000.0);
    create(2, "Bob", 500.0);
    inquiry(1);
    inquiry(2);
    */
    return 0;
}
