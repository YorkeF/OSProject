#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Function to read the balance from an account file
double getBalance(const char *accountName) {
    char filename[30];
    snprintf(filename, sizeof(filename), "%s.txt", accountName);

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
void updateBalance(const char *accountName, double newBalance) {
    char filename[30];
    snprintf(filename, sizeof(filename), "%s.txt", accountName);

    FILE *file = fopen(filename, "w");
    
    if (file == NULL) {         //if file does not exist
        printf("Error updating the file.\n");
        return;
    }
    
    fprintf(file, "%.2lf", newBalance);
    fclose(file);
}

// Function to handle deposits
void deposit(const char *filename, double amount) {
    double balance = getBalance(filename);
    
    if (balance < 0) return;
    
    balance += amount;
    updateBalance(filename, balance);
    printf("Deposit successful. New balance: %.2lf\n", balance);
}

// Function to handle withdrawals
void withdraw(const char *filename, double amount) {
    double balance = getBalance(filename);
    
    if (balance < 0) return;    //user should not be able to withdraw from a negative account balance
    
    if (amount > balance) {
        printf("Insufficient funds. Current balance: %.2lf\n", balance);
    } else {
        balance -= amount;
        updateBalance(filename, balance);
        printf("Withdrawal successful. New balance: %.2lf\n", balance);
    }
}

// Function to inquire about the balance
void inquiry(const char *filename) {
    double balance = getBalance(filename);
    printf("Account %s balance: %.2lf\n", filename, balance);
}

// Function to transfer funds between accounts
void transfer(const char *fromAccount, double amount, const char *toAccount) {
    double fromBalance = getBalance(fromAccount);
    double toBalance = getBalance(toAccount);

    if (fromBalance < 0 || toBalance < 0) return;   //if the transferrring account is negative quit the transfer

    //REMOVE THESE IT IS ONLY FOR DEBUGGING
    printf("Debug: Original balance for %s: %.2lf\n", fromAccount, fromBalance);
    printf("Debug: Original balance for %s: %.2lf\n", toAccount, toBalance);

    if (amount > fromBalance) {
        printf("Insufficient funds in account %s to transfer %.2lf\n", fromAccount, amount);
    } else {
        fromBalance -= amount;
        toBalance += amount;

        updateBalance(fromAccount, fromBalance);
        updateBalance(toAccount, toBalance);

        printf("Transfer successful. %.2lf transferred from %s to %s\n", amount, fromAccount, toAccount);
        printf("New balance for %s: %.2lf\n", fromAccount, fromBalance); // Print new balance for fromAccount
        printf("New balance for %s: %.2lf\n", toAccount, toBalance);
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
int main() {
    char command[50];
    char account1[20], account2[20];
    double amount;

    while (1) {
        printf("Enter command (Exit to quit): ");
        fgets(command, sizeof(command), stdin);
        trimWhitespace(command); // Trim whitespace

        // Check for exit command
        if (strncmp(command, "Exit", 4) == 0) {
            printf("Exiting program.\n");
            break;
        }

        // Tokenize the command using spaces
        char *token = strtok(command, " ");
        if (token == NULL) {
            printf("Invalid command.\n");
            continue;
        }

        // First token is the account name
        strcpy(account1, token);

        // Get the second token for the command type
        token = strtok(NULL, " ");
        if (token == NULL) {
            printf("Invalid command.\n");
            continue;
        }

        // Check command type
        if (strcmp(token, "Withdraw") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                amount = atof(token);
                printf("withdrawal function\n");
                withdraw(account1, amount);
            }
        } else if (strcmp(token, "Deposit") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                amount = atof(token);
                printf("deposit function\n");
                deposit(account1, amount);
            }
        } else if (strcmp(token, "Inquiry") == 0) {
            printf("inquiry function\n");
            inquiry(account1);
        } else if (strcmp(token, "Transfer") == 0) {
            token = strtok(NULL, " ");
            if (token != NULL) {
                amount = atof(token);
                token = strtok(NULL, " ");
                if (token != NULL) {
                    strcpy(account2, token);
                    printf("I am going to run the transfer function from: %s, amount: %.2lf, to: %s\n", account1, amount, account2);
                    transfer(account1, amount, account2);
                }
            }
        } else {
            printf("Invalid command.\n");
        }
    }

    return 0;
}