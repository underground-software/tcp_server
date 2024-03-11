CC = clang
CFLAGS = -static -std=c2x -Weverything -Wno-declaration-after-statement -Wno-c++98-compat -Wno-padded \
	-Wno-unsafe-buffer-usage -Wno-disabled-macro-expansion -Wno-pre-c2x-compat -D_GNU_SOURCE
#CFLAGS += -DDEBUG -Og -g

.PHONY: all clean

all: tcp_server

tcp_server: tcp_server.c
	if hash clang; then                         \
		$(CC) $(CFLAGS) -o $@ $^;               \
	else                                        \
		podman build --tag tcp_server .;        \
		podman create --name tmp tcp_server;    \
		podman cp tmp:/tcp_server/tcp_server .; \
		podman rm tmp;                          \
	fi

clean:
	rm -f tcp_server
