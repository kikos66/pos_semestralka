#ifndef CLIENT_H
#define CLIENT_H

#define BUF_SIZE 256

#define RED "\x1b[31m"
#define GREEN "\x1b[32m"
#define BLUE "\x1b[34m"
#define RESET "\x1b[0m"
#define YELLOW "\x1b[33m"

#define NUM_SHIP_TYPES 1

void run_client(int port);
void run_client_interactive();

#endif //CLIENT_H
