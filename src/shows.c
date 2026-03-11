#include "../include/shows.h"
#include "../include/csv_utils.h"

// --- Context Structs for Loading Data ---

typedef struct {
    int target_id;
    Movie *movie;
    int found;
} MovieContext;

typedef struct {
    int target_id;
    Theatre *theatre;
    int found;
} TheatreContext;

typedef struct {
    int target_id;
    Screen *screen;
    int found;
} ScreenContext;

typedef struct {
    int target_id;
    Show *show;
    int found;
} ShowContext;

typedef struct {
    int show_id;
    int count;
} TicketCountContext;

// --- Row Handlers ---

int movie_handler(char **fields, int field_count, void *context) {
    MovieContext *ctx = (MovieContext *)context;
    if (atoi(fields[0]) == ctx->target_id) {
        ctx->movie->id = atoi(fields[0]);
        strcpy(ctx->movie->name, fields[1]);
        ctx->movie->duration = atoi(fields[2]);
        strcpy(ctx->movie->genre_ids, fields[3]);
        ctx->found = 1;
        return 1;
    }
    return 0;
}

int theatre_handler(char **fields, int field_count, void *context) {
    TheatreContext *ctx = (TheatreContext *)context;
    if (atoi(fields[0]) == ctx->target_id) {
        ctx->theatre->id = atoi(fields[0]);
        strcpy(ctx->theatre->name, fields[1]);
        strcpy(ctx->theatre->location, fields[2]);
        ctx->found = 1;
        return 1;
    }
    return 0;
}

int screen_handler(char **fields, int field_count, void *context) {
    ScreenContext *ctx = (ScreenContext *)context;
    if (atoi(fields[0]) == ctx->target_id) {
        ctx->screen->id = atoi(fields[0]);
        ctx->screen->theatre_id = atoi(fields[1]);
        strcpy(ctx->screen->name, fields[2]);
        ctx->screen->total_seats = atoi(fields[3]);
        ctx->found = 1;
        return 1;
    }
    return 0;
}

int show_lookup_handler(char **fields, int field_count, void *context) {
    ShowContext *ctx = (ShowContext *)context;
    if (atoi(fields[0]) == ctx->target_id) {
        ctx->show->id = atoi(fields[0]);
        ctx->show->theatre_id = atoi(fields[1]);
        ctx->show->screen_id = atoi(fields[2]);
        ctx->show->movie_id = atoi(fields[3]);
        strcpy(ctx->show->datetime, fields[4]);
        ctx->show->price = atoi(fields[5]);
        ctx->found = 1;
        return 1;
    }
    return 0;
}

int ticket_count_handler(char **fields, int field_count, void *context) {
    TicketCountContext *ctx = (TicketCountContext *)context;
    // ticket_id, user_id, show_id, seat_number, status
    if (atoi(fields[2]) == ctx->show_id && strcmp(fields[4], "BOOKED") == 0) {
        ctx->count++;
    }
    return 0;
}

// --- Public Functions ---

int get_movie_details(int movie_id, Movie *movie) {
    MovieContext ctx = {movie_id, movie, 0};
    read_csv("data/movies.csv", 1, movie_handler, &ctx);
    return ctx.found ? 0 : -1;
}

int get_theatre_details(int theatre_id, Theatre *theatre) {
    TheatreContext ctx = {theatre_id, theatre, 0};
    read_csv("data/theatres.csv", 1, theatre_handler, &ctx);
    return ctx.found ? 0 : -1;
}

int get_screen_details(int screen_id, Screen *screen) {
    ScreenContext ctx = {screen_id, screen, 0};
    read_csv("data/screens.csv", 1, screen_handler, &ctx);
    return ctx.found ? 0 : -1;
}

int get_show_details(int show_id, Show *show) {
    ShowContext ctx = {show_id, show, 0};
    read_csv("data/shows.csv", 1, show_lookup_handler, &ctx);
    return ctx.found ? 0 : -1;
}

int get_booked_seats(int show_id) {
    TicketCountContext ctx = {show_id, 0};
    read_csv("data/tickets.csv", 1, ticket_count_handler, &ctx);
    return ctx.count;
}

// Handler to display each show row
int display_show_row_handler(char **fields, int field_count, void *context) {
    // show_id, theatre_id, screen_id, movie_id, datetime, price
    int show_id = atoi(fields[0]);
    int theatre_id = atoi(fields[1]);
    int screen_id = atoi(fields[2]);
    int movie_id = atoi(fields[3]);
    char *datetime = fields[4];
    int price = atoi(fields[5]);

    Movie movie;
    Theatre theatre;
    Screen screen;

    if (get_movie_details(movie_id, &movie) != 0) return 0;
    if (get_theatre_details(theatre_id, &theatre) != 0) return 0;
    if (get_screen_details(screen_id, &screen) != 0) return 0;

    int booked = get_booked_seats(show_id);
    int available = screen.total_seats - booked;

    printf("%-5d | %-20s | %-15s | %-10s | %-16s | %-5d | %-5d\n",
           show_id, movie.name, theatre.name, screen.name, datetime, price, available);
    
    return 0;
}

void display_all_shows() {
    printf("\nAvailable Shows:\n");
    printf("%-5s | %-20s | %-15s | %-10s | %-16s | %-5s | %-5s\n",
           "ID", "Movie", "Theatre", "Screen", "Time", "Price", "Avail");
    printf("------------------------------------------------------------------------------------------------\n");
    read_csv("data/shows.csv", 1, display_show_row_handler, NULL);
    printf("------------------------------------------------------------------------------------------------\n");
}
