CC=gcc
CFLAGS=-Wall -pedantic-errors -Werror
LIBFLAGS=-lpthread -lncurses

all: chatclient

chatclient: chatclient.c chat_lib.c
	$(CC) -o $@ $^ $(CFLAGS) $(LIBFLAGS)


clean:
	rm -f chatclient
