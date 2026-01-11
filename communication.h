#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#define BUF_SIZE 256

#define STATUS_MAX 5

extern char status_msgs[STATUS_MAX][BUF_SIZE];
extern int status_msg_count;

void sendMsg(int sockfd, char* msg);
int recvBuff(int sockfd, char* buff, int size);
void push_status(const char *msg);
void clear_status();

#endif //COMMUNICATION_H
