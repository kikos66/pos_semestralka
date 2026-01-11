#include "interface.h"
#include <stdio.h>
#include <time.h>

void clear_screen() {
    printf("\033[2J\033[H");
}

void render_header(int player_id, int current_turn_player, time_t turn_end_time) {
    printf("\033[s\033[H");
    printf("=== BATTLESHIP : Player %d ===\n", player_id);

    if (current_turn_player != -1) {
        int remaining = (int)(turn_end_time - time(NULL));
        if (remaining < 0) {
            remaining = 0;
        }
        if (current_turn_player == player_id) {
            printf(GREEN "Your turn! " RESET);
        }
        else
            printf("Player %d's turn. ", current_turn_player);

        printf("Time left: %d sec\n", remaining);
    }
    printf("\n\033[u");
    fflush(stdout);
}

void render_game(int player_id, int num_players, int h, int w, char* boards,
                   int current_turn_player, time_t turn_end_time, char status_msg[][BUF_SIZE], int status_msg_count) {
    clear_screen();
    render_header(player_id, current_turn_player, turn_end_time);

    printf("\n\n--- MY BOARD ---\n   ");
    for(int x = 0; x < w; x++) {
        printf("%d ", x);
    }
    printf("\n");

    for (int y = 0; y < h; y++) {
        printf("%d |", y);
        for (int x = 0; x < w; x++) {
            char cell = boards[BOARD_INDEX(player_id, y, x, h, w)];
            if (cell == 0) {
                printf(BLUE "~ " RESET);
            }
            else if (cell == 1) {
                printf(GREEN "S " RESET);
            }
            else if (cell == 2) {
                printf(YELLOW "O " RESET);
            }
            else if (cell == 3) {
                printf(RED "X " RESET);
            }
        }
        printf("|\n");
    }

    printf("\n--- OPPONENT BOARDS ---\n");
    for (int p = 0; p < num_players; p++) {
        if (p == player_id) continue;

        printf("Player %d:\n   ", p);
        for(int x = 0; x < w; x++) {
            printf("%d ", x);
        }
        printf("\n");

        for (int y = 0; y < h; y++) {
            printf("%d |", y);
            for (int x = 0; x < w; x++) {
                char cell = boards[BOARD_INDEX(p, y, x, h, w)];
                if (cell == 0 || cell == 1) {
                    printf(BLUE "~ " RESET);
                }
                else if (cell == 2) {
                    printf(YELLOW "O " RESET);
                }
                else if (cell == 3) {
                    printf(RED "X " RESET);
                }
            }
            printf("|\n");
        }
    }
    if (status_msg_count > 0) {
        printf("\n");
        for (int i = 0; i < status_msg_count; i++) {
            printf(RED "%s\n" RESET, status_msg[i]);
        }
    }
    fflush(stdout);
}

void print_message(const char* msg) {
    printf("%s\n", msg);
}