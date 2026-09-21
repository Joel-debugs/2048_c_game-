CC=gcc
CFLAGS=-Wall -Wextra -std=c11

all: server tests

server:
	$(CC) $(CFLAGS) backend/game.c backend/server.c -o backend/2048_server.exe -lws2_32

tests:
	$(CC) $(CFLAGS) backend/test_game.c backend/game.c -o backend/test_game.exe

clean:
	rm -f backend/2048_server.exe backend/test_game.exe
