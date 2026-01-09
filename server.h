#ifndef SERVER_H
#define SERVER_H
#include <pthread.h>

#define MAX_CLIENTS 10
#define BUF_SIZE 256

void run_server(int port, int num_clients, int height, int width);

#endif //SERVER_H
