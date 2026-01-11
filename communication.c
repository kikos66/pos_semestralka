#include "communication.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void sendMsg(int sockfd, char* msg) {
    char buffer[BUF_SIZE + 2];
    snprintf(buffer, sizeof(buffer), "%s\n", msg);
    write(sockfd, buffer, strlen(buffer));
}

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

void push_status(const char* msg) {
    if (status_msg_count < STATUS_MAX) {
        strncpy(status_msgs[status_msg_count++], msg, BUF_SIZE - 1);
    } else {
        for (int i = 1; i < STATUS_MAX; i++) {
            strcpy(status_msgs[i - 1], status_msgs[i]);
        }
        strncpy(status_msgs[STATUS_MAX - 1], msg, BUF_SIZE - 1);
    }
}

void clear_status() {
    for (int i = 0; i < status_msg_count; i++) {
        status_msgs[i][0] = '\0';
    }
    status_msg_count = 0;
}

