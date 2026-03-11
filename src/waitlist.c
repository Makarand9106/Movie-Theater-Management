#include "../include/waitlist.h"
#include "../include/csv_utils.h"
#include "../include/shows.h"
#include <stdlib.h>

// LINKED LIST QUEUE NODE (with ticket count)
typedef struct WaitlistNode {
    int waitlist_id;
    int user_id;
    int show_id;
    int ticket_count; 
    struct WaitlistNode *next;
} WaitlistNode;

// STACK NODE (for temporary storage during dequeue)
typedef struct StackNode {
    WaitlistNode *data;
    struct StackNode *next;
} StackNode;

// Stack operations
void push(StackNode **top, WaitlistNode *node) {
    StackNode *new_node = (StackNode*)malloc(sizeof(StackNode));
    new_node->data = node;
    new_node->next = *top;
    *top = new_node;
}

WaitlistNode* pop(StackNode **top) {
    if (*top == NULL) return NULL;
    StackNode *temp = *top;
    WaitlistNode *data = temp->data;
    *top = (*top)->next;
    free(temp);
    return data;
}

int is_stack_empty(StackNode *top) {
    return top == NULL;
}

// Helper: Create new queue node
WaitlistNode* create_node(int waitlist_id, int user_id, int show_id, int ticket_count) {
    WaitlistNode *node = (WaitlistNode*)malloc(sizeof(WaitlistNode));
    node->waitlist_id = waitlist_id;
    node->user_id = user_id;
    node->show_id = show_id;
    node->ticket_count = ticket_count;
    node->next = NULL;
    return node;
}

// Helper: Load entire waitlist from CSV into memory
WaitlistNode* load_waitlist_from_csv() {
    WaitlistNode *head = NULL;
    WaitlistNode *tail = NULL;
    
    FILE *file = fopen("data/waitlist.csv", "r");
    if (!file) return NULL;
    
    char line[MAX_LINE_LENGTH];
    int row = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (row++ == 0) continue;
        trim_newline(line);
        
        char *fields[MAX_FIELDS];
        int fc = 0;
        char *token = strtok(line, ",");
        while (token) {
            fields[fc++] = token;
            token = strtok(NULL, ",");
        }
        
        if (fc >= 4) {
            WaitlistNode *node = create_node(
                atoi(fields[0]),  // waitlist_id
                atoi(fields[1]),  // user_id
                atoi(fields[2]),  // show_id
                atoi(fields[3])   // ticket_count
            );
            
            if (head == NULL) {
                head = tail = node;
            } else {
                tail->next = node;
                tail = node;
            }
        }
    }
    fclose(file);
    return head;
}

// Helper: Save entire waitlist back to CSV
void save_waitlist_to_csv(WaitlistNode *head) {
    FILE *file = fopen("data/waitlist.csv", "w");
    if (!file) return;
    
    fprintf(file, "waitlist_id,user_id,show_id,ticket_count\n");
    
    WaitlistNode *current = head;
    while (current != NULL) {
        fprintf(file, "%d,%d,%d,%d\n", 
                current->waitlist_id, 
                current->user_id, 
                current->show_id,
                current->ticket_count);
        current = current->next;
    }
    fclose(file);
}

// Helper: Free entire waitlist
void free_waitlist(WaitlistNode *head) {
    while (head != NULL) {
        WaitlistNode *temp = head;
        head = head->next;
        free(temp);
    }
}

// ENQUEUE: Add user to waitlist for a show (with ticket count)
int add_to_waitlist(int user_id, int show_id) {
    int ticket_count;
    printf("How many tickets do you need? ");
    if (scanf("%d", &ticket_count) != 1 || ticket_count <= 0) {
        while(getchar() != '\n');
        printf("Invalid ticket count.\n");
        return -1;
    }
    while(getchar() != '\n');
    
    // Load entire waitlist
    WaitlistNode *head = load_waitlist_from_csv();
    
    // Find the tail
    WaitlistNode *current = head;
    WaitlistNode *prev = NULL;
    
    while (current != NULL) {
        prev = current;
        current = current->next;
    }
    
    // Generate new ID
    int new_id = get_next_id("data/waitlist.csv");
    
    // Create new node
    WaitlistNode *new_node = create_node(new_id, user_id, show_id, ticket_count);
    
    // Add to end of list (FIFO)
    if (prev == NULL) {
        head = new_node;
    } else {
        prev->next = new_node;
    }
    
    // Save back to CSV
    save_waitlist_to_csv(head);
    free_waitlist(head);
    
    printf("Added to waitlist for %d ticket(s).\n", ticket_count);
    return 0;
}

// DEQUEUE with STACK: Process waitlist for a show (find first user with matching ticket count)
// Returns: user_id if found, -1 if not found
// Sets: *matched_ticket_count to the number of tickets the matched user needs
int process_waitlist(int show_id, int available_seats, int *matched_ticket_count) {
    // Load entire waitlist
    WaitlistNode *head = load_waitlist_from_csv();
    
    // Stack for temporary storage
    StackNode *stack = NULL;
    
    WaitlistNode *current = head;
    WaitlistNode *prev = NULL;
    int found_user_id = -1;
    *matched_ticket_count = 0;
    
    // Traverse queue, looking for first match for this show
    while (current != NULL) {
        if (current->show_id == show_id) {
            if (current->ticket_count <= available_seats) {
                // MATCH FOUND!
                found_user_id = current->user_id;
                *matched_ticket_count = current->ticket_count;
                
                // Remove this node from queue
                if (prev == NULL) {
                    head = current->next;
                } else {
                    prev->next = current->next;
                }
                
                WaitlistNode *to_free = current;
                current = current->next;
                free(to_free);
                break;
            } else {
                // No match - push to stack and continue
                WaitlistNode *next = current->next;
                
                // Remove from queue
                if (prev == NULL) {
                    head = next;
                } else {
                    prev->next = next;
                }
                
                current->next = NULL; 
                push(&stack, current);
                current = next;
                continue;
            }
        }
        prev = current;
        current = current->next;
    }
    
    // Pop from stack and re-enqueue at end
    while (!is_stack_empty(stack)) {
        WaitlistNode *node = pop(&stack);
        
        // Find tail
        WaitlistNode *tail = head;
        if (tail == NULL) {
            head = node;
        } else {
            while (tail->next != NULL) {
                tail = tail->next;
            }
            tail->next = node;
        }
    }
    
    // Save updated list back to CSV
    save_waitlist_to_csv(head);
    free_waitlist(head);
    
    return found_user_id;
}

// View user's waitlist entries
void view_my_waitlists(int user_id) {
    printf("\nMy Waitlists:\n");
    printf("------------------------------------------------------------------------------------------------\n");
    
    WaitlistNode *head = load_waitlist_from_csv();
    WaitlistNode *current = head;
    int found_any = 0;
    
    while (current != NULL) {
        if (current->user_id == user_id) {
            found_any = 1;
            
            Show show;
            if (get_show_details(current->show_id, &show) == 0) {
                Movie movie;
                Theatre theatre;
                get_movie_details(show.movie_id, &movie);
                get_theatre_details(show.theatre_id, &theatre);

                // Calculate position for this show
                int pos = 1;
                WaitlistNode *temp = head;
                while (temp != current) {
                    if (temp->show_id == current->show_id) {
                        pos++;
                    }
                    temp = temp->next;
                }

                printf("Waitlist ID: %d | Movie: %s | Theatre: %s | Time: %s | Tickets: %d | Position: %d\n",
                       current->waitlist_id, movie.name, theatre.name, show.datetime, current->ticket_count, pos);
            }
        }
        current = current->next;
    }
    
    if (!found_any) {
        printf("You are not on any waitlists.\n");
    }
    printf("------------------------------------------------------------------------------------------------\n");
    
    free_waitlist(head);
}

// Leave a waitlist
void leave_waitlist(int user_id) {
    // First show user's waitlists
    printf("\nYour Waitlists:\n");
    printf("------------------------------------------------------------------------------------------------\n");
    
    WaitlistNode *head = load_waitlist_from_csv();
    WaitlistNode *current = head;
    int found_any = 0;
    
    while (current != NULL) {
        if (current->user_id == user_id) {
            found_any = 1;
            
            Show show;
            if (get_show_details(current->show_id, &show) == 0) {
                Movie movie;
                Theatre theatre;
                get_movie_details(show.movie_id, &movie);
                get_theatre_details(show.theatre_id, &theatre);

                // Calculate position
                int pos = 1;
                WaitlistNode *temp = head;
                while (temp != current) {
                    if (temp->show_id == current->show_id) {
                        pos++;
                    }
                    temp = temp->next;
                }

                printf("Waitlist ID: %d | Movie: %s | Theatre: %s | Time: %s | Tickets: %d | Position: %d\n",
                       current->waitlist_id, movie.name, theatre.name, show.datetime, current->ticket_count, pos);
            }
        }
        current = current->next;
    }
    
    if (!found_any) {
        printf("You are not on any waitlists.\n");
        printf("------------------------------------------------------------------------------------------------\n");
        free_waitlist(head);
        return;
    }
    printf("------------------------------------------------------------------------------------------------\n");

    int waitlist_id;
    printf("\nEnter Waitlist ID to leave (Enter 0 to go back): ");
    if (scanf("%d", &waitlist_id) != 1) {
        while(getchar() != '\n');
        free_waitlist(head);
        return;
    }
    while(getchar() != '\n');

    if (waitlist_id == 0) {
        free_waitlist(head);
        return;
    }

    // Find and remove the node
    current = head;
    WaitlistNode *prev = NULL;
    int found = 0;

    while (current != NULL) {
        if (current->waitlist_id == waitlist_id && current->user_id == user_id) {
            found = 1;
            
            // Remove node
            if (prev == NULL) {
                head = current->next;
            } else {
                prev->next = current->next;
            }
            
            free(current);
            break;
        }
        prev = current;
        current = current->next;
    }

    if (!found) {
        printf("Invalid Waitlist ID or entry does not belong to you.\n");
    } else {
        printf("Successfully left the waitlist.\n");
        
        save_waitlist_to_csv(head);
    }
    
    free_waitlist(head);
}
