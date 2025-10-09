#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

//clean the input buffer
void clearBuffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
// Trim newline/CR characters from end of string
static void trim_newline(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r')) {
        s[--n] = '\0';
    }
}
// Validate username: length 3-10, only letters, digits, underscores
static bool is_valid_username(const char *u) {
    size_t len = strlen(u);
    if (len < 3 || len > 10) return false;
    for (size_t i = 0; i < len; ++i) {
        if (!(isalnum((unsigned char)u[i]) || u[i] == '_')) return false;
    }
    return true;
}
// Validate password: length 6-11, at least one upper, one lower, one digit
static bool is_valid_password(const char *p) {
    size_t len = strlen(p);
    if (len < 6 || len > 11) return false;
    bool has_upper = false, has_lower = false, has_digit = false;
    for (size_t i = 0; i < len; ++i) {
        unsigned char ch = (unsigned char)p[i];
        if (isupper(ch)) has_upper = true;
        else if (islower(ch)) has_lower = true;
        else if (isdigit(ch)) has_digit = true;
        /* allow other printable chars if desired */
    }
    // enforce at least one upper, one lower, one digit 
    return has_upper && has_lower && has_digit;
}

typedef struct {
    char username[64];
    char password[64];
    char account_id[8];    // 7 digits + null terminator
    char withdraw_pin[7];  // 6 digits + null terminator
    double balance;
} Account;

/* --- new helpers: validate 7-digit account id and check uniqueness --- */
static bool is_valid_account_id(const char *id) {
    if (!id) return false;
    if (strlen(id) != 7) return false;
    for (size_t i = 0; i < 7; ++i) {
        if (!isdigit((unsigned char)id[i])) return false;
    }
    return true;
}

static bool account_id_exists(const Account accounts[], int count, const char *id) {
    if (!id) return false;
    for (int i = 0; i < count; ++i) {
        if (accounts[i].account_id[0] == '\0') continue; /* skip unused slots */
        if (strcmp(accounts[i].account_id, id) == 0) return true;
    }
    return false;
}

/* find account index by username; returns -1 if not found */
static int find_account_by_username(const Account accounts[], int count, const char *username) {
    if (!username) return -1;
    for (int i = 0; i < count; ++i) {
        if (accounts[i].account_id[0] == '\0') continue; /* skip empty slot */
        if (strcmp(accounts[i].username, username) == 0) return i;
    }
    return -1;
}

/* simple login prompt: username + password, up to 3 attempts
   returns account index on success or -1 on failure */
static int login_prompt(const Account accounts[], int count) {
    char uname[64];
    char pwd[64];
    const int max_attempts = 3;

    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        printf("\nLogin (attempt %d of %d)\n", attempt, max_attempts);
        printf("Username: ");
        if (!fgets(uname, sizeof(uname), stdin)) return -1;
        trim_newline(uname);

        int idx = find_account_by_username(accounts, count, uname);
        if (idx < 0) {
            printf("No such username. Try again.\n");
            continue;
        }

        printf("Password: ");
        if (!fgets(pwd, sizeof(pwd), stdin)) return -1;
        trim_newline(pwd);

        if (strcmp(accounts[idx].password, pwd) == 0) {
            return idx; /* success */
        } else {
            printf("Incorrect password.\n");
        }
    }

    return -1; /* failed after attempts */
}

int main() {

    /* use compile-time constant for array size */
    #define MAX_ACCOUNTS 10
    Account accounts[MAX_ACCOUNTS];
    for (int i = 0; i < MAX_ACCOUNTS; ++i) accounts[i].account_id[0] = '\0'; /* mark empty */

    int account_count = 0;

    char username[64];
    char password[64];

    printf("----------welcome to Community Bank Simulator----------\n");
    // username creat and valid
    while (true) {
        printf("Please creat your account name (3-10 chars, letters/digits/_ only): ");
        if (!fgets(username, sizeof(username), stdin)) {
            printf("Input error.\n");
            return 1;
        }
        trim_newline(username);
        if (!is_valid_username(username)) {
            printf("Error: invalid username. Length 3-10 and only letters/digits/_ allowed. (got %zu)\n", strlen(username));
            continue;
        }
        break;
    }

    // Password creation and validation (with confirmation)
    while (true) {
        char confirm[64];

        printf("Please create your PASSWORD (6-11 chars, must include upper, lower, digit): ");
        if (!fgets(password, sizeof(password), stdin)) {
            printf("Input error.\n");
            return 1;
        }
        trim_newline(password);

        if (!is_valid_password(password)) {
            printf("Error: invalid password. Must be 6-11 chars and include at least one uppercase, one lowercase and one digit.\n");
            continue;
        }

        printf("Please confirm your PASSWORD: ");
        if (!fgets(confirm, sizeof(confirm), stdin)) {
            printf("Input error.\n");
            return 1;
        }
        trim_newline(confirm);

        if (strcmp(password, confirm) != 0) {
            printf("Error: passwords do not match. Please try again.\n");
            continue;
        }

        break;
    }

    /* --- new: collect 6-digit withdraw PIN --- */
    while (true) {
        char pin[16], pin_confirm[16];
        printf("Set a 6-digit withdraw PIN (digits only): ");
        if (!fgets(pin, sizeof(pin), stdin)) { printf("Input error.\n"); return 1; }
        trim_newline(pin);

        // validate length and digits
        size_t pinlen = strlen(pin);
        bool ok = (pinlen == 6);
        for (size_t i = 0; ok && i < pinlen; ++i) {
            if (!isdigit((unsigned char)pin[i])) ok = false;
        }
        if (!ok) { printf("Invalid PIN. It must be exactly 6 digits.\n"); continue; }

        printf("Confirm withdraw PIN: ");
        if (!fgets(pin_confirm, sizeof(pin_confirm), stdin)) { printf("Input error.\n"); return 1; }
        trim_newline(pin_confirm);

        if (strcmp(pin, pin_confirm) != 0) {
            printf("Error: PINs do not match. Please try again.\n");
            continue;
        }

        /* --- prompt user for 7-digit account id (must be unique) --- */
        char accid[16];
        while (true) {
            if (account_count >= MAX_ACCOUNTS) {
                printf("No more accounts can be created.\n");
                return 1;
            }
            printf("Enter a 7-digit account ID (digits only): ");
            if (!fgets(accid, sizeof(accid), stdin)) { printf("Input error.\n"); return 1; }
            trim_newline(accid);

            if (!is_valid_account_id(accid)) {
                printf("Invalid account ID. It must be exactly 7 digits.\n");
                continue;
            }
            if (account_id_exists(accounts, account_count, accid)) {
                printf("That account ID is already taken. Choose another.\n");
                continue;
            }
            break;
        }

        /* save pin and create Account instance */
        char saved_pin[7];
        strncpy(saved_pin, pin, sizeof(saved_pin)-1);
        saved_pin[6] = '\0';

        Account new_acc = {0};
        strncpy(new_acc.username, username, sizeof(new_acc.username)-1);
        strncpy(new_acc.password, password, sizeof(new_acc.password)-1);
        strncpy(new_acc.withdraw_pin, saved_pin, sizeof(new_acc.withdraw_pin)-1);
        strncpy(new_acc.account_id, accid, sizeof(new_acc.account_id)-1);
        new_acc.balance = 0.0; /* or initial deposit if you collect one */

        /* store into accounts array */
        accounts[account_count++] = new_acc;

        printf("Withdraw PIN set and account id assigned: %s\n", new_acc.account_id);

        /* --- immediately offer login to demonstrate retrieval --- */
        int logged = login_prompt(accounts, account_count);
        if (logged >= 0) {
            printf("\nLogin successful. Welcome, %s!\n", accounts[logged].username);
            printf("Account ID: %s  Balance: %.2f\n", accounts[logged].account_id, accounts[logged].balance);
        } else {
            printf("\nLogin failed.\n");
        }

        break;
    }

    printf("Your account has benn successfully created!\n");
    return 0;
}