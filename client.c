#include "client.h"
#include "communication.h"
#include "server.h"
#include "interface.h"
#include "input.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>

int sock, player_id;
volatile int game_running = 1;
char status_message[BUF_SIZE] = "";
char* localBoards;
int my_turn = 0;
pthread_mutex_t turn_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t turn_cond = PTHREAD_COND_INITIALIZER;
int current_turn_player = -1;
time_t turn_end_time = 0;
int random_shot = 0;

int g_players, g_height, g_width;

int shipHeights[] = {3,2,1,2,2};
int shipWidths[]  = {1,1,3,1,2};

char status_msgs[STATUS_MAX][BUF_SIZE];
int status_msg_count = 0;

void handle_signal() {
    sendMsg(sock, "Quitting game");
    if (sock > 0) {
        close(sock);
    }
    _exit(0);
}

void init_local_boards(int n, int h, int w) {
    g_players = n;
    g_height = h;
    g_width = w;

    localBoards = (char*)malloc(sizeof(char*) * n * h * w);
    if (!localBoards) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    memset(localBoards, 0, sizeof(char*) * n * h * w);
}

void update_board_state(int target_id, int x, int y, int status) {
    if (target_id >= 0 && target_id < g_players && x >= 0 && x < g_width && y >= 0 && y < g_height) {
        localBoards[BOARD_INDEX(target_id, y, x, g_height, g_width)] = status;
    }
}

void* timer_thread() {
    while (game_running) {
        sleep(1);

        pthread_mutex_lock(&turn_mutex);
        int redraw_timer = (current_turn_player != -1);
        pthread_mutex_unlock(&turn_mutex);

        if (redraw_timer) {
            render_header(player_id, current_turn_player, turn_end_time);
        }
    }
    return NULL;
}

int handle_init() {
    static int currentShipID = 0;
    int x = 0, y = 0;
    char msg[BUF_SIZE];
    char buffer[BUF_SIZE];

    int h = shipHeights[currentShipID];
    int w = shipWidths[currentShipID];
    printf("\n--- Placing Ship Type %d Height: %d Width: %d) ---\n", currentShipID, h, w);

    printf("Enter the X coordinates of your ship:\n");
    if (scanf("%d", &x) != 1) {
        printf("Invalid input\n");
        while (getchar() != '\n');
        return 0;
    }

    printf("Enter the Y coordinates of your ship:\n");
    if (scanf("%d", &y) != 1) {
        printf("Invalid input\n");
        while (getchar() != '\n');
        return 0;
    }

    snprintf(msg, BUF_SIZE, "%d %d %d %d", x, y, h, w);
    sendMsg(sock, msg);

    memset(buffer, 0, BUF_SIZE);
    recvBuff(sock, buffer, BUF_SIZE);

    if (strncmp(buffer, "OK", 2) == 0) {
        for (int i = 0; i < h; i++) {
            for (int j = 0; j < w; j++) {
                localBoards[BOARD_INDEX(player_id, y + i, x + j, g_height, g_width)] = 1;
            }
        }
        snprintf(status_message, BUF_SIZE, "Ship placed.");
        push_status(status_message);
        render_game(player_id, g_players, g_height, g_width, localBoards,
            current_turn_player, turn_end_time, status_msgs, status_msg_count);
        clear_status();
        currentShipID = (currentShipID + 1) % NUM_SHIP_TYPES;
        return 1;
    }
    printf("Server: %s\n", buffer);
    return 0;
}

void handle_shoot_action() {
    int target, x, y;

    printf("\n--- YOUR TURN ---\n");
    while (1) {
        printf("Enter Target Player ID: ");
        fflush(stdout);
        if (get_safe(&target, sock) <= 0) {
            return;
        }
        printf("Enter X Coordinate: ");
        fflush(stdout);
        if (get_safe(&x, sock) <= 0) {
            return;
        }
        printf("Enter Y Coordinate: ");
        fflush(stdout);
        if (get_safe(&y, sock) <= 0) {
            return;
        }

        if (x < 0 || y < 0 || x >= g_width || y >= g_height) {
            printf("Coordinates outside of board range\n");
            continue;
        }
        if (target == player_id || target < 0 || target >= g_players) {
            printf("Invalid target\n");
            continue;
        }
        char cell = localBoards[BOARD_INDEX(target, y, x, g_height, g_width)];

        if (cell != 0) {
            int flag = 0;
            printf("You have already fired on this position, are you sure you wish to fire here again?\n1.YES\n2.NO\n");

            if (scanf("%d", &flag) != 1) {
                printf("Invalid input\n");
                continue;
            }
            if (flag == 2) {
                continue;
            }
        }
        break;
    }
    char msg[BUF_SIZE];
    snprintf(msg, BUF_SIZE, "%d %d %d", target, x, y);
    sendMsg(sock, msg);
}

void* receive_thread() {
    char buffer[BUF_SIZE];
    int bytes;
    while (game_running && (bytes = recvBuff(sock, buffer, BUF_SIZE - 1)) > 0) {
        buffer[bytes] = '\0';
        if (strncmp(buffer, "START", 5) == 0) {
            print_message(player_id == 0 ? "Game started" : "Game Started! Waiting for Host...");
        } else if (strncmp(buffer, "INIT", 4) == 0) {
            snprintf(status_message, BUF_SIZE, "Setting up ships...");
            push_status(status_message);
            render_game(player_id, g_players, g_height, g_width, localBoards,
                current_turn_player, turn_end_time, status_msgs, status_msg_count);
            clear_status();
            if (player_id == 0) {
                while(!handle_init()) {

                }

                while (1) {
                    int choice = 0;
                    printf("1. Add another ship\n2. Finish placement\n");
                    if (scanf("%d", &choice) != 1) {
                        while(getchar() != '\n');
                    }

                    if (choice == 1) {
                        handle_init();
                    } else if (choice == 2) {
                        sendMsg(sock, "DONE");
                        char resp[BUF_SIZE];
                        recvBuff(sock, resp, BUF_SIZE);
                        if (strncmp(resp, "READY", 5) == 0) {
                            printf("Waiting for other players...\n");
                            break;
                        }
                    }
                }
            } else {
                int ships_needed = 0;
                if (sscanf(buffer, "INIT %d", &ships_needed) == 1) {
                    snprintf(status_message, BUF_SIZE, "Host placed %d ships. You must place %d ships.\n", ships_needed, ships_needed);
                    for (int i = 0; i < ships_needed; i++) {
                        snprintf(status_message, BUF_SIZE, "--- Placing Ship %d of %d ---\n", i + 1, ships_needed);
                        push_status(status_message);
                        while (!handle_init()) {
                            clear_status();
                        }
                    }

                    char resp[BUF_SIZE];
                    recvBuff(sock, resp, BUF_SIZE);
                    if (strncmp(resp, "READY", 5) == 0) {
                        printf("All ships placed! Game starting...\n");
                    }
                }
            }
        } else if (strncmp(buffer, "TURN", 4) == 0) {
            int player, seconds;
            if (sscanf(buffer, "TURN %d %d", &player, &seconds) == 2) {

                pthread_mutex_lock(&turn_mutex);

                current_turn_player = player;
                turn_end_time = time(NULL) + seconds;

                if (player == player_id) {
                    my_turn = 1;
                    pthread_cond_signal(&turn_cond);
                }

                render_game(player_id, g_players, g_height, g_width, localBoards,
                    current_turn_player, turn_end_time, status_msgs, status_msg_count);

                pthread_mutex_unlock(&turn_mutex);
            }
        } else if (strncmp(buffer, "HIT", 3) == 0) {
            int p_from, p_to, tx, ty;
            if(sscanf(buffer, "HIT %d %d %d %d", &p_from, &p_to, &tx, &ty) == 4) {
                update_board_state(p_to, tx, ty, 3);
                if (my_turn == 0 && random_shot == 0) {
                    clear_status();
                }
                snprintf(buffer, BUF_SIZE, "Player %d hit Player %d at (%d,%d).", p_from, p_to, tx, ty);
                push_status(buffer);
                random_shot = 0;
            }
        } else if (strncmp(buffer, "MISS", 4) == 0) {
            int p_from, p_to, tx, ty;
            if(sscanf(buffer, "MISS %d %d %d %d", &p_from, &p_to, &tx, &ty) == 4) {
                if (my_turn == 0 && random_shot == 0) {
                    clear_status();
                }
                update_board_state(p_to, tx, ty, 2);
                snprintf(buffer, BUF_SIZE, "Player %d missed Player %d at (%d,%d).", p_from, p_to, tx, ty);
                push_status(buffer);
                random_shot = 0;
            }
        } else if (strncmp(buffer, "ELIMINATED", 10) == 0) {
            int dead;
            sscanf(buffer, "ELIMINATED %d", &dead);
            snprintf(status_message, BUF_SIZE, "Player %d has been eliminated!", dead);
            push_status(status_message);
        } else if (strncmp(buffer, "WINNER", 6) == 0) {
            int win;
            sscanf(buffer, "WINNER %d", &win);
            render_game(player_id, g_players, g_height, g_width, localBoards,
                    current_turn_player, turn_end_time, status_msgs, status_msg_count);
            printf(GREEN "\n!!! Player %d WINS !!!\n" RESET, win);
            pthread_mutex_lock(&turn_mutex);
            game_running = 0;
            pthread_cond_broadcast(&turn_cond);
            pthread_mutex_unlock(&turn_mutex);
            break;
        } else if (strncmp(buffer, "ERROR", 5) == 0) {
            push_status(buffer);
        } else if (strncmp(buffer, "RANDOM SHOT", 11) == 0) {
            int shooter, target, x, y;
            if (sscanf(buffer, "RANDOM SHOT %d %d %d %d", &shooter, &target, &x, &y) == 4) {
                clear_status();
                snprintf(status_message, BUF_SIZE,
                    "Player %d timed out! Random shot at Player %d (%d,%d).", shooter, target, x, y);
                push_status(status_message);
                random_shot = 1;
            }
        }
    }
    game_running = 0;
    pthread_cond_broadcast(&turn_cond);
    return NULL;
}


void run_client(int port) {
    pthread_t recv_thread, timer;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return;
    }

    signal(SIGINT, handle_signal);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("Connection failed");
        close(sock);
        return;
    }

    char buffer[BUF_SIZE];
    int n = recvBuff(sock, buffer, BUF_SIZE - 1);
    if (n <= 0) {
        printf("Server closed connection.\n");
        close(sock);
        return;
    }
    buffer[n] = '\0';

    int temp_n, temp_h, temp_w;
    if (sscanf(buffer, "ID %d %d %d %d", &player_id, &temp_n, &temp_h, &temp_w) == 4) {
        printf("Game Config Received: %d Players, Map %dx%d\n", temp_n, temp_w, temp_h);
        init_local_boards(temp_n, temp_h, temp_w);
    } else {
        printf("Error: Invalid handshake from server.\n");
        close(sock);
        return;
    }

    printf("Connected as Player %d. Waiting for game start...\n", player_id);

    pthread_create(&recv_thread, NULL, receive_thread, NULL);
    pthread_create(&timer, NULL, timer_thread, NULL);

    while (game_running) {
        pthread_mutex_lock(&turn_mutex);
        while (my_turn == 0 && game_running) {
            pthread_cond_wait(&turn_cond, &turn_mutex);
        }

        if (game_running && my_turn) {
            pthread_mutex_unlock(&turn_mutex);
            render_game(player_id, g_players, g_height, g_width, localBoards,
                current_turn_player, turn_end_time,  status_msgs, status_msg_count);
            handle_shoot_action();
            status_message[0] = '\0';
            pthread_mutex_lock(&turn_mutex);
            my_turn = 0;
        }
        pthread_mutex_unlock(&turn_mutex);
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);

    pthread_join(timer, NULL);
    pthread_join(recv_thread, NULL);

    free(localBoards);
    localBoards = NULL;

    printf("Client exiting.\n");
}

void run_client_interactive() {
    int choice = 0;
    int port = 0;
    int num = 0, height = 0, width = 0;

    printf("Create a new game - 1\nJoin game - 2\n");
    while (1) {
        if (scanf("%d", &choice) != 1) {
            while (getchar() != '\n');
            printf("Invalid option, please type 1 or 2.\n");
            continue;
        }
        if (choice == 1) {
            port = get_port();
            get_init_params(&num, &height, &width);

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                return;
            }
            if (pid == 0) {
                run_server(port, num, height, width);
                exit(0);
            } else {
                printf("Starting client in 1 second...\n");
                sleep(1);
                run_client(port);
                printf("Client exited. Server continues running until game over.\n");
                break;
            }
        } else if (choice == 2) {
            port = get_port();
            run_client(port);
            break;
        } else {
            while (getchar() != '\n');
            printf("Invalid option, please type 1 or 2.\n");
        }
    }
}