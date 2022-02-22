#define _POSIX_C_SOURCE 200809L

#pragma once

#include <stddef.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <pthread.h>

#define SOC_ADDRESS_MAX_LENGHT 108
#define SOC_DATAGRAM_MAX_LENGHT 1024

typedef enum {
	SOCK_STREAM,
	SOCK_DGRAM,
	SOCK_SEQPACKET
} sockType;

typedef int socket;

typedef struct {
	socket sockId;
	void* data;
	size_t size;
	int actualSize;
	int isReady;
	pthread_mutex_t mutex;
	pthread_cond_t cv;
	pthread_t thread;
} future;

// Initializes Sockets library 
// Needs to be called at the begining of the main function 
int soc_init();

socket soc_open(sockType type);
int soc_close(socket);

int soc_bind(socket, const char* addr);
int soc_listen(socket);
socket soc_accept(socket);

int soc_connect(socket, const char* addr);

int soc_recv(socket, void* buf, size_t size);
int soc_send(socket, void* buf, size_t size);
int soc_sendTo(socket, void* buf, size_t size, const char* addr);

future* soc_recvAsync(socket, size_t size);
int soc_sendAsync(socket, void* buf, size_t size);
int soc_sendToAsync(socket, void* buf, size_t size, const char* addr);

// Awaits future ready and copies data into buf
// Also destroes the future
int soc_futureGet(future*, void* buf);
int soc_futureIsReady(future*);
