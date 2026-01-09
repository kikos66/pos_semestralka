#include "server.h"
#include "game.h"
#include <string.h>
#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include <arpa/inet.h>

pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;

void sendMsg(int sockfd, char* msg) {
    char buffer[BUF_SIZE + 2];
    snprintf(buffer, sizeof(buffer), "%s\n", msg);
    write(sockfd, buffer, strlen(buffer));
}

int ship_handler(int client, Board* board, int limit) {
    int placed_count = 0;

    while (1) {
        char buffer[BUF_SIZE];
        int x, y, h, w;
        if (sscanf(buffer, "%d %d %d %d", &x, &y, &h, &w) == 4) {
            pthread_mutex_lock(&server_mutex);
            int result = add_ship(board, h, w, x, y);
            pthread_mutex_unlock(&server_mutex);
            if (result == 1) {
                placed_count++;
                sendMsg(client, "OK");
            } else if (result == 0) {
                sendMsg(client, "FULL");
            } else {
                sendMsg(client, "INVALID");
            }
        }
    }
}

void run_server(int port, int num_clients, int height, int width) {
    int server_fd, newSocket;
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    int connected_clients = 0;
    int clients[MAX_CLIENTS];
    Game* gameInstance = game_init(height, width, num_clients);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, num_clients) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    printf("[SERVER] Waiting for players on port %d...\n", port);

    while (connected_clients < num_clients) {
        if ((newSocket = accept(server_fd, (struct sockaddr *)&addr, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            continue;
        }

        clients[connected_clients] = newSocket;
        printf("[SERVER] Player %d connected.\n", connected_clients);

        char id_msg[64];
        snprintf(id_msg, sizeof(id_msg), "ID %d %d %d %d",
                 connected_clients, num_clients, height, width);
        sendMsg(newSocket, id_msg);

        connected_clients++;
    }

    for (int i = 0; i < num_clients; i++) {
        sendMsg(clients[i], "START");
    }

    printf("[SERVER] Host (Player 0) is placing ships...\n");
    sendMsg(clients[0], "INIT HOST");
    int total_ships = ship_handler(clients[0], gameInstance->boards[0], -1);
    printf("[SERVER] Host placed %d ships.\n", total_ships);
}