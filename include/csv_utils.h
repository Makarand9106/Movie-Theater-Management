#ifndef CSV_UTILS_H
#define CSV_UTILS_H

#include "common.h"

// Callback function type for processing each row
// Returns 0 on success, non-zero to stop processing
typedef int (*RowHandler)(char **fields, int field_count, void *context);

// Reads a CSV file and calls the handler for each row
// skip_header: 1 to skip the first line, 0 otherwise
int read_csv(const char *filename, int skip_header, RowHandler handler, void *context);

// Appends a new row to the CSV file
// fields: array of strings
// field_count: number of fields
int append_csv_row(const char *filename, char **fields, int field_count);

// Updates a specific row in the CSV file based on a unique ID (assumed to be in the first column)
// This is a simple implementation that rewrites the file.
// id: the ID to match in the first column
// new_fields: the new data for the row
int update_csv_row(const char *filename, const char *id, char **new_fields, int field_count);

// Deletes a row with the given ID (assumed first column)
int delete_csv_row(const char *filename, const char *id);

// Helper to get the next available ID (max ID + 1) from the first column
int get_next_id(const char *filename);

#endif
