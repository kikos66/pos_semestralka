#include "server.h"
#include <string.h>
#include "communication.h"
#include "game.h"
#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/poll.h>

pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
server_state_t state = SERVER_LOBBY;

int ship_handler(int client, Board* board, int limit) {
    int placed_count = 0;

    while (1) {
        char buffer[BUF_SIZE];

        if (limit != -1 && placed_count >= limit) {
            sendMsg(client, "READY");
            return placed_count;
        }

        memset(buffer, 0, BUF_SIZE);
        int bytes = recvBuff(client, buffer, BUF_SIZE);
        if (bytes <= 0) {
            return 0;
        }

        if (limit == -1 && strncmp(buffer, "DONE", 4) == 0) {
            if (placed_count > 0) {
                sendMsg(client, "READY");
                return placed_count;
            } else {
                sendMsg(client, "ERROR MIN 1");
                continue;
            }
        }

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

void generate_random_shot(Game* game, int shooter_idx, int num_clients, int* player_alive, int* target_id, int* x, int* y) {
    int target;

    while (1) {
        int r = rand() % num_clients;
        if (r != shooter_idx && player_alive[r]) {
            target = r;
            break;
        }
    }

    Board* b = game->boards[target];
    int rx, ry;

    while (1) {
        rx = rand() % b->width;
        ry = rand() % b->height;
        if (b->hits_map[ry * b->width + rx] == 0) {
            break;
        }
    }
    *target_id = target;
    *x = rx;
    *y = ry;
}

void run_server(int port, int num_clients, int height, int width) {
    srand(time(NULL));

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
    state = SERVER_IN_GAME;
    for (int i = 0; i < num_clients; i++) {
        sendMsg(clients[i], "START");
    }

    printf("[SERVER] Host (Player 0) is placing ships...\n");
    sendMsg(clients[0], "INIT HOST");
    int total_ships = ship_handler(clients[0], gameInstance->boards[0], -1);
    printf("[SERVER] Host placed %d ships.\n", total_ships);

    for (int i = 1; i < num_clients; i++) {
        char msg[BUF_SIZE];
        snprintf(msg, BUF_SIZE, "INIT %d", total_ships);
        sendMsg(clients[i], msg);
        ship_handler(clients[i], gameInstance->boards[i], total_ships);
    }

    printf("[SERVER] All ships placed. Starting Game Loop.\n");

    int* player_alive = (int*)malloc(sizeof(int) * num_clients);
    for(int i = 0; i < num_clients; i++) {
        player_alive[i] = 1;
    }
    int players_remaining = num_clients;
    int current_turn = 0;
    char buffer[BUF_SIZE];

    while (state == SERVER_IN_GAME) {
        if (player_alive[current_turn] == 0) {
            current_turn = (current_turn + 1) % num_clients;
            continue;
        }

        int active_fd = clients[current_turn];
        char turn_msg[BUF_SIZE];
        snprintf(turn_msg, BUF_SIZE, "TURN %d %d", current_turn, ROUND_TIMEOUT_SEC);

        for (int i = 0; i < num_clients; i++) {
            sendMsg(clients[i], turn_msg);
        }

        struct pollfd pfd;
        pfd.fd = active_fd;
        pfd.events = POLLIN;

        int ret = poll(&pfd, 1, ROUND_TIMEOUT_SEC * 1000);

        int target_id = -1, x = -1, y = -1;
        int valid_input_received = 0;

         if (ret > 0) {
            if (pfd.revents & POLLIN) {
                memset(buffer, 0, BUF_SIZE);
                int bytes = recvBuff(active_fd, buffer, BUF_SIZE);

                if (bytes <= 0) {
                    printf("Player %d disconnected.\n", current_turn);
                    player_alive[current_turn] = 0;
                    players_remaining--;
                    close(active_fd);
                    if (players_remaining < 2) {
                        break;
                    }
                    current_turn = (current_turn + 1) % num_clients;
                    continue;
                }

                if (sscanf(buffer, "%d %d %d", &target_id, &x, &y) == 3) {
                    valid_input_received = 1;
                } else {
                    sendMsg(active_fd, "ERROR Invalid format");
                    continue;
                }
            }
        }
        else if (ret == 0) {
            printf("\n[SERVER] Player %d timed out. Performing random shot.\n", current_turn);
            generate_random_shot(gameInstance, current_turn, num_clients, player_alive, &target_id, &x, &y);
            char info[BUF_SIZE];
            snprintf(info, BUF_SIZE, "RANDOM SHOT %d %d %d %d", current_turn, target_id, x, y);
            for (int i = 0; i < num_clients; i++) {
                sendMsg(clients[i], info);
            }
            valid_input_received = 1;
        }
        else {
            perror("poll error");
            break;
        }

        if (valid_input_received) {
            int target_idx = target_id;
            if (target_idx < 0 || target_idx >= num_clients || target_idx == current_turn || player_alive[target_idx] == 0) {
                sendMsg(active_fd, "ERROR Player ID is invalid");
                continue;
            }

            pthread_mutex_lock(&server_mutex);
            int is_hit = hit(gameInstance->boards[target_idx], x, y);
            int still_alive = is_player_alive(gameInstance->boards[target_idx]);
            pthread_mutex_unlock(&server_mutex);

            char res[BUF_SIZE];
            if (is_hit == 1) {
                snprintf(res, BUF_SIZE, "HIT %d %d %d %d", current_turn, target_idx, x, y);
            } else if (is_hit == 0) {
                snprintf(res, BUF_SIZE, "MISS %d %d %d %d", current_turn, target_idx, x, y);
            } else {
                sendMsg(active_fd, "ERROR Already hit there! Try again.");
                continue;
            }

            for (int i = 0; i < num_clients; i++) {
                sendMsg(clients[i], res);
            }

            if (!still_alive) {
                player_alive[target_idx] = 0;
                players_remaining--;
                snprintf(res, BUF_SIZE, "ELIMINATED %d", target_idx);
                for (int i = 0; i < num_clients; i++) {
                    sendMsg(clients[i], res);
                }
            }

            if (players_remaining == 1) {
                int winner = -1;
                for (int i = 0; i < num_clients; i++) {
                    if(player_alive[i]) {
                        winner = i;
                    }
                }
                snprintf(res, BUF_SIZE, "WINNER %d", winner);
                for (int i = 0; i < num_clients; i++) {
                    sendMsg(clients[i], res);
                }
                state = SERVER_GAME_OVER;
            } else {
                current_turn = (current_turn + 1) % num_clients;
            }
        }
    }
    free(player_alive);
    game_destroy(gameInstance);
    close(server_fd);
    printf("[SERVER] Game Over. Server shutting down.\n");
}