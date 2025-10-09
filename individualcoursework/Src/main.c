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
// Trim newline characters from end of string
static void trim_newline(char *s) {
    size_t n = strlen(s);
    if (n && s[n-1] == '\n') s[n-1] = '\0';
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
    /* enforce at least one upper, one lower, one digit (adjust rules as needed) */
    return has_upper && has_lower && has_digit;
}

int main() {

    char username[64];   /* larger buffer to safely read input */
    char password[64];

    printf("----------welcome to Community Bank Simulator----------\n");

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

    /* Password input */
    while (true) {
        printf("Please creat your PASSWORD (6-11 chars, must include upper, lower, digit): ");
        if (!fgets(password, sizeof(password), stdin)) {
            printf("Input error.\n");
            return 1;
        }
        trim_newline(password);
        if (!is_valid_password(password)) {
            printf("Error: invalid password. Must be 8-16 chars and include at least one uppercase, one lowercase and one digit.\n");
            continue;
        }
        break;
    }

    printf("Your account creation successful!\n");
    return 0;
}