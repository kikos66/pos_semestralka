#include "game.h"
#include <stdlib.h>

Board* board_init(int height, int width) {
    Board* board = (Board*)malloc(sizeof(Board));
    board->height = height;
    board->width = width;
    board->ships_map = (Ship**)malloc(sizeof(Ship*) * height * width);
    for (int i = 0; i < height * width; i++) {
        board->ships_map[i] = NULL;
    }
    return board;
}

Ship* ship_init(int height, int width) {
    Ship* ship = (Ship*)malloc(sizeof(Ship));
    ship->height = height;
    ship->width = width;
    return ship;
}

Game* game_init(int height, int width, int numberOfPlayers) {
    Game* game = (Game*)malloc(sizeof(Game));
    game->number_of_players = numberOfPlayers;
    game->boards = (Board**)malloc(sizeof(Board*) * numberOfPlayers);

    for (int i = 0; i < numberOfPlayers; i++) {
        game->boards[i] = board_init(height, width);
    }
    return game;
}

int add_ship(Board* board, int height, int width, int x, int y) {
    Ship* ship = ship_init(height, width);
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            if (board->ships_map[(y+i)*board->width+(x+j)] != NULL) {
                free(ship);
                return 0;
            }
        }
    }
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            board->ships_map[(y+i)*board->width+(x+j)] = ship;
        }
    }
    return 1;
}

void ship_destroy(Ship* ship) {
    free(ship);
}

void board_destroy(Board* board) {
    for (int i = 0; i < board->height * board->width; i++) {
        if (board->ships_map[i] != NULL) {
            ship_destroy(board->ships_map[i]);
        }
    }
    free(board->ships_map);
    free(board);
}

void game_destroy(Game* game) {
    for (int i = 0; i < game->number_of_players; i++) {
        board_destroy(game->boards[i]);
    }
    free(game->boards);
    free(game);
}

