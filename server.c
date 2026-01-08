#include "server.h"
#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include <arpa/inet.h>

void run_server(int port, int num_clients, int height, int width) {
    int server_fd, newSocket;
    struct sockaddr_in addr;
    int addrlen = sizeof(addr);
    int connected_clients = 0;
    int clients[MAX_CLIENTS];

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
}