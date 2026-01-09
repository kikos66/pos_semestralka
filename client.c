#include "client.h"
#include "communication.h"
#include "server.h"
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
int my_turn = 0;
char* localBoards;
pthread_mutex_t turn_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t turn_cond = PTHREAD_COND_INITIALIZER;

int g_players, g_height, g_width;
#define BOARD_INDEX(p, y, x) (((p) * g_height + (y)) * g_width + (x))

int shipHeights[] = {1};
int shipWidths[]  = {1};

void handle_signal() {
    sendMsg(STDOUT_FILENO, "Quitting game");
    if (sock > 0) {
        close(sock);
    }
    _exit(0);
}

int get_port_number_from_user() {
    int port = 0;
    int ok = 0;
    printf("Enter port number:\n");
    while (!ok) {
        if (scanf("%d", &port) != 1) {
            while (getchar() != '\n');
            printf("Invalid input. Enter port number:\n");
            continue;
        }
        if (port >= 1024 && port <= 65535) {
            ok = 1;
        } else {
            while (getchar() != '\n');
            printf("Port number is invalid, port must be in range 1024-65535:\n");
        }
    }
    return port;
}

void get_init_from_user(int *n, int *h, int *w) {
    while (1) {
        int height = 0;
        int width = 0;
        int num = 0;
        printf("Please enter number of players [2-10]:\n");
        if (scanf("%d", &num) != 1 || num < 2 || num > 10) {
            printf("Invalid input.\n");
            while (getchar() != '\n');
            continue;
        }

        printf("Please enter height of your playing board [4-10]:\n");
        if (scanf("%d", &height) != 1 || height < 4 || height > 10) {
            printf("Invalid input.\n");
            while (getchar() != '\n');
            continue;
        }

        printf("Please enter width of your playing board [4-10]:\n");
        if (scanf("%d", &width) != 1 || width < 4 || width > 10) {
            printf("Invalid input.\n");
            while (getchar() != '\n');
            continue;
        }
        *n = num;
        *h = height;
        *w = width;
        break;
    }
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

void render_game() {
    printf("\033[2J\033[H"); //AI

    printf("=== BATTLESHIP : Player %d ===\n\n", player_id);

    printf("--- MY BOARD ---\n");
    printf("   ");
    for(int x = 0; x < g_width; x++) {
        printf("%d ", x);
    }
    printf("\n");

    for (int y = 0; y < g_height; y++) {
        printf("%d |", y);
        for (int x = 0; x < g_width; x++) {
            char cell = localBoards[BOARD_INDEX(player_id, y, x)];
            if (cell == 0) {
                printf(BLUE "~ " RESET);
            } else if (cell == 1) {
                printf(GREEN "S " RESET);
            } else if (cell == 2) {
                printf(YELLOW "O " RESET);
            } else if (cell == 3) {
                printf(RED "X " RESET);
            }
        }
        printf("|\n");
    }
    printf("\n");

    printf("--- OPPONENT BOARDS ---\n");
    for (int p = 0; p < g_players; p++) {
        if (p == player_id) {
            continue;
        }

        printf("Player %d:\n", p);
        printf("   ");
        for(int x = 0; x < g_width; x++) {
            printf("%d ", x);
        }
        printf("\n");

        for (int y = 0; y < g_height; y++) {
            printf("%d |", y);
            for (int x = 0; x < g_width; x++) {
                char cell = localBoards[BOARD_INDEX(p, y, x)];
                if (cell == 0 || cell == 1) {
                    printf(BLUE "~ " RESET);
                } else if (cell == 2) {
                    printf(YELLOW "O " RESET);
                } else if (cell == 3) {
                    printf(RED "X " RESET);
                }
            }
            printf("|\n");
        }
        printf("\n");
    }
    if (status_message[0] != '\0') {
        printf(RED "\n%s\n" RESET, status_message);
    }
    status_message[0] = '\0';
    fflush(stdout);
}

void update_board_state(int target_id, int x, int y, int status) {
    if (target_id >= 0 && target_id < g_players && x >= 0 && x < g_width && y >= 0 && y < g_height) {
        localBoards[BOARD_INDEX(target_id, y, x)] = status;
    }
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
                localBoards[BOARD_INDEX(player_id, y + i, x + j)] = 1;
            }
        }
        render_game();
        printf("Ship successfully placed\n");
        currentShipID = (currentShipID + 1) % NUM_SHIP_TYPES;
        return 1;
    }
    if (strncmp(buffer, "FULL", 4) == 0) {
        printf("There is already a ship on these coordinates\n");
        return 0;
    }
    if (strncmp(buffer, "INVALID", 7) == 0) {
        printf("Entered coordinates are outside the board range\n");
        return 0;
    }
    printf("Unknown server response\n");
    return 0;
}

void handle_shoot_action() {
    int target, x, y;

    printf("\n--- YOUR TURN ---\n");
    while (1) {
        printf("Enter Target Player ID: ");
        if (scanf("%d", &target) != 1) {
            continue;
        }
        printf("Enter X Coordinate: ");
        if (scanf("%d", &x) != 1) {
            continue;
        }
        printf("Enter Y Coordinate: ");
        if (scanf("%d", &y) != 1) {
            continue;
        }

        if (x < 0 || y < 0 || x >= g_width || y >= g_height) {
            printf("Coordinates outside of board range\n");
            continue;
        }
        if (target == player_id || target < 0 || target >= g_players) {
            printf("Invalid target\n");
            continue;
        }

        char cell = localBoards[BOARD_INDEX(target, y, x)];

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
            if (player_id == 0) {
                printf("Game started\n");
            } else {
                printf("\nGame Started! Waiting for Player 1 to set up the game...\n");
            }
        } else if (strncmp(buffer, "INIT", 4) == 0) {
            render_game();
            printf("[INFO] Setting up ships\n");

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
                    printf("Host placed %d ships. You must place %d ships.\n", ships_needed, ships_needed);

                    for (int i = 0; i < ships_needed; i++) {
                        printf("--- Placing Ship %d of %d ---\n", i + 1, ships_needed);
                        while (!handle_init());
                    }

                    char resp[BUF_SIZE];
                    recvBuff(sock, resp, BUF_SIZE);
                    if (strncmp(resp, "READY", 5) == 0) {
                        printf("All ships placed! Game starting...\n");
                    }
                }
            }
        } else if (strncmp(buffer, "YOUR TURN", 9) == 0) {
            pthread_mutex_lock(&turn_mutex);
            my_turn = 1;
            pthread_cond_signal(&turn_cond);
            pthread_mutex_unlock(&turn_mutex);
        } else if (strncmp(buffer, "HIT", 3) == 0) {
            int p_from, p_to, tx, ty;
            if(sscanf(buffer, "HIT %d %d %d %d", &p_from, &p_to, &tx, &ty) == 4) {
                update_board_state(p_to, tx, ty, 3);
                snprintf(buffer, BUF_SIZE, "Player %d hit Player %d at (%d,%d).", p_from, p_to, tx, ty);
                strncpy(status_message, buffer, BUF_SIZE - 1);
                if (p_from == player_id) {
                    render_game();
                }
            }
        } else if (strncmp(buffer, "MISS", 4) == 0) {
            int p_from, p_to, tx, ty;
            if(sscanf(buffer, "MISS %d %d %d %d", &p_from, &p_to, &tx, &ty) == 4) {
                update_board_state(p_to, tx, ty, 2);
                snprintf(buffer, BUF_SIZE, "Player %d missed Player %d at (%d,%d).", p_from, p_to, tx, ty);
                strncpy(status_message, buffer, BUF_SIZE - 1);
                if (p_from == player_id) {
                    render_game();
                }
            }
        } else if (strncmp(buffer, "ELIMINATED", 10) == 0) {
            int dead;
            sscanf(buffer, "ELIMINATED %d", &dead);
            printf(RED "\nPlayer %d Eliminated!\n" RESET, dead);
            if (dead == player_id) {
                printf("[ALERT] You have lost all your ships. You are out.\n");
            }
        } else if (strncmp(buffer, "WINNER", 6) == 0) {
            int win;
            sscanf(buffer, "WINNER %d", &win);
            printf(GREEN "\n!!! Player %d WINS !!!\n" RESET, win);
            pthread_mutex_lock(&turn_mutex);
            game_running = 0;
            pthread_cond_broadcast(&turn_cond);
            pthread_mutex_unlock(&turn_mutex);
            break;
        } else if (strncmp(buffer, "ERROR", 5) == 0) {
            strncpy(status_message, buffer, BUF_SIZE - 1);
        }
    }
    game_running = 0;
    pthread_cond_broadcast(&turn_cond);
    return NULL;
}


void run_client(int port) {
    pthread_t recv_thread;
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
        printf("Connected as Player %d.\n", player_id);
        printf("Game Config Received: %d Players, Map %dx%d\n", temp_n, temp_w, temp_h);

        init_local_boards(temp_n, temp_h, temp_w);
    } else {
        printf("Error: Invalid handshake from server.\n");
        close(sock);
        return;
    }

    printf("Connected as Player %d. Waiting for game start...\n", player_id);

    pthread_create(&recv_thread, NULL, receive_thread, NULL);

    while (game_running) {
        pthread_mutex_lock(&turn_mutex);
        while (my_turn == 0 && game_running) {
            pthread_cond_wait(&turn_cond, &turn_mutex);
        }

        if (game_running && my_turn) {
            pthread_mutex_unlock(&turn_mutex);
            render_game();
            handle_shoot_action();
            pthread_mutex_lock(&turn_mutex);
            my_turn = 0;
        }
        pthread_mutex_unlock(&turn_mutex);
    }

    shutdown(sock, SHUT_RDWR);
    close(sock);

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
            port = get_port_number_from_user();
            get_init_from_user(&num, &height, &width);

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
            port = get_port_number_from_user();
            run_client(port);
            break;
        } else {
            while (getchar() != '\n');
            printf("Invalid option, please type 1 or 2.\n");
        }
    }
}