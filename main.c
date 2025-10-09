#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>
typedef struct {
    char username[64];
    char password[64];
    char account_id[8];     // 7 digits + null terminator
    char Pin[7];            // 6 digits + null terminator
    double balance;
    int withdrawals_today;  // count of withdrawals today
    int failed_attempts;    // count of consecutive failed login attempts
    bool frozen;
} Account;


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

// validate 7-digit account id and check uniqueness in accounts array
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

// find account index by username; returns -1 if not found or invalid input
static int find_account_by_username(const Account accounts[], int count, const char *username) {
    if (!username) return -1;
    for (int i = 0; i < count; ++i) {
        if (accounts[i].account_id[0] == '\0') continue; /* skip empty slot */
        if (strcmp(accounts[i].username, username) == 0) return i;
    }
    return -1;
}

// find account index by account_id; returns -1 if not found
static int find_account_by_id(const Account accounts[], int count, const char *id) {
    if (!id) return -1;
    for (int i = 0; i < count; ++i) {
        if (accounts[i].account_id[0] == '\0') continue; /* skip empty slot */
        if (strcmp(accounts[i].account_id, id) == 0) return i;
    }
    return -1;
}

/* deposit amount into account identified by account_id
   returns:
     0 = success
    -1 = not found
    -2 = invalid amount (<= 0)
*/
static int deposit(Account accounts[], int count, const char *account_id, double amount) {
    if (amount <= 0.0) return -2;
    int idx = find_account_by_id(accounts, count, account_id);
    if (idx < 0) return -1;
    accounts[idx].balance += amount;
    return 0;
}
// interactive deposit prompt (PIN required)
static void deposit_prompt(Account accounts[], int count) {
    char accid[16];
    char pin_in[16];
    char buf[64];

    printf("\n--- Deposit ---\n");
    printf("Enter 7-digit account ID: ");
    if (!fgets(accid, sizeof(accid), stdin)) { printf("Input error.\n"); return; }
    trim_newline(accid);

    int idx = find_account_by_id(accounts, count, accid);
    if (idx < 0) {
        printf("Account ID not found.\n");
        return;
    }

// verify PIN
    printf("Enter your 6-digit PIN: ");
    if (!fgets(pin_in, sizeof(pin_in), stdin)) { printf("Input error.\n"); return; }
    trim_newline(pin_in);
    if (strlen(pin_in) != 6 || strcmp(accounts[idx].Pin, pin_in) != 0) {
        printf("Incorrect PIN. Deposit aborted.\n");
        return;
    }

    // proceed with deposit
    printf("Enter deposit amount (> 0): ");
    if (!fgets(buf, sizeof(buf), stdin)) { printf("Input error.\n"); return; }
    trim_newline(buf);
    char *endptr;
    double amt = strtod(buf, &endptr);
    if (endptr == buf || amt <= 0.0) {
        printf("Invalid amount.\n");
        return;
    }

    int res = deposit(accounts, count, accid, amt);
    if (res == 0) {
        printf("Deposit successful. New balance: %.2f\n", accounts[idx].balance);
    } else {
        printf("Deposit failed (code %d).\n", res);
    }
}

/* withdraw amount from account identified by account_id and verified by PIN

   returns:
     0 = success
    -1 = account not found
    -2 = invalid amount (<= 0)
    -3 = insufficient funds
    -4 = incorrect PIN
    -5 = daily withdrawal limit reached
    -6 = amount exceeds per-withdrawal limit (500)
*/
static int withdraw(Account accounts[], int count, const char *account_id, const char *pin, double amount) {
    if (amount <= 0.0) return -2;
    if (amount > 500.0) return -6; /* enforce per-withdrawal cap */
    int idx = find_account_by_id(accounts, count, account_id);
    if (idx < 0) return -1;
    if (!pin || strcmp(accounts[idx].Pin, pin) != 0) return -4;
    if (accounts[idx].withdrawals_today >= 3) return -5; /* daily limit */
    if (accounts[idx].balance < amount) return -3;
    accounts[idx].balance -= amount;
    accounts[idx].withdrawals_today += 1;
    return 0;
}

/* interactive withdraw prompt (PIN required) */
static void withdraw_prompt(Account accounts[], int count) {
    char accid[16];
    char pin_in[16];
    char buf[64];

    printf("\n--- Withdraw ---\n");
    printf("Enter 7-digit account ID: ");
    if (!fgets(accid, sizeof(accid), stdin)) { printf("Input error.\n"); return; }
    trim_newline(accid);

    int idx = find_account_by_id(accounts, count, accid);
    if (idx < 0) {
        printf("Account ID not found.\n");
        return;
    }

    printf("Enter your 6-digit PIN: ");
    if (!fgets(pin_in, sizeof(pin_in), stdin)) { printf("Input error.\n"); return; }
    trim_newline(pin_in);
    if (strlen(pin_in) != 6 || strcmp(accounts[idx].Pin, pin_in) != 0) {
        printf("Incorrect PIN. Withdrawal aborted.\n");
        return;
    }

    printf("Enter withdrawal amount (> 0, max 500): ");
    if (!fgets(buf, sizeof(buf), stdin)) { printf("Input error.\n"); return; }
    trim_newline(buf);
    char *endptr;
    double amt = strtod(buf, &endptr);
    if (endptr == buf || amt <= 0.0) {
        printf("Invalid amount.\n");
        return;
    }

    int res = withdraw(accounts, count, accid, pin_in, amt);
    if (res == 0) {
        printf("Withdrawal successful. New balance: %.2f\n", accounts[idx].balance);
    } else if (res == -3) {
        printf("Withdrawal failed: insufficient funds. Current balance: %.2f\n", accounts[idx].balance);
    } else if (res == -5) {
        printf("Withdrawal failed: daily withdrawal limit (3) reached for this account.\n");
    } else if (res == -6) {
        printf("Withdrawal failed: amount exceeds per-withdrawal limit of 500.\n");
    } else {
        printf("Withdrawal failed (code %d).\n", res);
    }
}

// interactive login prompt; returns index of logged-in account or -1 on failure
static int login_prompt(Account accounts[], int count) {
    char accid[16];
    char pwd[64];

    while (true) {
        printf("\nLogin...\n");
        printf("Account ID: ");
        if (!fgets(accid, sizeof(accid), stdin)) return -1;
        trim_newline(accid);

        if (!is_valid_account_id(accid)) {
            printf("Invalid account ID format. It must be 7 digits.\n");
            continue; /* ask for account id again */
        }

        int idx = find_account_by_id(accounts, count, accid);
        if (idx < 0) {
            printf("No such account ID. Try again.\n");
            continue; /* ask for account id again */
        }

        if (accounts[idx].frozen) {
            printf("This account (%s) is frozen due to multiple failed login attempts.\n", accid);
            return -1;
        }

        // Now prompt for password up to 3 times for this account
        for (int attempt = 1; attempt <= 3; ++attempt) {
            printf("Password (attempt %d of 3): ", attempt);
            if (!fgets(pwd, sizeof(pwd), stdin)) return -1;
            trim_newline(pwd);

            if (strcmp(accounts[idx].password, pwd) == 0) {
                // successful login: reset failed attempts and return index
                accounts[idx].failed_attempts = 0;
                return idx;
            } else {
                accounts[idx].failed_attempts++;
                int remaining = 3 - accounts[idx].failed_attempts;
                if (accounts[idx].failed_attempts >= 3) {
                    accounts[idx].frozen = true;
                    printf("Incorrect password. Account %s has been frozen after 3 failed attempts.\n", accid);
                    return -1;
                } else {
                    printf("Incorrect password. %d attempt(s) remaining for this account.\n", remaining);
                }
            }
        }

        return -1;
    }

    return -1;
}

#define MAX_ACCOUNTS 10


static int create_account_prompt(Account accounts[], int *account_count) {
    if (!account_count) return -1;
    if (*account_count >= MAX_ACCOUNTS) {
        printf("No more accounts can be created.\n");
        return -1;
    }

    char username[64];
    char password[64];
    char confirm[64];

    // username
    while (true) {
        printf("Please create your account name (3-10 chars, letters/digits/_ only): ");
        if (!fgets(username, sizeof(username), stdin)) return -1;
        trim_newline(username);
        if (!is_valid_username(username)) {
            printf("Error: invalid username. Length 3-10 and only letters/digits/_ allowed.\n");
            continue;
        }
        break;
    }

    // password
    while (true) {
        printf("Please create your PASSWORD (6-11 chars, include upper, lower, digit): ");
        if (!fgets(password, sizeof(password), stdin)) return -1;
        trim_newline(password);
        if (!is_valid_password(password)) {
            printf("Error: invalid password. Must be 6-11 chars with upper/lower/digit.\n");
            continue;
        }
        printf("Please confirm your PASSWORD: ");
        if (!fgets(confirm, sizeof(confirm), stdin)) return -1;
        trim_newline(confirm);
        if (strcmp(password, confirm) != 0) {
            printf("Error: passwords do not match. Try again.\n");
            continue;
        }
        break;
    }

    // generate unique 7-digit account id
    char accid[16];
    int attempts = 0;
    do {
        int idnum = (rand() % 9000000) + 1000000; /* produce 7-digit number */
        snprintf(accid, sizeof(accid), "%07d", idnum);
        attempts++;
        if (attempts > 100000) {
            printf("Failed to generate unique account ID.\n");
            return -1;
        }
    } while (account_id_exists(accounts, *account_count, accid));

    printf("Assigned Account ID: %s\n", accid);

    // set 6-digit PIN
    char pin[16], pin_confirm[16];
    while (true) {
        printf("Set a 6-digit PIN (digits only): ");
        if (!fgets(pin, sizeof(pin), stdin)) return -1;
        trim_newline(pin);
        size_t pinlen = strlen(pin);
        bool ok = (pinlen == 6);
        for (size_t i = 0; ok && i < pinlen; ++i) if (!isdigit((unsigned char)pin[i])) ok = false;
        if (!ok) { printf("Invalid PIN. It must be exactly 6 digits.\n"); continue; }

        printf("Confirm PIN: ");
        if (!fgets(pin_confirm, sizeof(pin_confirm), stdin)) return -1;
        trim_newline(pin_confirm);
        if (strcmp(pin, pin_confirm) != 0) { printf("PINs do not match. Try again.\n"); continue; }
        break;
    }

    // create and store account
    Account a = {0};
    strncpy(a.username, username, sizeof(a.username)-1);
    strncpy(a.password, password, sizeof(a.password)-1);
    strncpy(a.Pin, pin, sizeof(a.Pin)-1);
    strncpy(a.account_id, accid, sizeof(a.account_id)-1);
    a.balance = 0.0;
    a.withdrawals_today = 0;
    a.failed_attempts = 0;
    a.frozen = false;

    accounts[*account_count] = a;
    int idx = (*account_count)++;
    printf("Account created successfully! Username: %s  Account ID: %s\n", a.username, a.account_id);
    return idx;
}

int main() {
    Account accounts[MAX_ACCOUNTS];
    for (int i = 0; i < MAX_ACCOUNTS; ++i) {
        accounts[i].account_id[0] = '\0';
        accounts[i].withdrawals_today = 0;
        accounts[i].failed_attempts = 0;
        accounts[i].frozen = false;
    }
    int account_count = 0;

    srand((unsigned)time(NULL));
    printf("---------- welcome to Community Bank Simulator ----------\n");

    while (true) {
        printf("\n----- Account Menu -----\n");
        printf("1) Create account\n");
        printf("2) Login\n");
        printf("3) Simulate new day (reset withdrawals counters)\n");
        printf("4) Exit\n");
        printf("Choose an option: ");

        char choice_buf[16];
        if (!fgets(choice_buf, sizeof(choice_buf), stdin)) break;
        trim_newline(choice_buf);
        int choice = atoi(choice_buf);

        if (choice == 1) {
            create_account_prompt(accounts, &account_count);
        } else if (choice == 2) {
            if (account_count == 0) {
                printf("No accounts exist. Please create an account first.\n");
                continue;
            }
            int logged = login_prompt(accounts, account_count);
            if (logged < 0) {
                printf("Login failed.\n");
                continue;
            }

            printf("\nLogin successful. Welcome, %s!\n", accounts[logged].username);
            
            while (true) {
                printf("\n----- Account Menu -----\n");
                printf("Username: %s\n", accounts[logged].username);
                printf("Account ID: %s\n", accounts[logged].account_id);
                printf("\n1) Withdraw\n");
                printf("2) Deposit\n");
                printf("3) Check balance\n");
                printf("4) Logout\n");
                printf("5) Exit program\n");
                printf("Choose an option: ");
                if (!fgets(choice_buf, sizeof(choice_buf), stdin)) { printf("Input error.\n"); break; }
                trim_newline(choice_buf);
                int sub = atoi(choice_buf);

                if (sub == 1) {
                   
                    char pin_buf[16], amt_buf[64];
                    printf("Enter your 6-digit PIN: ");
                    if (!fgets(pin_buf, sizeof(pin_buf), stdin)) { printf("Input error.\n"); continue; }
                    trim_newline(pin_buf);
                    if (strlen(pin_buf) != 6 || strcmp(accounts[logged].Pin, pin_buf) != 0) {
                        printf("Incorrect PIN. Withdrawal cancelled.\n"); continue;
                    }
                    printf("Enter withdrawal amount (> 0, max 500): ");
                    if (!fgets(amt_buf, sizeof(amt_buf), stdin)) { printf("Input error.\n"); continue; }
                    trim_newline(amt_buf);
                    char *endptr; double amt = strtod(amt_buf, &endptr);
                    if (endptr == amt_buf || amt <= 0.0) { printf("Invalid amount.\n"); continue; }
                    int r = withdraw(accounts, account_count, accounts[logged].account_id, pin_buf, amt);
                    if (r == 0) printf("Withdrawal successful. New balance: %.2f\n", accounts[logged].balance);
                    else if (r == -3) printf("Insufficient funds. Balance: %.2f\n", accounts[logged].balance);
                    else if (r == -5) printf("Daily withdrawal limit reached (3). Try next day.\n");
                    else if (r == -6) printf("Amount exceeds per-withdrawal limit (500).\n");
                    else printf("Withdrawal failed (code %d).\n", r);
                } else if (sub == 2) {
                   
                    char pin_buf[16], amt_buf[64];
                    printf("Enter your 6-digit PIN: ");
                    if (!fgets(pin_buf, sizeof(pin_buf), stdin)) { printf("Input error.\n"); continue; }
                    trim_newline(pin_buf);
                    if (strlen(pin_buf) != 6 || strcmp(accounts[logged].Pin, pin_buf) != 0) {
                        printf("Incorrect PIN. Deposit cancelled.\n"); continue;
                    }
                    printf("Enter deposit amount (> 0): ");
                    if (!fgets(amt_buf, sizeof(amt_buf), stdin)) { printf("Input error.\n"); continue; }
                    trim_newline(amt_buf);
                    char *endptr; double amt = strtod(amt_buf, &endptr);
                    if (endptr == amt_buf || amt <= 0.0) { printf("Invalid amount.\n"); continue; }
                    int r = deposit(accounts, account_count, accounts[logged].account_id, amt);
                    if (r == 0) printf("Deposit successful. New balance: %.2f\n", accounts[logged].balance);
                    else printf("Deposit failed (code %d).\n", r);
                } else if (sub == 3) {
                    printf("Current balance: %.2f\n", accounts[logged].balance);
                    printf("Withdrawals today: %d/3\n", accounts[logged].withdrawals_today);
                } else if (sub == 4) {
                    printf("Logging out...\n");
                    break;
                } else if (sub == 5) {
                    printf("Goodbye.\n");
                    return 0;
                } else {
                    printf("Invalid choice.\n");
                }
            }
        } else if (choice == 3) {
           
            for (int i = 0; i < account_count; ++i) accounts[i].withdrawals_today = 0;
            printf("New day simulated: withdrawal counters reset for all accounts.\n");
        } else if (choice == 4) {
            printf("Goodbye.\n");
            break;
        } else {
            printf("Invalid option.\n");
        }
    }

    return 0;
}