CC = gcc
CFLAGS = -Wall -Wextra -pthread -g
OBJ = main.o server.o client.o game.o communication.o

all: app server	client

app: $(OBJ)
	$(CC) $(CFLAGS) -o app $(OBJ)

server: app
	cp app server

client: app
	cp app client

main.o: main.c server.h client.h
	$(CC) $(CFLAGS) -c main.c

server.o: server.c server.h game.h
	$(CC) $(CFLAGS) -c server.c

client.o: client.c client.h
	$(CC) $(CFLAGS) -c client.c

game.o: game.c game.h
	$(CC) $(CFLAGS) -c game.c

communication.o: communication.c communication.h
	$(CC) $(CFLAGS) -c communication.c

clean:
	rm -f *.o app server client