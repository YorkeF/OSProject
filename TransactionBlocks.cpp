#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "SharedMemory.h"
#include "Monitor.h"


void deposit(Monitor *monitor, const char *accountId, double amount);
void withdraw(Monitor *monitor, const char *accountId, double amount);
void inquiry(Monitor *monitor, const char *accountId);
void transfer(Monitor *monitor, const char *fromAccountId, double amount, const char *toAccountId);
void closeAccount(Monitor *monitor, const char *accountId);
void createAccount(Monitor *monitor, const char *accountId);



void recordTransaction(const char *type, const char *accountId, double amount, const char *status, const char *reason, SharedMemorySegment *shm_ptr, const char *recipientAccountId = NULL) {
    TransactionRecord record;
    strcpy(record.transaction_type, type);
    strcpy(record.account_id, accountId);
    if (recipientAccountId != NULL) {
        strcpy(record.recipient_account_id, recipientAccountId);
    } else {
        record.recipient_account_id[0] = '\0'; // Empty string
    }
    record.amount = amount;
    strcpy(record.status, status);
    strcpy(record.reason, reason);

    // Get current timestamp
    time_t now = time(NULL);
    strftime(record.timestamp, sizeof(record.timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Critical Section Start
    pthread_mutex_lock(&(shm_ptr->mutex));

    // Write to shared memory
    int idx = shm_ptr->transaction_count;
    shm_ptr->records[idx] = record;
    shm_ptr->transaction_count++;

    pthread_mutex_unlock(&(shm_ptr->mutex));
    // Critical Section End
}



// Function to read the balance from an account file
double getBalance(const char *accountId, SharedMemorySegment *shm_ptr) {
    char filename[30];
    snprintf(filename, sizeof(filename), "%s.txt", accountId);

    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        printf("Error reading account file: %s\n", filename);
        return -1;
    }
    double balance;
    fscanf(file, "%lf", &balance);
    fclose(file);
    return balance;
}

// Function to update the balance in the file
void updateBalance(const char *accountId, double newBalance, SharedMemorySegment *shm_ptr) {
    char filename[30];
    snprintf(filename, sizeof(filename), "%s.txt", accountId);

    FILE *file = fopen(filename, "w");
    
    if (file == NULL) {         //if file does not exist
        printf("Error updating the file.\n");
        return;
    }
    
    fprintf(file, "%.2lf", newBalance);
    fclose(file);
}

void closeAccount(const char *accountId, SharedMemorySegment *shm_ptr) {
    double balance = getBalance(accountId, shm_ptr);

    if (balance < 0) {
        // Account does not exist
        printf("Error: Account %s not found.\n", accountId);
        recordTransaction("CLOSE", accountId, 0.0, "FAILED", "Account not found", shm_ptr);
        return;
    }

    if (balance != 0.0) {
        // Account balance is not zero
        printf("Cannot close account %s. Balance is not zero: %.2lf\n", accountId, balance);
        recordTransaction("CLOSE", accountId, 0.0, "FAILED", "Balance not zero", shm_ptr);
    } else {
        // Delete the account file
        char filename[30];
        snprintf(filename, sizeof(filename), "%s.txt", accountId);
        if (remove(filename) == 0) {
            printf("Account %s closed successfully.\n", accountId);
            recordTransaction("CLOSE", accountId, 0.0, "SUCCESS", "N/A", shm_ptr);
        } else {
            printf("Error closing account %s.\n", accountId);
            recordTransaction("CLOSE", accountId, 0.0, "FAILED", "Error deleting file", shm_ptr);
        }
    }
}


// Function to handle deposits
void deposit(const char *accountId, double amount, SharedMemorySegment *shm_ptr) {
    double balance = getBalance(accountId, shm_ptr);

    if (balance < 0) {
        // Record failure in shared memory
        recordTransaction("DEPOSIT", accountId, amount, "FAILED", "Account not found", shm_ptr);
        return;
    }

    balance += amount;
    updateBalance(accountId, balance, shm_ptr);
    printf("Deposit successful. New balance: %.2lf\n", balance);

    // Record success in shared memory
    recordTransaction("DEPOSIT", accountId, amount, "SUCCESS", "N/A", shm_ptr);
}



// Function to handle withdrawals
void withdraw(const char *accountId, double amount, SharedMemorySegment *shm_ptr) {
    double balance = getBalance(accountId, shm_ptr);

    if (balance < 0) {
        // Account does not exist
        printf("Error: Account %s not found.\n", accountId);
        recordTransaction("WITHDRAW", accountId, amount, "FAILED", "Account not found", shm_ptr);
        return;
    }

    if (amount > balance) {
        // Insufficient funds
        printf("Insufficient funds in account %s. Current balance: %.2lf\n", accountId, balance);
        recordTransaction("WITHDRAW", accountId, amount, "FAILED", "Insufficient funds", shm_ptr);
    } else {
        balance -= amount;
        updateBalance(accountId, balance, shm_ptr);
        printf("Withdrawal successful. New balance: %.2lf\n", balance);

        // Record success in shared memory
        recordTransaction("WITHDRAW", accountId, amount, "SUCCESS", "N/A", shm_ptr);
    }
}


// Function to inquire about the balance
void inquiry(const char *accountId, SharedMemorySegment *shm_ptr) {
    double balance = getBalance(accountId, shm_ptr);

    if (balance < 0) {
        // Account does not exist
        printf("Error: Account %s not found.\n", accountId);
        recordTransaction("INQUIRY", accountId, 0.0, "FAILED", "Account not found", shm_ptr);
        return;
    }

    printf("Account %s balance: %.2lf\n", accountId, balance);

    // Record successful inquiry
    recordTransaction("INQUIRY", accountId, 0.0, "SUCCESS", "N/A", shm_ptr);
}


// Function to transfer funds between accounts
void transfer(const char *fromAccountId, double amount, const char *toAccountId, SharedMemorySegment *shm_ptr) {
    double fromBalance = getBalance(fromAccountId, shm_ptr);
    double toBalance = getBalance(toAccountId, shm_ptr);

    if (fromBalance < 0) {
        printf("Error: From account %s not found.\n", fromAccountId);
        recordTransaction("TRANSFER", fromAccountId, amount, "FAILED", "From account not found", shm_ptr, toAccountId);
        return;
    }

    if (toBalance < 0) {
        printf("Error: To account %s not found.\n", toAccountId);
        recordTransaction("TRANSFER", fromAccountId, amount, "FAILED", "To account not found", shm_ptr, toAccountId);
        return;
    }

    if (amount > fromBalance) {
        printf("Insufficient funds in account %s to transfer %.2lf\n", fromAccountId, amount);
        recordTransaction("TRANSFER", fromAccountId, amount, "FAILED", "Insufficient funds", shm_ptr, toAccountId);
    } else {
        fromBalance -= amount;
        toBalance += amount;

        updateBalance(fromAccountId, fromBalance, shm_ptr);
        updateBalance(toAccountId, toBalance, shm_ptr);

        printf("Transfer successful. %.2lf transferred from %s to %s\n", amount, fromAccountId, toAccountId);
        printf("New balance for %s: %.2lf\n", fromAccountId, fromBalance);
        printf("New balance for %s: %.2lf\n", toAccountId, toBalance);

        // Record success in shared memory
        recordTransaction("TRANSFER", fromAccountId, amount, "SUCCESS", "N/A", shm_ptr, toAccountId);
    }
}


// Function to trim leading and trailing whitespace
void trimWhitespace(char *str) {
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str)) str++;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    // Null terminate string after last non-space character
    *(end + 1) = '\0';
}

// Main function
// int main() {
//     char command[50];
//     char account1[20], account2[20];
//     double amount;
//
//     while (1) {
//         printf("Enter command (Exit to quit): ");
//         fgets(command, sizeof(command), stdin);
//         trimWhitespace(command); // Trim whitespace
//
//         // Check for exit command
//         if (strncmp(command, "Exit", 4) == 0) {
//             printf("Exiting program.\n");
//             break;
//         }
//
//         // Tokenize the command using spaces
//         char *token = strtok(command, " ");
//         if (token == NULL) {
//             printf("Invalid command.\n");
//             continue;
//         }
//
//         // First token is the account name
//         strcpy(account1, token);
//
//         // Get the second token for the command type
//         token = strtok(NULL, " ");
//         if (token == NULL) {
//             printf("Invalid command.\n");
//             continue;
//         }
//
//         // Check command type
//         if (strcmp(token, "Withdraw") == 0) {
//             token = strtok(NULL, " ");
//             if (token != NULL) {
//                 amount = atof(token);
//                 printf("withdrawal function\n");
//                 withdraw(account1, amount);
//             }
//         } else if (strcmp(token, "Deposit") == 0) {
//             token = strtok(NULL, " ");
//             if (token != NULL) {
//                 amount = atof(token);
//                 printf("deposit function\n");
//                 deposit(account1, amount);
//             }
//         } else if (strcmp(token, "Inquiry") == 0) {
//             printf("inquiry function\n");
//             inquiry(account1);
//         } else if (strcmp(token, "Transfer") == 0) {
//             token = strtok(NULL, " ");
//             if (token != NULL) {
//                 amount = atof(token);
//                 token = strtok(NULL, " ");
//                 if (token != NULL) {
//                     strcpy(account2, token);
//                     printf("I am going to run the transfer function from: %s, amount: %.2lf, to: %s\n", account1, amount, account2);
//                     transfer(account1, amount, account2);
//                 }
//             }
//         } else {
//             printf("Invalid command.\n");
//         }
//     }
//
//     return 0;
// }
