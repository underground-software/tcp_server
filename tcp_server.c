#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

static const struct sockaddr_in bind_addr =
{
	.sin_family = AF_INET,
	.sin_port = 0x901F, //port 8080 in network byte order
	.sin_addr = {.s_addr = 0,}, //listen on all interfaces
};

#ifdef DEBUG
static const int one = 1;
#endif

static const char *get_handler(const char *handler_path)
{
	int handler = open(handler_path, O_PATH);
	if(0 > handler)
		err(1, "invalid handler program \"%s\"", handler_path);
	static char proc_path[64];
	int size = snprintf(proc_path, sizeof proc_path, "/proc/self/fd/%d", handler);
	if(size < 0 || sizeof proc_path <= (size_t)size)
		err(1, "handler proc_path too small (this is a bug)");
	return proc_path;
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

int main(int argc, char **argv)
{
	if(argc < 2 || argc > 3)
		errx(1, "Usage: %s connection_handler_program [chroot_dir]", argv[0]);
	const char *handler = get_handler(argv[1]);
	if(argc == 3)
		setup_chroot(argv[2]);
	int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(0 > socket_fd)
		err(1, "unable to open socket");
#ifdef DEBUG
	if(0 > setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one))
		err(1, "unable to set re-use flag on socket");
#endif
	if(0 > bind(socket_fd, (const struct sockaddr *)&bind_addr, sizeof bind_addr))
		err(1, "unable to bind to address");
	if(0 > listen(socket_fd, 32))
		err(1, "unable to listen for connections");

	setup_signal_handler();
	for(;;)
	{
		int client_socket_fd = accept4(socket_fd, NULL, NULL, SOCK_CLOEXEC);
		if(0 > socket_fd)
			err(1, "client accept failed");
		switch(fork())
		{
		case -1:
			err(1, "failed to create child for request");
		default:
			close(client_socket_fd);
			break;
		case 0:
			close(socket_fd);
			dup2(client_socket_fd, STDIN_FILENO);
			dup2(client_socket_fd, STDOUT_FILENO);
			execve(handler, (char*[]){NULL}, environ);
			err(1, "failed to execute handler for request");
		}
	}
}
