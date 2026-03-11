#include "../include/bfs.h"
#include "../include/shows.h"
#include "../include/csv_utils.h"
#include <stdlib.h>

#define MAX_MOVIES 100

typedef struct {
    int movie_id;
    char name[MAX_FIELD_LENGTH];
    int genre_ids[10];
    int genre_count;
} MovieNode;

typedef struct {
    float adj[MAX_MOVIES][MAX_MOVIES];  // Weighted adjacency matrix
    MovieNode movies[MAX_MOVIES];
    int movie_count;
} MovieGraph;

// LINKED LIST for watched movies
typedef struct WatchedMovieNode {
    int movie_id;
    struct WatchedMovieNode *next;
} WatchedMovieNode;

typedef struct {
    int user_id;
    WatchedMovieNode *watched_head;  // LINKED LIST instead of array
    int watched_count;
} UserHistoryContext;

// Helper function to add movie to watched list
void add_watched_movie(UserHistoryContext *ctx, int movie_id) {
    // Check if already in list
    WatchedMovieNode *current = ctx->watched_head;
    while (current != NULL) {
        if (current->movie_id == movie_id) {
            return; // Already added
        }
        current = current->next;
    }
    
    // Add new node at head
    WatchedMovieNode *new_node = (WatchedMovieNode*)malloc(sizeof(WatchedMovieNode));
    new_node->movie_id = movie_id;
    new_node->next = ctx->watched_head;
    ctx->watched_head = new_node;
    ctx->watched_count++;
}

// Helper function to free watched list
void free_watched_list(WatchedMovieNode *head) {
    while (head != NULL) {
        WatchedMovieNode *temp = head;
        head = head->next;
        free(temp);
    }
}

int history_handler(char **fields, int field_count, void *context) {
    UserHistoryContext *ctx = (UserHistoryContext *)context;
    // ticket_id, user_id, show_id, seat_number, status
    if (atoi(fields[1]) == ctx->user_id && strcmp(fields[4], "BOOKED") == 0) {
        int show_id = atoi(fields[2]);
        Show show;
        if (get_show_details(show_id, &show) == 0) {
            add_watched_movie(ctx, show.movie_id);
        }
    }
    return 0;
}

void parse_genres(char *genre_str, int *genres, int *count) {
    *count = 0;
    char *token = strtok(genre_str, "|");
    while (token) {
        genres[(*count)++] = atoi(token);
        token = strtok(NULL, "|");
    }
}

// Calculate Jaccard similarity between two movies based on shared genres
float calculate_similarity(MovieNode *m1, MovieNode *m2) {
    if (m1->genre_count == 0 || m2->genre_count == 0) return 0.0;
    
    // Count shared genres (intersection)
    int shared = 0;
    for (int i = 0; i < m1->genre_count; i++) {
        for (int j = 0; j < m2->genre_count; j++) {
            if (m1->genre_ids[i] == m2->genre_ids[j]) {
                shared++;
                break;
            }
        }
    }
    
    // Calculate union size
    int union_size = m1->genre_count + m2->genre_count - shared;
    
    // Jaccard similarity
    return (float)shared / union_size;
}

void build_movie_graph(MovieGraph *graph) {
    // Load all movies
    FILE *file = fopen("data/movies.csv", "r");
    char line[MAX_LINE_LENGTH];
    int row = 0;
    
    while (fgets(line, sizeof(line), file) && graph->movie_count < MAX_MOVIES) {
        if (row++ == 0) continue;
        trim_newline(line);
        
        char *fields[MAX_FIELDS];
        int fc = 0;
        char line_copy[MAX_LINE_LENGTH];
        strcpy(line_copy, line);
        char *token = strtok(line_copy, ",");
        while (token) {
            fields[fc++] = token;
            token = strtok(NULL, ",");
        }
        
        if (fc >= 4) {
            int idx = graph->movie_count;
            graph->movies[idx].movie_id = atoi(fields[0]);
            strcpy(graph->movies[idx].name, fields[1]);
            parse_genres(fields[3], graph->movies[idx].genre_ids, &graph->movies[idx].genre_count);
            graph->movie_count++;
        }
    }
    fclose(file);
    
    // Build weighted edges
    for (int i = 0; i < graph->movie_count; i++) {
        for (int j = 0; j < graph->movie_count; j++) {
            if (i == j) {
                graph->adj[i][j] = 0.0;
            } else {
                graph->adj[i][j] = calculate_similarity(&graph->movies[i], &graph->movies[j]);
            }
        }
    }
}

typedef struct {
    int movie_idx;
    float similarity;
} RecommendationScore;

void recommend_movies(int user_id) {
    // 1. Get User History (LINKED LIST)
    UserHistoryContext history = {user_id, NULL, 0};
    read_csv("data/tickets.csv", 1, history_handler, &history);

    if (history.watched_count == 0) {
        printf("No history found. Watch some movies to get recommendations!\n");
        return;
    }

    // 2. Build Movie Graph
    MovieGraph graph = {{{0}}, {{0}}, 0};
    build_movie_graph(&graph);

    printf("\n=== Movie Recommendation Process ===\n");
    printf("Using Weighted Movie Similarity Graph with BFS-like scoring\n\n");
    
    // Display watched movies (LINKED LIST TRAVERSAL)
    printf("Step 1: Your Watched Movies\n");
    printf("------------------------------------------------------------------------------------------------\n");
    WatchedMovieNode *current = history.watched_head;
    while (current != NULL) {
        for (int j = 0; j < graph.movie_count; j++) {
            if (graph.movies[j].movie_id == current->movie_id) {
                printf("  - %s\n", graph.movies[j].name);
                break;
            }
        }
       current = current->next;
    }
    printf("\n");

    // 3. Calculate recommendation scores using weighted BFS
    float scores[MAX_MOVIES] = {0};
    int visited[MAX_MOVIES] = {0};
    
    // Mark watched movies (LINKED LIST TRAVERSAL)
    current = history.watched_head;
    while (current != NULL) {
        for (int j = 0; j < graph.movie_count; j++) {
            if (graph.movies[j].movie_id == current->movie_id) {
                visited[j] = 1;
                break;
            }
        }
        current = current->next;
    }
    
    // Calculate aggregate similarity scores for unwatched movies
    printf("Step 2: Calculating Similarity Scores (Traversing Graph)\n");
    printf("------------------------------------------------------------------------------------------------\n");
    printf("For each unwatched movie, computing aggregate similarity to all watched movies...\n\n");
    
    int displayed_details = 0;
    for (int i = 0; i < graph.movie_count && displayed_details < 5; i++) {
        if (visited[i]) continue;
        
        printf("Candidate: %s\n", graph.movies[i].name);
        
        // Sum similarity to all watched movies (LINKED LIST TRAVERSAL)
        current = history.watched_head;
        while (current != NULL) {
            for (int j = 0; j < graph.movie_count; j++) {
                if (graph.movies[j].movie_id == current->movie_id && graph.adj[i][j] > 0) {
                    scores[i] += graph.adj[i][j];
                    printf("  + Similarity to %s: %.3f\n", graph.movies[j].name, graph.adj[i][j]);
                    break;
                }
            }
            current = current->next;
        }
        printf("  = Total Score: %.3f\n\n", scores[i]);
        displayed_details++;
    }
    
    // Continue calculating for remaining movies silently (LINKED LIST TRAVERSAL)
    for (int i = 0; i < graph.movie_count; i++) {
        if (visited[i]) continue;
        
        // If not already calculated
        if (scores[i] == 0) {
            current = history.watched_head;
            while (current != NULL) {
                for (int j = 0; j < graph.movie_count; j++) {
                    if (graph.movies[j].movie_id == current->movie_id) {
                        scores[i] += graph.adj[i][j];
                        break;
                    }
                }
                current = current->next;
            }
        }
    }
    
    if (graph.movie_count > displayed_details + history.watched_count) {
        printf("... (showing first 5 candidates, %d more calculated silently)\n\n", 
               graph.movie_count - displayed_details - history.watched_count);
    }
    
    // 4. Sort recommendations by score
    RecommendationScore recommendations[MAX_MOVIES];
    int rec_count = 0;
    
    for (int i = 0; i < graph.movie_count; i++) {
        if (!visited[i] && scores[i] > 0) {
            recommendations[rec_count].movie_idx = i;
            recommendations[rec_count].similarity = scores[i];
            rec_count++;
        }
    }
    
    // Simple bubble sort by similarity (descending)
    for (int i = 0; i < rec_count - 1; i++) {
        for (int j = 0; j < rec_count - i - 1; j++) {
            if (recommendations[j].similarity < recommendations[j + 1].similarity) {
                RecommendationScore temp = recommendations[j];
                recommendations[j] = recommendations[j + 1];
                recommendations[j + 1] = temp;
            }
        }
    }
    
    // 5. Display recommendations
    printf("Step 3: Final Recommendations (Sorted by Score)\n");
    printf("------------------------------------------------------------------------------------------------\n");
    
    int displayed = 0;
    for (int i = 0; i < rec_count && displayed < 10; i++) {
        int idx = recommendations[i].movie_idx;
        printf("%2d. %s (Score: %.3f)\n", 
               displayed + 1,
               graph.movies[idx].name, 
               recommendations[i].similarity);
        displayed++;
    }
    
    if (displayed == 0) {
        printf("No recommendations available.\n");
    }
    
    printf("================================================================================================\n");
    
    // CLEANUP: Free linked list memory
    free_watched_list(history.watched_head);
}

void display_genre_graph() {
    printf("\n=== Movie Similarity Graph (Weighted) ===\n");
    printf("This graph shows how movies are connected based on shared genres.\n");
    printf("Edge weights represent Jaccard similarity (0.0 to 1.0).\n\n");
    
    // Build the graph
    MovieGraph graph = {{{0}}, {{0}}, 0};
    build_movie_graph(&graph);
    
    // Display graph
    printf("Nodes (Movies):\n");
    printf("------------------------------------------------------------------------------------------------\n");
    for (int i = 0; i < graph.movie_count; i++) {
        printf("  [%d] %s (Genres: ", graph.movies[i].movie_id, graph.movies[i].name);
        for (int j = 0; j < graph.movies[i].genre_count; j++) {
            printf("%d", graph.movies[i].genre_ids[j]);
            if (j < graph.movies[i].genre_count - 1) printf(", ");
        }
        printf(")\n");
    }
    
    printf("\nEdges (Similarity >= 0.3 shown):\n");
    printf("------------------------------------------------------------------------------------------------\n");
    int edge_count = 0;
    for (int i = 0; i < graph.movie_count; i++) {
        for (int j = i + 1; j < graph.movie_count; j++) {
            if (graph.adj[i][j] >= 0.3) {
                printf("  %s <--> %s (Weight: %.2f)\n", 
                       graph.movies[i].name, 
                       graph.movies[j].name, 
                       graph.adj[i][j]);
                edge_count++;
            }
        }
    }
    
    if (edge_count == 0) {
        printf("  No strong connections found (threshold: 0.3).\n");
    }
    
    printf("\nTotal Strong Connections: %d\n", edge_count);
    printf("================================================================================================\n");
}
