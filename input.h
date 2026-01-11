#ifndef INPUT_H
#define INPUT_H

int get_port();
void get_init_params(int *n, int *h, int *w);
int get_safe(int *var, int sock_fd);

#endif //INPUT_H
