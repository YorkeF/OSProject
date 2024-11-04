#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

// Structure to represent a User
struct User {
    int userId;
    char name[50];
    double balance;
};

// new user and save to a file
void create(int userId, const char* name, double initialBalance) {
    pid_t pid = fork();

    if (pid == 0) { 
        // filename based on the user ID
        char filename[20];
        snprintf(filename, sizeof(filename), "%d.txt", userId);

        //Open account file
        FILE* accountFile = fopen(filename, "w");
        if (accountFile == NULL) {
            fprintf(stderr, "Error creating file for user ID %d.\n", userId);
            exit(1); 
        }

        // setting initial balance
        fprintf(accountFile, "%.2f", initialBalance);
        fclose(accountFile);

        printf("User %s created with ID %d and file %s.\n", name, userId, filename);
        exit(0);  
    } else if (pid > 0) {
        wait(NULL); 
    } else {
        fprintf(stderr, "Fork failed!\n");
    }
}

// not needed but Inquiry: just used to test code
void Inquiry(int userId) {
    char filename[20];
    snprintf(filename, sizeof(filename), "%d.txt", userId);

    FILE* accountFile = fopen(filename, "r");
    if (accountFile != NULL) {
        double balance;
        fscanf(accountFile, "%lf", &balance);
        printf("User ID: %d, Balance: $%.2f\n", userId, balance);
        fclose(accountFile);
    } else {
        fprintf(stderr, "Error reading file for user ID %d.\n", userId);
    }
}

int main() {
    
   // tests to check if it works
    create(1, "Alice", 1000.0);
    create(2, "Bob", 500.0);
    Inquiry(1);
    Inquiry(2);

    return 0;
}
