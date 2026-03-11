#include "../include/booking.h"
#include "../include/shows.h"
#include "../include/csv_utils.h"
#include "../include/waitlist.h"
#include <stdlib.h>

typedef struct {
    int show_id;
    char seat_number[10];
    int is_booked;
} SeatCheckContext;

int seat_check_handler(char **fields, int field_count, void *context) {
    SeatCheckContext *ctx = (SeatCheckContext *)context;
    // ticket_id, user_id, show_id, seat_number, status
    if (atoi(fields[2]) == ctx->show_id && 
        strcmp(fields[3], ctx->seat_number) == 0 && 
        strcmp(fields[4], "BOOKED") == 0) {
        ctx->is_booked = 1;
        return 1;
    }
    return 0;
}

int is_seat_booked(int show_id, const char *seat_number) {
    SeatCheckContext ctx = {show_id, "", 0};
    strcpy(ctx.seat_number, seat_number);
    read_csv("data/tickets.csv", 1, seat_check_handler, &ctx);
    return ctx.is_booked;
}

struct CountCtx { int show_id; int count; };

int count_handler(char **f, int c, void *ctx) {
    struct CountCtx *x = (struct CountCtx*)ctx;
    if (atoi(f[2]) == x->show_id && strcmp(f[4], "BOOKED") == 0) x->count++;
    return 0;
}

// LINKED LIST for booked seats
typedef struct SeatNode {
    char seat_number[10];
    struct SeatNode *next;
} SeatNode;

typedef struct {
    int show_id;
    SeatNode *booked_head;  // LINKED LIST instead of 2D array
    int count;
} SeatMapContext;

// Helper function to add seat to list
void add_seat(SeatMapContext *ctx, const char *seat_number) {
    SeatNode *new_node = (SeatNode*)malloc(sizeof(SeatNode));
    strcpy(new_node->seat_number, seat_number);
    new_node->next = ctx->booked_head;
    ctx->booked_head = new_node;
    ctx->count++;
}

// Helper function to check if seat is in list
int is_seat_in_list(SeatNode *head, const char *seat_number) {
    SeatNode *current = head;
    while (current != NULL) {
        if (strcasecmp(current->seat_number, seat_number) == 0) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

// Helper function to free seat list
void free_seat_list(SeatNode *head) {
    while (head != NULL) {
        SeatNode *temp = head;
        head = head->next;
        free(temp);
    }
}

int seat_map_handler(char **fields, int fc, void *context) {
    SeatMapContext *c = (SeatMapContext*)context;
    if (atoi(fields[2]) == c->show_id && strcmp(fields[4], "BOOKED") == 0) {
        add_seat(c, fields[3]);
    }
    return 0;
}

void display_seat_map(int show_id, int total_seats, int price) {
    SeatMapContext ctx = {show_id, NULL, 0};
    read_csv("data/tickets.csv", 1, seat_map_handler, &ctx);

    int seats_per_row = 10;
    int rows = (total_seats + seats_per_row - 1) / seats_per_row;

    printf("\n        ________________\n");
    printf("       /  SCREEN HERE   \\\n");
    printf("      /__________________\\\n\n");

    for (int r = 0; r < rows; r++) {
        char row_char = 'A' + r;
        printf("%c | ", row_char);
        for (int c = 1; c <= seats_per_row; c++) {
            int seat_idx = r * seats_per_row + c;
            if (seat_idx > total_seats) break;

            char seat_str[10];
            sprintf(seat_str, "%c%d", row_char, c);

            // Check if seat is in linked list
            int is_booked = is_seat_in_list(ctx.booked_head, seat_str);

            if (is_booked) printf("[X] ");
            else printf("[ ] ");
        }
        printf("\n");
    }
    printf("    ");
    for (int c = 1; c <= seats_per_row; c++) printf(" %-3d", c);
    printf("\n\n");
    printf("Price: %d | Legend: [ ] Available, [X] Booked\n", price);
    
    // CLEANUP: Free linked list memory
    free_seat_list(ctx.booked_head);
}

#include <ctype.h>

void to_upper(char *str) {
    for (int i = 0; str[i]; i++) {
        str[i] = toupper(str[i]);
    }
}

void book_ticket(int user_id) {
    display_all_shows();

    int show_id;
    printf("\nEnter Show ID to book (Enter 0 to cancel): ");
    if (scanf("%d", &show_id) != 1) {
        while(getchar() != '\n');
        return;
    }
    while(getchar() != '\n');

    if (show_id == 0) {
        printf("Booking cancelled.\n");
        return;
    }

    Show show;
    if (get_show_details(show_id, &show) != 0) {
        printf("Invalid Show ID.\n");
        return;
    }

    Screen screen;
    if (get_screen_details(show.screen_id, &screen) != 0) {
        printf("Error retrieving screen details.\n");
        return;
    }

    display_seat_map(show_id, screen.total_seats, show.price);

    // Check availability
    struct CountCtx count_ctx = {show_id, 0};
    read_csv("data/tickets.csv", 1, count_handler, &count_ctx);
    int booked_count = count_ctx.count;

    if (booked_count >= screen.total_seats) {
        printf("\nShow is full! (Total: %d, Booked: %d)\n", screen.total_seats, booked_count);
        char choice;
        printf("Do you want to join the waitlist? (y/n) (Enter 0 to cancel): ");
        scanf("%c", &choice);
        while(getchar() != '\n');

        if (choice == '0') {
            printf("Operation cancelled.\n");
            return;
        }

        if (choice == 'y' || choice == 'Y') {
            add_to_waitlist(user_id, show_id);
            printf("Added to waitlist.\n");
        }
        return;
    }

    char input_line[256];
    printf("Enter Seat Numbers (comma separated, e.g. A1,A2) (Enter 0 to cancel): ");
    if (fgets(input_line, sizeof(input_line), stdin) == NULL) return;
    trim_newline(input_line);

    if (strcmp(input_line, "0") == 0) {
        printf("Booking cancelled.\n");
        return;
    }

    // Parse seats
    char *seats[20];
    int seat_count = 0;
    char *token = strtok(input_line, ",");
    while (token && seat_count < 20) {
        // Trim whitespace around token
        while(*token == ' ') token++;
        char *end = token + strlen(token) - 1;
        while(end > token && *end == ' ') *end-- = '\0';
        
        if (strlen(token) > 0) {
            to_upper(token); // Normalize to uppercase
            seats[seat_count++] = token;
        }
        token = strtok(NULL, ",");
    }

    if (seat_count == 0) {
        printf("No seats entered.\n");
        return;
    }

    // Validate all seats first
    for (int i = 0; i < seat_count; i++) {
        // Check if seat is valid (simple check: starts with letter, followed by digits)
        // Also check if already booked
        // Note: is_seat_booked does exact match, but we normalized to uppercase.
        // However, existing data might be lowercase if manually entered or from bug.
        // Ideally we should fix is_seat_booked to be case-insensitive too.
        if (is_seat_booked(show_id, seats[i])) {
            printf("Seat %s is already booked. Booking cancelled.\n", seats[i]);
            return;
        }
    }

    // Calculate Total Price
    int total_price = seat_count * show.price;
    printf("\n--- Booking Summary ---\n");
    // Let's fetch movie name for better UX
    Movie movie;
    get_movie_details(show.movie_id, &movie);
    printf("Movie: %s\n", movie.name);
    printf("Seats: ");
    for(int i=0; i<seat_count; i++) printf("%s ", seats[i]);
    printf("\nTotal Price: %d x %d = %d\n", seat_count, show.price, total_price);
    
    printf("Confirm booking? (y/n) (Enter 0 to cancel): ");
    char confirm;
    scanf("%c", &confirm);
    while(getchar() != '\n');

    if (confirm != 'y' && confirm != 'Y') {
        printf("Booking cancelled.\n");
        return;
    }

    // Proceed to book all
    printf("Processing payment and booking %d tickets...\n", seat_count);
    
    // Generate SINGLE payment ID for this entire booking
    int payment_id = get_next_id("data/payment.csv");
    char payment_id_str[20];
    sprintf(payment_id_str, "%d", payment_id);
    
    // Create payment record
    char user_id_str[20], amount_str[20];
    sprintf(user_id_str, "%d", user_id);
    sprintf(amount_str, "%d", total_price);  // Total for all tickets
    
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char time_str[64];
    sprintf(time_str, "%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    char *payment_fields[] = {payment_id_str, user_id_str, "-", amount_str, time_str};  // "-" for ticket_id (group payment)
    append_csv_row("data/payment.csv", payment_fields, 5);
    
    // Book all tickets with the SAME payment_id
    for (int i = 0; i < seat_count; i++) {
        int ticket_id = get_next_id("data/tickets.csv");
        char ticket_id_str[20], show_id_str[20];
        sprintf(ticket_id_str, "%d", ticket_id);
        sprintf(show_id_str, "%d", show_id);

        // Add payment_id to ticket record
        char *ticket_fields[] = {ticket_id_str, user_id_str, show_id_str, seats[i], "BOOKED", payment_id_str};
        append_csv_row("data/tickets.csv", ticket_fields, 6);
        
        printf("Ticket booked: %s (ID: %d)\n", seats[i], ticket_id);
    }
    printf("All tickets booked successfully! Payment ID: %d\n", payment_id);
}
