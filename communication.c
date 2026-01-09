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
