CC = clang
CFLAGS = -std=c2x -Weverything -Wno-declaration-after-statement -D_GNU_SOURCE
CFLAGS += -DDEBUG -Og -g

.PHONY: all clean

all: tcp_server

tcp_server: tcp_server.c
	$(CC) $(CFLAGS) -o $@ $^
clean:
	-rm tcp_server
