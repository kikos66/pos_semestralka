#ifndef INTERFACE_H
#define INTERFACE_H

#include <time.h>

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define BLUE "\x1b[34m"
#define RESET "\x1b[0m"
#define YELLOW "\x1b[33m"

#define BUF_SIZE 256

#define BOARD_INDEX(p, y, x, h, w) (((p) * (h) + (y)) * (w) + (x))

void clear_screen();
void render_header(int player_id, int current_turn_player, time_t turn_end_time);
void render_game(int player_id, int num_players, int h, int w, char* boards,
                   int current_turn_player, time_t turn_end_time, char status_msg[][BUF_SIZE], int status_msg_count);
void print_message(const char* msg);

#endif //INTERFACE_H
