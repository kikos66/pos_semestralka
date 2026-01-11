CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
OBJ = main.o server.o client.o game.o interface.o communication.o input.o

all: app server	client

app: $(OBJ)
	$(CC) $(CFLAGS) -o app $(OBJ)

server: app
	cp app server

client: app
	cp app client

main.o: main.c server.h client.h
	$(CC) $(CFLAGS) -c main.c

server.o: server.c server.h game.h communication.h
	$(CC) $(CFLAGS) -c server.c

client.o: client.c client.h interface.h communication.h input.h
	$(CC) $(CFLAGS) -c client.c

interface.o: interface.c interface.h
	$(CC) $(CFLAGS) -c interface.c

game.o: game.c game.h
	$(CC) $(CFLAGS) -c game.c

communication.o: communication.c communication.h
	$(CC) $(CFLAGS) -c communication.c

input.o: input.c input.h
	$(CC) $(CFLAGS) -c input.c

clean:
	rm -f *.o app server client