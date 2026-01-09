#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#define BUF_SIZE 256

void sendMsg(int sockfd, char* msg);
int recvBuff(int sockfd, char* buff, int size);

#endif //COMMUNICATION_H
