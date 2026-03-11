#include "../include/common.h"
#include "../include/login.h"
#include "../include/shows.h"
#include "../include/booking.h"
#include "../include/tickets.h"
#include "../include/bfs.h"
#include "../include/waitlist.h"

void user_menu(int user_id) {
    int choice;
    while (1) {
        printf("\n=== Theatre Booking System ===\n");
        printf("1. View Shows\n");
        printf("2. Book Ticket\n");
        printf("3. View My Tickets\n");
        printf("4. Cancel Ticket\n");
        printf("5. View My Waitlists\n");
        printf("6. Leave Waitlist\n");
        printf("7. Get Recommendations\n");
        printf("8. View Recommendation Graph\n");
        printf("9. Logout\n");
        printf("Enter choice: ");
        
        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n');
            continue;
        }
        while(getchar() != '\n');

        switch (choice) {
            case 1:
                display_all_shows();
                break;
            case 2:
                book_ticket(user_id);
                break;
            case 3:
                view_my_tickets(user_id);
                break;
            case 4:
                cancel_ticket(user_id);
                break;
            case 5:
                view_my_waitlists(user_id);
                break;
            case 6:
                leave_waitlist(user_id);
                break;
            case 7:
                recommend_movies(user_id);
                break;
            case 8:
                display_genre_graph();
                break;
            case 9:
                printf("Logging out...\n");
                return;
            default:
                printf("Invalid choice.\n");
        }
    }
}

int main() {
    int choice;
    while (1) {
        printf("\n=== Welcome ===\n");
        printf("1. Login\n");
        printf("2. Register\n");
        printf("3. Exit\n");
        printf("Enter choice: ");
        
        if (scanf("%d", &choice) != 1) {
            while(getchar() != '\n');
            continue;
        }
        while(getchar() != '\n');

        if (choice == 1) {
            char username[MAX_FIELD_LENGTH];
            char password[MAX_FIELD_LENGTH];

            printf("Username: ");
            scanf("%s", username);
            while(getchar() != '\n');

            printf("Password: ");
            scanf("%s", password);
            while(getchar() != '\n');

            int user_id = login_user(username, password);
            if (user_id != -1) {
                printf("Login successful! Welcome User %d.\n", user_id);
                user_menu(user_id);
            } else {
                printf("Invalid credentials.\n");
            }
        } else if (choice == 2) {
            char username[MAX_FIELD_LENGTH];
            char password[MAX_FIELD_LENGTH];

            printf("Enter New Username: ");
            scanf("%s", username);
            while(getchar() != '\n');

            printf("Enter New Password: ");
            scanf("%s", password);
            while(getchar() != '\n');

            int user_id = register_user(username, password);
            if (user_id != -1) {
                printf("Registration successful! You are now logged in as User %d.\n", user_id);
                user_menu(user_id);
            }
        } else if (choice == 3) {
            printf("Goodbye!\n");
            break;
        } else {
            printf("Invalid choice.\n");
        }
    }
    return 0;
}
