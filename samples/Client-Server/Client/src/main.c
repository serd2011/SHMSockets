#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Sockets.h"

#define SOCK_ADDRESS	"/tmp/Lab1/Client"
#define SERVER_ADDRESS	"/tmp/Lab1/Server"
#define BUF_SIZE 1000

#define FAIL_FAST_IF_ERROR(res, loc) {if (res < 0) {fprintf(stderr, "%s: %d\n",loc, res);exit(-1);}}

int main() {

	char buffer[BUF_SIZE];
	char message[] = "Message from the Client!";

	FAIL_FAST_IF_ERROR(soc_init(), "init");

	unlink(SOCK_ADDRESS);

	socket sock = soc_open(SOCK_STREAM);
	FAIL_FAST_IF_ERROR(sock, "open");
	int res = soc_bind(sock, SOCK_ADDRESS);
	FAIL_FAST_IF_ERROR(res, "bind");
	printf("[Client] Connecting...\n");
	res = soc_connect(sock, SERVER_ADDRESS);
	FAIL_FAST_IF_ERROR(res, "connect");
	printf("[Client] Connected! Sending data...\n");
	soc_send(sock, message, 25);
	printf("[Client] Sent\n");
	sleep(10);
	printf("[Client] Recieving reply...\n");
	int count = soc_recv(sock, buffer, BUF_SIZE);
	printf("[Client] Reply from the server: ");
	fwrite(buffer, (size_t)count, 1, stdout);
	printf("\n");

	soc_close(sock);

	return 0;
}
