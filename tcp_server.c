#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static int get_handler_fd(const char *handler_path)
{
	int handler = open(handler_path, O_PATH);
	if(0 > handler)
		err(1, "invalid handler program \"%s\"", handler_path);
	return handler;
}

static int setup_socket(void)
{
	struct sockaddr_in bind_addr =
	{
		.sin_family = AF_INET,
		.sin_port = htons(8080),
		.sin_addr = { .s_addr = htonl(INADDR_ANY), },
	};
	int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(0 > socket_fd)
		err(1, "unable to open socket");
#ifdef DEBUG
	static const int one = 1;
	if(0 > setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one))
		err(1, "unable to set re-use flag on socket");
#endif
	if(0 > bind(socket_fd, (const struct sockaddr *)&bind_addr, sizeof bind_addr))
		err(1, "unable to bind to address");
	if(0 > listen(socket_fd, 32))
		err(1, "unable to listen for connections");
	return socket_fd;
}

static void setup_signal_handler(void)
{
	struct sigaction child_act;
	if(0 > sigaction(SIGCHLD, NULL, &child_act))
		err(1, "failed to get default signal action for SIGCHLD (this is a bug)");
	child_act.sa_flags |= SA_NOCLDWAIT; //avoid needing to reap children processes
	if(0 > sigaction(SIGCHLD, &child_act, NULL))
		err(1, "failed to set signal action for SIGCHLD (this is a bug)");
}

static void setup_chroot(const char *chroot_path)
{
	//avoid a race condition by opening path once
	//then using resulting fd for all subsequent actions
	int chroot_dir = open(chroot_path, O_PATH);
	if(0 > chroot_dir)
		err(1, "invalid chroot directory \"%s\"", chroot_path);
	if(0 > fchdir(chroot_dir))
		err(1, "unable to chdir into chroot directory \"%s\"", chroot_path);
	char proc_path[64];
	int size = snprintf(proc_path, sizeof proc_path, "/proc/self/fd/%d", chroot_dir);
	if(size < 0 || sizeof proc_path <= (size_t)size || 0 > chroot(proc_path))
		err(1, "unable to chroot into chroot directory \"%s\"", chroot_path);
}

static void accept_connection(int socket_fd, int handler_fd)
{
	int client_socket_fd = accept4(socket_fd, NULL, NULL, SOCK_CLOEXEC);
	if(0 > client_socket_fd)
		err(1, "client accept failed");
	switch(fork())
	{
	case -1:
		err(1, "failed to create child for request");
	default:
		close(client_socket_fd);
		return;
	case 0:
		close(socket_fd);
		dup2(client_socket_fd, STDIN_FILENO);
		dup2(client_socket_fd, STDOUT_FILENO);
		fexecve(handler_fd, (char *[]){NULL}, environ);
		err(1, "failed to execute handler for request");
	}
}

int main(int argc, char **argv)
{
	if(argc < 2 || argc > 3)
		errx(1, "Usage: %s connection_handler_program [chroot_dir]", argv[0]);
	int handler_fd = get_handler_fd(argv[1]);

	if(argc == 3)
		setup_chroot(argv[2]);

	setup_signal_handler();

	int socket_fd = setup_socket();

	for(;;)
		accept_connection(socket_fd, handler_fd);
}
