#ifndef LOGIN_H
#define LOGIN_H

#include "common.h"

typedef struct {
    int user_id;
    char username[MAX_FIELD_LENGTH];
    char password[MAX_FIELD_LENGTH];
} User;

// Authenticates a user. Returns user_id on success, -1 on failure.
int login_user(const char *username, const char *password);

// Registers a new user. Returns new user_id on success, -1 on failure.
int register_user(const char *username, const char *password);

#endif
