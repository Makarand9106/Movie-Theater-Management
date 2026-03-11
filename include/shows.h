#ifndef SHOWS_H
#define SHOWS_H

#include "common.h"

typedef struct {
    int id;
    char name[MAX_FIELD_LENGTH];
    int duration;
    char genre_ids[MAX_FIELD_LENGTH];
} Movie;

typedef struct {
    int id;
    char name[MAX_FIELD_LENGTH];
    char location[MAX_FIELD_LENGTH];
} Theatre;

typedef struct {
    int id;
    int theatre_id;
    char name[MAX_FIELD_LENGTH];
    int total_seats;
} Screen;

typedef struct {
    int id;
    int theatre_id;
    int screen_id;
    int movie_id;
    char datetime[MAX_FIELD_LENGTH];
    int price;
} Show;

// Displays all available shows with details
void display_all_shows();

// Gets a show by ID. Returns 0 on success, -1 if not found.
int get_show_details(int show_id, Show *show);

// Helper to get screen details
int get_screen_details(int screen_id, Screen *screen);

// Helper to get movie details
int get_movie_details(int movie_id, Movie *movie);

// Helper to get theatre details
int get_theatre_details(int theatre_id, Theatre *theatre);

#endif
