#include "input.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/poll.h>

void clear_stdin() {
    while (getchar() != '\n');
}

int get_port() {
    int port = 0;
    printf("Enter port number:\n");
    while (1) {
        if (scanf("%d", &port) != 1) {
            clear_stdin();
            printf("Invalid input. Enter port number:\n");
            continue;
        }
        if (port >= 1024 && port <= 65535) {
            return port;
        }
        clear_stdin();
        printf("Port number is invalid (1024-65535):\n");
    }
}

void get_init_params(int *n, int *h, int *w) {
    while (1) {
        printf("Please enter number of players [2-10]:\n");
        if (scanf("%d", n) != 1 || *n < 2 || *n > 10) {
            printf("Invalid input.\n");
            clear_stdin();
            continue;
        }
        printf("Please enter height of your playing board [4-10]:\n");
        if (scanf("%d", h) != 1 || *h < 4 || *h > 10) {
            printf("Invalid input.\n");
            clear_stdin();
            continue;
        }
        printf("Please enter width of your playing board [4-10]:\n");
        if (scanf("%d", w) != 1 || *w < 4 || *w > 10) {
            printf("Invalid input.\n");
            clear_stdin();
            continue;
        }
        break;
    }
}

int get_safe(int *var, int sock_fd) {
    struct pollfd fds[2];
    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = sock_fd;
    fds[1].events = POLLIN;

    while (1) {
        int ret = poll(fds, 2, -1);
        if (ret < 0) {
            return 0;
        }

        if (fds[1].revents & POLLIN) {
            return -1;
        }

        if (fds[0].revents & POLLIN) {
            if (scanf("%d", var) == 1) {
                return 1;
            }
            clear_stdin();
            printf("Invalid. Try again: ");
            fflush(stdout);
        }
    }
}