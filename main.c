#include <unistd.h>

#include "server.h"
#include "client.h"

int main(void) {
    run_server(1024, 2, 4, 4);
    sleep(2);
    run_client(1024);
    return 0;
}