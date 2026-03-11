#include "../include/login.h"
#include "../include/csv_utils.h"

typedef struct {
    const char *username;
    const char *password;
    int found_user_id;
} LoginContext;

int login_row_handler(char **fields, int field_count, void *context) {
    if (field_count < 3) return 0; // Skip invalid rows

    LoginContext *ctx = (LoginContext *)context;
    // fields[0] = user_id, fields[1] = username, fields[2] = password
    
    if (strcmp(fields[1], ctx->username) == 0 && strcmp(fields[2], ctx->password) == 0) {
        ctx->found_user_id = atoi(fields[0]);
        return 1; // Stop processing
    }
    return 0;
}

int login_user(const char *username, const char *password) {
    LoginContext ctx = {username, password, -1};
    read_csv("data/users.csv", 1, login_row_handler, &ctx);
    return ctx.found_user_id;
}

int check_username_handler(char **fields, int fc, void *context) {
    if (strcmp(fields[1], ((LoginContext*)context)->username) == 0) {
        ((LoginContext*)context)->found_user_id = 1; // Found
        return 1;
    }
    return 0;
}

int register_user(const char *username, const char *password) {
    // Check if username exists
    LoginContext ctx = {username, "", -1};
    read_csv("data/users.csv", 1, check_username_handler, &ctx);
    
    if (ctx.found_user_id != -1) {
        printf("Username already exists.\n");
        return -1;
    }

    int new_id = get_next_id("data/users.csv");
    char id_str[20];
    sprintf(id_str, "%d", new_id);
    
    // Cast to char* to match signature, though we know append_csv_row doesn't modify them
    char *fields[] = {id_str, (char*)username, (char*)password};
    append_csv_row("data/users.csv", fields, 3);
    
    return new_id;
}
