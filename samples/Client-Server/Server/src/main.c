#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Sockets.h"

#define SOCK_ADDRESS "/tmp/Lab1/Server"

#define BUF_SIZE 10

#define FAIL_FAST_IF_ERROR(res, loc) {if (res < 0) {fprintf(stderr, "%s: %d\n",loc, res);exit(-1);}}

int main() {
	char buffer[BUF_SIZE];
	char reply[] = "Reply from the server!";

	unlink(SOCK_ADDRESS);

	FAIL_FAST_IF_ERROR(soc_init(), "init");

	socket sock = soc_open(SOCK_STREAM);
	FAIL_FAST_IF_ERROR(sock, "open");
	int res = soc_bind(sock, SOCK_ADDRESS);
	FAIL_FAST_IF_ERROR(res, "bind");
	res = soc_listen(sock);
	FAIL_FAST_IF_ERROR(res, "listen");

	while (1) {
		printf("[Server]: Listening\n");
		socket con = soc_accept(sock);
		FAIL_FAST_IF_ERROR(con, "accept");
		printf("[Server]: Connected\n");
		int count = 0;
		while (1) {
			count = soc_recv(con, buffer, BUF_SIZE);
			if (count < 0)
				break;
			if (count > 0) {
				printf("[Server]: Recieved message: ");
				fwrite(buffer, (size_t)count, 1, stdout);
				printf("\n");
				soc_send(con, reply, 23);
			}
			sleep(1);
		}
		soc_close(con);
		printf("[Server]: Disconnected\n");
	}

	soc_close(sock);

	return 0;
}
