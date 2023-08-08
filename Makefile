CC = clang
CFLAGS = -std=c2x -Weverything -Wno-declaration-after-statement -Wno-c++98-compat -Wno-padded -Wno-unsafe-buffer-usage -D_GNU_SOURCE
#CFLAGS += -DDEBUG -Og -g

.PHONY: all clean

all: tcp_server

tcp_server: tcp_server.c
	$(CC) $(CFLAGS) -o $@ $^
clean:
	-rm tcp_server
