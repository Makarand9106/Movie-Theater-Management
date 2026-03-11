#ifndef WAITLIST_H
#define WAITLIST_H

#include "common.h"

// Adds a user to the waitlist for a specific show (prompts for ticket count)
int add_to_waitlist(int user_id, int show_id);

int process_waitlist(int show_id, int available_seats, int *matched_ticket_count);

// View user's waitlist entries
void view_my_waitlists(int user_id);

// Leave a waitlist
void leave_waitlist(int user_id);

#endif
