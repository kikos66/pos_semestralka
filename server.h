#ifndef SERVER_H
#define SERVER_H
#include <pthread.h>

#define MAX_CLIENTS 10

typedef enum {
    SERVER_LOBBY,
    SERVER_IN_GAME,
    SERVER_GAME_OVER
} server_state_t;

extern server_state_t state;

void run_server(int port, int num_clients, int height, int width);

#endif //SERVER_H
