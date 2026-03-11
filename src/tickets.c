#include "../include/tickets.h"
#include "../include/shows.h"
#include "../include/csv_utils.h"
#include "../include/waitlist.h"
#include <stdlib.h>

typedef struct {
    int user_id;
    int found_any;
} ViewTicketsContext;

// Updated to show payment_id
int view_tickets_handler(char **fields, int field_count, void *context) {
    ViewTicketsContext *ctx = (ViewTicketsContext *)context;
    // ticket_id, user_id, show_id, seat_number, status, payment_id
    if (atoi(fields[1]) == ctx->user_id) {
        ctx->found_any = 1;
        int ticket_id = atoi(fields[0]);
        int show_id = atoi(fields[2]);
        char *seat = fields[3];
        char *status = fields[4];
        int payment_id = (field_count >= 6) ? atoi(fields[5]) : 0;

        Show show;
        if (get_show_details(show_id, &show) == 0) {
            Movie movie;
            Theatre theatre;
            get_movie_details(show.movie_id, &movie);
            get_theatre_details(show.theatre_id, &theatre);

            printf("Ticket ID: %d | Payment ID: %d | Movie: %s | Theatre: %s | Time: %s | Seat: %s | Status: %s\n",
                   ticket_id, payment_id, movie.name, theatre.name, show.datetime, seat, status);
        }
    }
    return 0;
}

void view_my_tickets(int user_id) {
    printf("\nMy Tickets:\n");
    printf("------------------------------------------------------------------------------------------------\n");
    ViewTicketsContext ctx = {user_id, 0};
    read_csv("data/tickets.csv", 1, view_tickets_handler, &ctx);
    if (!ctx.found_any) {
        printf("No tickets found.\n");
    }
    printf("------------------------------------------------------------------------------------------------\n");
}

// Helper: Cancel tickets and process waitlist
void cancel_tickets_and_process_waitlist(int *ticket_ids, int count, int user_id) {
    if (count == 0) return;
    
    // Track show_id and seats freed
    int show_id = -1;
    int seats_freed = 0;
    char freed_seats[20][10];  // Store seat numbers
    
    // Cancel all tickets
    FILE *file = fopen("data/tickets.csv", "r");
    FILE *temp = fopen("data/tickets.tmp", "w");
    char line[MAX_LINE_LENGTH];
    int row_count = 0;

    while (fgets(line, sizeof(line), file)) {
        if (row_count == 0) {
            fputs(line, temp);
            row_count++;
            continue;
        }
        
        char line_copy[MAX_LINE_LENGTH];
        strcpy(line_copy, line);
        trim_newline(line_copy);
        
        char *fields[MAX_FIELDS];
        int fc = 0;
        char *token = strtok(line_copy, ",");
        while (token) {
            fields[fc++] = token;
            token = strtok(NULL, ",");
        }

        int ticket_id = atoi(fields[0]);
        int should_cancel = 0;
        
        for (int i = 0; i < count; i++) {
            if (ticket_id == ticket_ids[i]) {
                should_cancel = 1;
                break;
            }
        }

        if (should_cancel && fc >= 5) {
            // Cancel this ticket
            show_id = atoi(fields[2]);
            strcpy(freed_seats[seats_freed++], fields[3]);
            fprintf(temp, "%s,%s,%s,%s,CANCELLED,%s\n", fields[0], fields[1], fields[2], fields[3], 
                    (fc >= 6) ? fields[5] : "0");
        } else {
            fputs(line, temp);
        }
        row_count++;
    }
    fclose(file);
    fclose(temp);
    remove("data/tickets.csv");
    rename("data/tickets.tmp", "data/tickets.csv");

    printf("%d ticket(s) cancelled successfully.\n", seats_freed);

    // Process waitlist with freed seats
    if (show_id != -1 && seats_freed > 0) {
        int matched_ticket_count = 0;
        int promoted_user_id = process_waitlist(show_id, seats_freed, &matched_ticket_count);
        
        if (promoted_user_id != -1 && matched_ticket_count > 0) {
            printf("Assigning %d seat(s) to user %d from waitlist...\n", matched_ticket_count, promoted_user_id);
            
            // Create tickets for promoted user
            Show show;
            get_show_details(show_id, &show);
            
            // Generate payment for waitlist user
            int payment_id = get_next_id("data/payment.csv");
            char payment_id_str[20], user_id_str[20], amount_str[20];
            sprintf(payment_id_str, "%d", payment_id);
            sprintf(user_id_str, "%d", promoted_user_id);
            sprintf(amount_str, "%d", show.price * matched_ticket_count);
            
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            char time_str[64];
            sprintf(time_str, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

            char *payment_fields[] = {payment_id_str, user_id_str, "-", amount_str, time_str};
            append_csv_row("data/payment.csv", payment_fields, 5);
            
            // Assign seats
            for (int i = 0; i < matched_ticket_count && i < seats_freed; i++) {
                int new_ticket_id = get_next_id("data/tickets.csv");
                char tid_str[20], sid_str[20];
                sprintf(tid_str, "%d", new_ticket_id);
                sprintf(sid_str, "%d", show_id);
                
                char *ticket_fields[] = {tid_str, user_id_str, sid_str, freed_seats[i], "BOOKED", payment_id_str};
                append_csv_row("data/tickets.csv", ticket_fields, 6);
                
                printf("Seat %s assigned to user %d (Ticket ID: %d)\n", freed_seats[i], promoted_user_id, new_ticket_id);
            }
        }
    }
}

// Cancel by payment ID
void cancel_by_payment_id(int user_id) {
    printf("\nYour Booked Tickets (grouped by Payment ID):\n");
    printf("------------------------------------------------------------------------------------------------\n");
    ViewTicketsContext view_ctx = {user_id, 0};
    read_csv("data/tickets.csv", 1, view_tickets_handler, &view_ctx);
    if (!view_ctx.found_any) {
        printf("No tickets found.\n");
        printf("------------------------------------------------------------------------------------------------\n");
        return;
    }
    printf("------------------------------------------------------------------------------------------------\n");

    int payment_id;
    printf("\nEnter Payment ID to cancel entire booking (Enter 0 to go back): ");
    if (scanf("%d", &payment_id) != 1) {
        while(getchar() != '\n');
        return;
    }
    while(getchar() != '\n');

    if (payment_id == 0) {
        return;
    }

    // Find all tickets with this payment_id
    int ticket_ids[50];
    int ticket_count = 0;
    
    FILE *file = fopen("data/tickets.csv", "r");
    char line[MAX_LINE_LENGTH];
    int row = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (row++ == 0) continue;
        
        char line_copy[MAX_LINE_LENGTH];
        strcpy(line_copy, line);
        trim_newline(line_copy);
        
        char *fields[MAX_FIELDS];
        int fc = 0;
        char *token = strtok(line_copy, ",");
        while (token) {
            fields[fc++] = token;
            token = strtok(NULL, ",");
        }
        
        if (fc >= 6 && atoi(fields[1]) == user_id && atoi(fields[5]) == payment_id && strcmp(fields[4], "BOOKED") == 0) {
            ticket_ids[ticket_count++] = atoi(fields[0]);
        }
    }
    fclose(file);

    if (ticket_count == 0) {
        printf("No booked tickets found with Payment ID %d.\n", payment_id);
        return;
    }

    printf("Found %d ticket(s) with Payment ID %d. Cancel all? (y/n): ", ticket_count, payment_id);
    char confirm;
    scanf("%c", &confirm);
    while(getchar() != '\n');

    if (confirm != 'y' && confirm != 'Y') {
        printf("Cancellation aborted.\n");
        return;
    }

    cancel_tickets_and_process_waitlist(ticket_ids, ticket_count, user_id);
}

// Cancel by ticket IDs
void cancel_by_ticket_ids(int user_id) {
    printf("\nYour Booked Tickets:\n");
    printf("------------------------------------------------------------------------------------------------\n");
    ViewTicketsContext view_ctx = {user_id, 0};
    read_csv("data/tickets.csv", 1, view_tickets_handler, &view_ctx);
    if (!view_ctx.found_any) {
        printf("No tickets found.\n");
        printf("------------------------------------------------------------------------------------------------\n");
        return;
    }
    printf("------------------------------------------------------------------------------------------------\n");

    char input[256];
    printf("\nEnter Ticket IDs to cancel (comma-separated, e.g., 1,2,3) or 0 to go back: ");
    if (fgets(input, sizeof(input), stdin) == NULL) return;
    trim_newline(input);

    if (strcmp(input, "0") == 0) {
        return;
    }

    // Parse ticket IDs
    int ticket_ids[50];
    int ticket_count = 0;
    char *token = strtok(input, ",");
    while (token && ticket_count < 50) {
        // Trim whitespace
        while(*token == ' ') token++;
        ticket_ids[ticket_count++] = atoi(token);
        token = strtok(NULL, ",");
    }

    if (ticket_count == 0) {
        printf("No ticket IDs entered.\n");
        return;
    }

    printf("Cancel %d ticket(s)? (y/n): ", ticket_count);
    char confirm;
    scanf("%c", &confirm);
    while(getchar() != '\n');

    if (confirm != 'y' && confirm != 'Y') {
        printf("Cancellation aborted.\n");
        return;
    }

    cancel_tickets_and_process_waitlist(ticket_ids, ticket_count, user_id);
}

// Main cancel ticket function with options
void cancel_ticket(int user_id) {
    printf("\n=== Cancel Tickets ===\n");
    printf("1. Cancel entire booking (by Payment ID)\n");
    printf("2. Cancel specific tickets (by Ticket IDs)\n");
    printf("3. Go back\n");
    printf("Enter choice: ");
    
    int choice;
    if (scanf("%d", &choice) != 1) {
        while(getchar() != '\n');
        return;
    }
    while(getchar() != '\n');

    switch (choice) {
        case 1:
            cancel_by_payment_id(user_id);
            break;
        case 2:
            cancel_by_ticket_ids(user_id);
            break;
        case 3:
            return;
        default:
            printf("Invalid choice.\n");
    }
}
