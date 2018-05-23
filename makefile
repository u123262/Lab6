CC=gcc

all: client server

client : client.o libutils.so
	$(CC) -pthread client.o -o client -L. -lutils

server : server.o libutils.so
	$(CC) -pthread server.o -o server -L. -lutils

client.o : client.c
	$(CC) -c client.c -o client.o

server.o : server.c
	$(CC) -c server.c -o server.o

libutils.so : utils.o
	$(CC) -shared utils.o -o libutils.so

utils.o : utils.c
	$(CC) -fPIC -c utils.c

clean :
	rm client client.o libutils.so utils.o server server.o