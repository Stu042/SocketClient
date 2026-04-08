#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/socketserver.sock"
#define INPUT_BUF_SIZE 1024

typedef struct {
	int fd;
	struct sockaddr_un addr;
} SocketClient;



static int client_init(SocketClient *client) {
	if (client == NULL) {
		return -1;
	}
	memset(client, 0, sizeof(*client));
	client->fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (client->fd < 0) {
		perror("socket");
		return -1;
	}
	memset(&client->addr, 0, sizeof(client->addr));
	client->addr.sun_family = AF_UNIX;
	strncpy(client->addr.sun_path, SOCKET_PATH, sizeof(client->addr.sun_path) - 1);
	if (connect(client->fd, (struct sockaddr *) &client->addr, sizeof(client->addr)) < 0) {
		perror("connect");
		close(client->fd);
		client->fd = -1;
		return -1;
	}
	return 0;
}

static int client_send(const SocketClient *client, const char *message) {
	if (client == NULL || client->fd < 0 || message == NULL) {
		return -1;
	}
	const size_t len = strlen(message);
	const ssize_t sent = send(client->fd, message, len, 0);
	if (sent < 0) {
		perror("send");
		return -1;
	}
	if ((size_t) sent != len) {
		fprintf(stderr, "partial send: %zd of %zu bytes\n", sent, len);
		return -1;
	}
	return 0;
}

static void client_cleanup(SocketClient *client) {
	if (client == NULL) {
		return;
	}
	if (client->fd >= 0) {
		close(client->fd);
		client->fd = -1;
	}
}

int main(const int argc, char *argv[]) {
	SocketClient client;
	char input[INPUT_BUF_SIZE];
	const char *message = NULL;
	if (argc > 1) {
		message = argv[1];
	} else {
		printf("Enter a single line: ");
		fflush(stdout);
		if (fgets(input, sizeof(input), stdin) == NULL) {
			fprintf(stderr, "failed to read input\n");
			return EXIT_FAILURE;
		}
		input[strcspn(input, "\n")] = '\0';
		message = input;
	}
	if (client_init(&client) != 0) {
		return EXIT_FAILURE;
	}
	if (client_send(&client, message) != 0) {
		client_cleanup(&client);
		return EXIT_FAILURE;
	}
	client_cleanup(&client);
	return EXIT_SUCCESS;
}
