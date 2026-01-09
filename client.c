#include "client.h"
#include "communication.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

int sock, player_id;
volatile int game_running = 1;
char status_message[BUF_SIZE] = "";
int my_turn = 0;
pthread_mutex_t turn_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t turn_cond = PTHREAD_COND_INITIALIZER;

int shipHeights[] = {1};
int shipWidths[]  = {1};

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
                snprintf(buffer, BUF_SIZE, "Player %d hit Player %d at (%d,%d).", p_from, p_to, tx, ty);
                strncpy(status_message, buffer, BUF_SIZE - 1);
            }
        } else if (strncmp(buffer, "MISS", 4) == 0) {
            int p_from, p_to, tx, ty;
            if(sscanf(buffer, "MISS %d %d %d %d", &p_from, &p_to, &tx, &ty) == 4) {
                snprintf(buffer, BUF_SIZE, "Player %d missed Player %d at (%d,%d).", p_from, p_to, tx, ty);
                strncpy(status_message, buffer, BUF_SIZE - 1);
            }
        } else if (strncmp(buffer, "ELIMINATED", 10) == 0) {
            int dead;
            sscanf(buffer, "ELIMINATED %d", &dead);
            printf("\nPlayer %d Eliminated!\n", dead);
            if (dead == player_id) {
                printf("[ALERT] You have lost all your ships. You are out.\n");
            }
        } else if (strncmp(buffer, "WINNER", 6) == 0) {
            int win;
            sscanf(buffer, "WINNER %d", &win);
            printf("\n!!! Player %d WINS !!!\n", win);
            game_running = 0;
            break;
        } else if (strncmp(buffer, "ERROR", 5) == 0) {
            strncpy(status_message, buffer, BUF_SIZE - 1);
        }
    }
    game_running = 0;
    return NULL;
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

    pthread_join(recv_thread, NULL);
    close(sock);
    printf("Client exiting.\n");
}