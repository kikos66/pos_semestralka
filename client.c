#include "client.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

int sock, player_id;
volatile int game_running = 1;

int recvBuff(int sockfd, char* buff, int size) {
    int i = 0;
    char c = '\0';
    int n;
    while (i < size - 1) {
        n = read(sockfd, &c, 1);
        if (n <= 0) {
            return -1;
        }
        if (c == '\n') {
            break;
        }
        buff[i++] = c;
    }
    buff[i] = '\0';
    return i;
}

void* receive_thread() {
    char buffer[BUF_SIZE];
    int bytes;
    while (game_running && (bytes = recvBuff(sock, buffer, BUF_SIZE - 1)) > 0) {
        buffer[bytes] = '\0';

    }
}

void run_client(int port) {
    pthread_t recv_thread;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    char buffer[BUF_SIZE];

    int temp_n, temp_h, temp_w;
    if (sscanf(buffer, "ID %d %d %d %d", &player_id, &temp_n, &temp_h, &temp_w) == 4) {
        printf("Connected as Player %d.\n", player_id);
        printf("Game Config Received: %d Players, Map %dx%d\n", temp_n, temp_w, temp_h);
    } else {
        printf("Error: Invalid handshake from server.\n");
        close(sock);
        return;
    }

    printf("Connected as Player %d. Waiting for game start...\n", player_id);

    pthread_create(&recv_thread, NULL, receive_thread, NULL);
}