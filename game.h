#ifndef GAME_H
#define GAME_H

typedef struct {
    int height;
    int width;
    int hits;
}Ship;

typedef struct {
    int height;
    int width;
    int* hits_map;
    Ship** ships_map;
}Board;

typedef struct {
    int number_of_players;
    Board** boards;
}Game;

Board* board_init(int height, int width);
Ship* ship_init(int height, int width);
Game* game_init(int height, int width, int number_of_players);
int add_ship(Board* board, int height, int width, int x, int y);
void game_destroy(Game* game);
int hit(Board* board, int x, int y);
int is_player_alive(Board* board);

#endif //GAME_H
