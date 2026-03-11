#include "../include/csv_utils.h"

void trim_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
    if (len > 1 && str[len - 2] == '\r') {
        str[len - 2] = '\0';
    }
}

int read_csv(const char *filename, int skip_header, RowHandler handler, void *context) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    int row_count = 0;

    while (fgets(line, sizeof(line), file)) {
        if (skip_header && row_count == 0) {
            row_count++;
            continue;
        }

        trim_newline(line);
        
        char *fields[MAX_FIELDS];
        int field_count = 0;
        char *token = strtok(line, ",");
        
        while (token != NULL && field_count < MAX_FIELDS) {
            fields[field_count++] = token;
            token = strtok(NULL, ",");
        }

        if (handler(fields, field_count, context) != 0) {
            break;
        }
        row_count++;
    }

    fclose(file);
    return 0;
}

int append_csv_row(const char *filename, char **fields, int field_count) {
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("Error opening file for appending");
        return -1;
    }

    for (int i = 0; i < field_count; i++) {
        fprintf(file, "%s", fields[i]);
        if (i < field_count - 1) {
            fprintf(file, ",");
        }
    }
    fprintf(file, "\n");

    fclose(file);
    return 0;
}

int get_next_id(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return 1; // If file doesn't exist, start at 1

    char line[MAX_LINE_LENGTH];
    int max_id = 0;
    int row_count = 0;

    while (fgets(line, sizeof(line), file)) {
        if (row_count == 0) { // Skip header
            row_count++;
            continue;
        }
        
        char *token = strtok(line, ",");
        if (token) {
            int id = atoi(token);
            if (id > max_id) max_id = id;
        }
    }

    fclose(file);
    return max_id + 1;
}

// Simple implementation: Read all, write to temp, rename
int update_csv_row(const char *filename, const char *id, char **new_fields, int field_count) {
    FILE *file = fopen(filename, "r");
    if (!file) return -1;

    char temp_filename[256];
    snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", filename);
    FILE *temp_file = fopen(temp_filename, "w");
    if (!temp_file) {
        fclose(file);
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    char line_copy[MAX_LINE_LENGTH];
    int row_count = 0;
    int found = 0;

    while (fgets(line, sizeof(line), file)) {
        strcpy(line_copy, line); // Keep original line for writing if not modified
        
        if (row_count == 0) {
            fputs(line, temp_file);
            row_count++;
            continue;
        }

        char *token = strtok(line, ","); // Destroys line
        if (token && strcmp(token, id) == 0) {
            // Match found, write new fields
            for (int i = 0; i < field_count; i++) {
                fprintf(temp_file, "%s", new_fields[i]);
                if (i < field_count - 1) {
                    fprintf(temp_file, ",");
                }
            }
            fprintf(temp_file, "\n");
            found = 1;
        } else {
            fputs(line_copy, temp_file);
        }
        row_count++;
    }

    fclose(file);
    fclose(temp_file);

    if (found) {
        remove(filename);
        rename(temp_filename, filename);
        return 0;
    } else {
        remove(temp_filename);
        return -1; // ID not found
    }
}

int delete_csv_row(const char *filename, const char *id) {
    FILE *file = fopen(filename, "r");
    if (!file) return -1;

    char temp_filename[256];
    snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", filename);
    FILE *temp_file = fopen(temp_filename, "w");
    if (!temp_file) {
        fclose(file);
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    char line_copy[MAX_LINE_LENGTH];
    int row_count = 0;
    int found = 0;

    while (fgets(line, sizeof(line), file)) {
        strcpy(line_copy, line);
        
        if (row_count == 0) {
            fputs(line, temp_file);
            row_count++;
            continue;
        }

        char *token = strtok(line, ",");
        if (token && strcmp(token, id) == 0) {
            found = 1;
            // Skip writing this line
        } else {
            fputs(line_copy, temp_file);
        }
        row_count++;
    }

    fclose(file);
    fclose(temp_file);

    if (found) {
        remove(filename);
        rename(temp_filename, filename);
        return 0;
    } else {
        remove(temp_filename);
        return -1;
    }
}
