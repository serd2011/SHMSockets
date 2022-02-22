#include "Sockets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "utils/Log.h"
#include "Errors.h"

#define PERMS 0666
#define GLOBAL_SOCKETS_PATH				"/tmp/Lab1"
#define GLOBAL_SOCKETS_KEY_FILE_NAME	"/tmp/Lab1/socket"
#define GLOBAL_SOCKETS_PROJECT_ID		'A'
#define SOCKETS_PROJECT_ID				'A'

#define CONNECTION_FILE_BEGIN			"/tmp/Lab1/conn_"
#define CONNECTION_FILE_BEGIN_LENGH		15
#define CONNECTION_FILE_MAX_LENGH		35
#define CONNECTION_QUEUE_SIZE			20
#define CONNECTION_ONE_WAY_BUF_SIZE		2048

#define ONLINE_SOCK_ARRAY_SIZE			64
#define ONLINE_SOCK_TIMEOUT_SEC			60
#define ONLINE_SOCK_TIMEOUT_SEC_DEV		10
#define ONLINE_SOCK_SIGNAL_OFFSET		0

#define SOCK_ARRAY_SIZE					1024
#define MAX_CON_PER_SOCK				64
#define CONNECTION_SIGNAL_OFFSET		1
#define CONNECTION_SIGNAL_MIN_OFFSET	2
#define CONNECTION_SIGNAL_MAX_OFFSET	11
#define CONNECTION_TIMEOUT_SEC			5

// Calculated signal's numbers
#define INT_ONL_SOCK_TIMER_SIGNAL	(SIGRTMIN + ONLINE_SOCK_SIGNAL_OFFSET)
#define INT_CONNECTION_SIGNAL		(SIGRTMIN + CONNECTION_SIGNAL_OFFSET)
#define INT_CONNECTION_SIGNAL_MIN	(SIGRTMIN + CONNECTION_SIGNAL_MIN_OFFSET)
#define INT_CONNECTION_SIGNAL_MAX	(SIGRTMIN + CONNECTION_SIGNAL_MAX_OFFSET)

#undef	M_CONCATENATE_
#undef	M_CONCATENATE
#undef	M_MAKE_STRING
#define M_CONCATENATE_(A, B) A ## B
#define M_CONCATENATE(A, B) M_CONCATENATE_(A,B)
#define M_MAKE_STRING(s) # s

#undef BUFFER_SIZE
#undef BUFFER_NAME
#define BUFFER_SIZE CONNECTION_ONE_WAY_BUF_SIZE
#define BUFFER_NAME conOneWayBuf
#include "utils/buf.h"

#undef T
#undef CAPACITY
#define T int
#define CAPACITY MAX_CON_PER_SOCK
#include "utils/StaticVector.h"

// We must define this union for semctl
union semun {
	int              val;    /* Value for SETVAL */
	struct semid_ds* buf;    /* Buffer for IPC_STAT, IPC_SET */
	unsigned short* array;  /* Array for GETALL, SETALL */
	struct seminfo* __buf;  /* Buffer for IPC_INFO
								(Linux-specific) */
};

typedef enum {
	SST_EMPTY,
	SST_BINDED,
	SST_LISTENING,
	SST_CONNECTED
} sockStatus;

typedef struct {
	sockType type;
	sockStatus status;
	struct bindInfo {
		int fd;
		key_t key;
		char* addr;
	} bind;
	union {
		union {
			struct {
				staticVector_int connections;
			} listening;
			struct infoConnected {
				struct connectInfo {
					int fd;
					char addr[CONNECTION_FILE_MAX_LENGH];
					key_t key;
				} connect;
				key_t addr;
				int semid;
				int shmid;
				void* shmData;
				conOneWayBuf* inBuf;
				conOneWayBuf* outBuf;
			} connected;
		} streamSeqPacket;
		struct {
			int fdCon;
			int semid;
			int shmid;
			key_t defaultSendAddr;
			conOneWayBuf* inBuf;
		} dgram;
	} info;
} sockInfo;

#undef T
#undef CAPACITY
#define T sockInfo
#define CAPACITY SOCK_ARRAY_SIZE
#include "utils/StaticArray.h"

typedef struct {
	key_t addr;
	pid_t pid;
	int sigNum;
} incomingConnection;

typedef struct {
	key_t addr;
	key_t key;
} acceptedConnectionInfo;

#undef T
#undef CAPACITY
#define T incomingConnection
#define CAPACITY CONNECTION_QUEUE_SIZE
#include "utils/StaticQueue.h"

#undef T
#undef CAPACITY
#define T acceptedConnectionInfo
#define CAPACITY CONNECTION_QUEUE_SIZE
#include "utils/StaticVector.h"

typedef struct {
	int fd;
	int shmid;
	int semid;
	pthread_t onlineSockUpdateThreadId;
	timer_t onlineSockUpdateTimerId;
} gs_info;

typedef struct {
	pid_t pid;
	key_t addrKey;
	sockType type;
	time_t lastUpdate;
	union {
		struct {
			staticQueue_incomingConnection connectionQueue;
			staticVector_acceptedConnectionInfo acceptedConnections;
		} streamSeqPacket;
		struct {
			key_t recvAddr;
		} dgram;
	} data;
} gs_onlSockInf;

#undef T
#undef CAPACITY
#define T gs_onlSockInf
#define CAPACITY ONLINE_SOCK_ARRAY_SIZE
#include "utils/StaticVector.h"

typedef struct {
	staticVector_gs_onlSockInf onlineSockets;
} gs_data;

// Global sockets header info
static gs_info info;
// Global sockets header data
static gs_data* gsData;

// Array that holds information about sockets (and connections)
static staticArray_sockInfo sockets;
static pthread_mutex_t socketsMutex;

// Mutex that if used than accepting connections to prevent simultaneous acception attempts from different threads.
// Required cuz we use one signal to confirm connection 
static pthread_mutex_t connectCreationMutex;

// Signal number that is gonna be used next by connecting sockets to allow multiple simultaneous connection waits
static int freeConnectionSignalNumsNextOffset = CONNECTION_SIGNAL_MIN_OFFSET;
static pthread_mutex_t freeConnectionSignalNumsMutex;

#define MUTEX_LOCK(mutex,val)	RETURN_LOG_IF_FAILED_MSG((errno = pthread_mutex_lock(mutex)), val, "MUTEX_LOCK")
#define MUTEX_UNLOCK(mutex,val)	RETURN_LOG_IF_FAILED_MSG((errno = pthread_mutex_unlock(mutex)), val, "MUTEX_UNLOCK")

typedef enum {
	SEM_LOCK,
	SEM_UNLOCK,
	SEM_WAIT_ZERO
} semAction;
#define SEM_LOCK(semid,val)		RETURN_LOG_IF_FAILED_MSG(semPerformAction(semid, 0, SEM_LOCK), val, "SEM_LOCK")
#define SEM_UNLOCK(semid,val)	RETURN_LOG_IF_FAILED_MSG(semPerformAction(semid, 0, SEM_UNLOCK), val, "SEM_UNLOCK")

// Preforms 'action' action on semaphore
int semPerformAction(int semid, unsigned short semnum, semAction action);
// Allocates and copies string src to dest ptr
int copyString(char** dest, const char* src);
// Routine that runs in a separete thread and updates 'lastUpdate' time in onlineSockets vector
void* timerThreadRoutine(void*);
// Routine that runs in a separete thread and recieves 
void* timerThreadRoutine(void*);
// Clears onlineSockets vector from outdated sockets.
// Semaphore needs to be already acquired
void tidyOnlineSocketsVector();
// Finds socket by addres in onlineSockets vector.
// Semaphore needs to be already acquired
gs_onlSockInf* findSocketInOnlineSockets(key_t addr);

typedef struct {
	socket sock;
	void* buf;
	size_t size;
	char* addr;
} sendAsyncData;

void* recvAsyncThreadRoutine(void* future);
void* sendAsyncThreadRoutine(void* data);


void onExit();

int soc_init() {
	atexit(onExit);

	int res;

	res = mkdir(GLOBAL_SOCKETS_PATH, 0770);
	RETURN_LOG_IF_MSG((res == -1 && errno != EEXIST), E_INT_ERROR, "soc_init.mkdir");
	info.fd = open(GLOBAL_SOCKETS_KEY_FILE_NAME, O_RDWR | O_CREAT | O_EXCL, PERMS);
	if (info.fd == -1) {
		if (errno != EEXIST)
			RETURN_LOG_MSG(E_INT_ERROR, "soc_init.open");
		info.fd = open(GLOBAL_SOCKETS_KEY_FILE_NAME, O_RDWR, PERMS);
		RETURN_LOG_IF_FAILED_MSG(info.fd, E_INT_ERROR, "soc_init.open2");
	}
	// Generating key for primary shared memory segment and it's semaphore
	key_t key = ftok(GLOBAL_SOCKETS_KEY_FILE_NAME, GLOBAL_SOCKETS_PROJECT_ID);
	RETURN_LOG_IF_FAILED_MSG(key, E_INT_ERROR, "soc_init.ftok");
	// Creating semaphore 
	info.semid = semget(key, 1, PERMS | IPC_CREAT | IPC_EXCL);
	if (info.semid == -1) {
		if (errno != EEXIST)
			RETURN_LOG_MSG(E_INT_ERROR, "soc_init.semget");
		info.semid = semget(key, 1, PERMS);
		RETURN_LOG_IF_FAILED_MSG(info.semid, E_INT_ERROR, "soc_init.semget");
	} else {
		SEM_UNLOCK(info.semid, E_INT_ERROR);
	}
	// Getting shared memory segment
	info.shmid = shmget(key, sizeof(gs_data), PERMS | IPC_CREAT | IPC_EXCL);
	if (info.shmid == -1) {
		if (errno != EEXIST)
			RETURN_LOG_MSG(E_INT_ERROR, "soc_init.shmget");
		info.shmid = shmget(key, 1, PERMS);
		RETURN_LOG_IF_FAILED_MSG(info.shmid, E_INT_ERROR, "soc_init.semget");
	}
	// Attaching shared memory segment
	gsData = (gs_data*)shmat(info.shmid, NULL, 0);
	RETURN_LOG_IF_MSG(gsData == (void*)-1, E_INT_ERROR, "soc_init.shmat");

	errno = pthread_mutex_init(&socketsMutex, NULL);
	RETURN_LOG_IF_MSG(errno != 0, E_INT_ERROR, "soc_init.pthread_mutex_init");
	errno = pthread_mutex_init(&connectCreationMutex, NULL);
	RETURN_LOG_IF_MSG(errno != 0, E_INT_ERROR, "soc_init.pthread_mutex_init2");
	errno = pthread_mutex_init(&freeConnectionSignalNumsMutex, NULL);
	RETURN_LOG_IF_MSG(errno != 0, E_INT_ERROR, "soc_init.pthread_mutex_init3");

	/* Timer setup for updating online status of the sock */

	sigset_t sigset;
	struct sigevent timeoutSev = {
		.sigev_notify = SIGEV_SIGNAL,
		.sigev_signo = INT_ONL_SOCK_TIMER_SIGNAL
	};
	struct itimerspec timeoutSpec = {
		.it_interval.tv_sec = ONLINE_SOCK_TIMEOUT_SEC,
		.it_value.tv_sec = ONLINE_SOCK_TIMEOUT_SEC
	};

	// Blocking signals
	res = sigemptyset(&sigset);
	RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_init.sigemptyset");
	res = sigaddset(&sigset, INT_ONL_SOCK_TIMER_SIGNAL);
	RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_init.sigaddset");
	res = sigaddset(&sigset, INT_CONNECTION_SIGNAL);
	RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_init.sigaddset2");
	for (int sigNum = INT_CONNECTION_SIGNAL_MIN; sigNum <= INT_CONNECTION_SIGNAL_MAX; sigNum++) {
		res = sigaddset(&sigset, sigNum);
		RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_init.sigaddset3");
	}
	errno = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	RETURN_LOG_IF_MSG(errno != 0, E_INT_ERROR, "soc_init.pthread_sigmask");
	// Creating timer
	res = timer_create(CLOCK_MONOTONIC, &timeoutSev, &info.onlineSockUpdateTimerId);
	RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_init.timer_create");
	res = timer_settime(info.onlineSockUpdateTimerId, 0, &timeoutSpec, NULL);
	RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_init.timer_settime");
	// Creating thread
	errno = pthread_create(&info.onlineSockUpdateThreadId, NULL, timerThreadRoutine, NULL);
	RETURN_LOG_IF_MSG(errno != 0, E_INT_ERROR, "soc_init.pthread_create");

	return 0;
}

socket soc_open(sockType type) {
	MUTEX_LOCK(&socketsMutex, E_INT_ERROR);
	sockInfo newSocket = {
		.type = type,
		.status = SST_EMPTY
	};
	int newSockId = sa_add_sockInfo(&sockets, &newSocket);
	MUTEX_UNLOCK(&socketsMutex, E_INT_ERROR);
	if (newSockId == -1)
		return E_NO_SPACE;
	return newSockId;
}

int soc_close(socket sockId) {
	if (sockId < 0)
		return E_ARG_INVAL;

	sockInfo* sock = sa_getPtr_sockInfo(&sockets, sockId);
	if (sock == NULL)
		return E_ARG_INVAL;

	if (sock->status == SST_EMPTY) {
		sa_remove_sockInfo(&sockets, sockId);
		return 0;
	}

	if (sock->type == SOCK_DGRAM || sock->status == SST_LISTENING) {
		SEM_LOCK(info.semid, E_INT_ERROR);
		for (size_t i = 0; i < sv_size_gs_onlSockInf(&gsData->onlineSockets); i++) {
			if (sv_getPtr_gs_onlSockInf(&gsData->onlineSockets, i)->addrKey == sock->bind.key) {
				sv_remove_gs_onlSockInf(&gsData->onlineSockets, i);
				break;
			}
		}
		SEM_UNLOCK(info.semid, E_INT_ERROR);
	}

	if (sock->type == SOCK_DGRAM) {
		shmdt(sock->info.dgram.inBuf);
		shmctl(sock->info.dgram.shmid, IPC_RMID, NULL);
		semctl(sock->info.dgram.semid, 0, IPC_RMID);
	}

	if (sock->status == SST_CONNECTED) {
		struct infoConnected* sockInfCon = &sock->info.streamSeqPacket.connected;
		shmdt(sockInfCon->shmData);
		shmctl(sockInfCon->shmid, IPC_RMID, NULL);
		semctl(sockInfCon->semid, 0, IPC_RMID);
		if (sockInfCon->connect.fd != -1) {
			close(sockInfCon->connect.fd);
			unlink(sockInfCon->connect.addr);
		}
	}

	if (sock->status == SST_LISTENING) {
		// closing all connection that socket has
		staticVector_int* vec = &sock->info.streamSeqPacket.listening.connections;
		for (size_t i = 0; i < sv_size_int(vec); i++) {
			soc_close(*sv_getPtr_int(vec, i));
		}
	}

	if (sock->status != SST_CONNECTED) {
		close(sock->bind.fd);
		unlink(sock->bind.addr);
		free(sock->bind.addr);
	}

	sa_remove_sockInfo(&sockets, sockId);
	return 0;
}

int soc_bind(socket sockId, const char* addr) {
	if (addr == NULL)
		return E_ARG_INVAL;

	sockInfo* sock = sa_getPtr_sockInfo(&sockets, (size_t)sockId);
	if (sock == NULL)
		return E_ARG_INVAL;
	if (sock->status != SST_EMPTY)
		return E_SOCK_BOUND;

	// Creating sock file and acquiring key
	sock->bind.fd = open(addr, O_RDWR | O_CREAT | O_EXCL, PERMS);
	if (sock->bind.fd == -1) {
		if (errno == EEXIST)
			return E_ADDR_TAKEN;
		RETURN_LOG_MSG(E_ADDR_INVAL, "soc_bind.open");
	}
	sock->bind.key = ftok(addr, SOCKETS_PROJECT_ID);
	RETURN_LOG_IF_FAILED_MSG(sock->bind.key, E_INT_ERROR, "soc_bind.ftok");
	int res = copyString(&(sock->bind.addr), addr);
	RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_bind.copyString");

	// If sock if of DGRAM type creating shm buffer for recieving dgrams
	if (sock->type == SOCK_DGRAM) {
		sock->info.dgram.defaultSendAddr = -1;
		// Creating semaphore 
		sock->info.dgram.semid = semget(sock->bind.key, 1, PERMS | IPC_CREAT);
		RETURN_LOG_IF_FAILED_MSG(sock->info.dgram.semid, E_INT_ERROR, "soc_bind.semget");
		// Getting shared memory segment
		sock->info.dgram.shmid = shmget(sock->bind.key, sizeof(conOneWayBuf), PERMS | IPC_CREAT);
		RETURN_LOG_IF_FAILED_MSG(sock->info.dgram.shmid, E_INT_ERROR, "soc_bind.shmget");
		// Attaching shared memory segment
		sock->info.dgram.inBuf = (conOneWayBuf*)shmat(sock->info.dgram.shmid, NULL, 0);
		RETURN_LOG_IF_MSG(sock->info.dgram.inBuf == (void*)-1, E_INT_ERROR, "soc_bind.shmat");
		b_clear_conOneWayBuf(sock->info.dgram.inBuf);

		SEM_LOCK(info.semid, E_INT_ERROR);
		tidyOnlineSocketsVector();

		gs_onlSockInf newSockInfo = {
			.pid = getpid(),
			.addrKey = sock->bind.key,
			.type = sock->type,
			.lastUpdate = time(NULL),
			.data.dgram.recvAddr = -1,
		};
		res = sv_add_gs_onlSockInf(&(gsData->onlineSockets), &newSockInfo);
		if (res != 0) {
			SEM_UNLOCK(info.semid, E_INT_ERROR);
			return E_NO_SPACE;
		}

		SEM_UNLOCK(info.semid, E_INT_ERROR);
	}

	sock->status = SST_BINDED;
	return 0;
}

int soc_listen(socket sockId) {
	sockInfo* sock = sa_getPtr_sockInfo(&sockets, sockId);
	if (sock == NULL)
		return E_ARG_INVAL;
	if (sock->type == SOCK_DGRAM)
		return E_SOCK_OP_NOT_SUPP;
	if (sock->status == SST_EMPTY)
		return E_SOCK_NOT_BOUND;
	if (sock->status != SST_BINDED)
		return E_ARG_INVAL;

	// Adding socket into online sockets vector so that it can be seen
	SEM_LOCK(info.semid, E_INT_ERROR);
	tidyOnlineSocketsVector();
	for (size_t i = 0; i < sv_size_gs_onlSockInf(&(gsData->onlineSockets)); i++) {
		if (sv_getPtr_gs_onlSockInf(&(gsData->onlineSockets), i)->addrKey == sock->bind.key) {
			sv_remove_gs_onlSockInf(&(gsData->onlineSockets), i);
			break;
		}
	}
	gs_onlSockInf newSockInfo = {
			.pid = getpid(),
			.addrKey = sock->bind.key,
			.type = sock->type,
			.lastUpdate = time(NULL)
	};
	int res = sv_add_gs_onlSockInf(&(gsData->onlineSockets), &newSockInfo);
	SEM_UNLOCK(info.semid, E_INT_ERROR);
	if (res != 0)
		return E_NO_SPACE;

	sock->status = SST_LISTENING;
	return 0;
}

socket soc_accept(socket sockId) {
	sockInfo* sock = sa_getPtr_sockInfo(&sockets, sockId);
	if (sock == NULL)
		return E_ARG_INVAL;
	if (sock->type == SOCK_DGRAM)
		return E_SOCK_OP_NOT_SUPP;
	if (sock->status == SST_CONNECTED)
		return E_ARG_INVAL;
	if (sock->status != SST_LISTENING)
		return E_SOCK_NOT_LIST;

	// Awaiting connection requests
	SEM_LOCK(info.semid, E_INT_ERROR);
	gs_onlSockInf* sockInf = findSocketInOnlineSockets(sock->bind.key);
	if (sockInf == NULL) {
		SEM_UNLOCK(info.semid, E_INT_ERROR);
		return E_INT_ERROR;
	}
	while (sq_size_incomingConnection(&sockInf->data.streamSeqPacket.connectionQueue) == 0) {
		SEM_UNLOCK(info.semid, E_INT_ERROR);
		int res = semPerformAction(info.semid, 0, SEM_WAIT_ZERO);
		RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_accept.semPerformAction");
		SEM_LOCK(info.semid, E_INT_ERROR);
		sockInf = findSocketInOnlineSockets(sock->bind.key);
		if (sockInf == NULL) {
			SEM_UNLOCK(info.semid, E_INT_ERROR);
			return E_INT_ERROR;
		}
	}
	incomingConnection incCon = *sq_popPtr_incomingConnection(&sockInf->data.streamSeqPacket.connectionQueue);
	SEM_UNLOCK(info.semid, E_INT_ERROR);

	// Creating new connection socket

	MUTEX_LOCK(&socketsMutex, E_INT_ERROR);
	sockInfo newConTmp = {
		.type = sock->type,
		.status = SST_CONNECTED,
		.bind.key = sock->bind.key,
		.info.streamSeqPacket.connected.addr = incCon.addr
	};
	int newId = sa_add_sockInfo(&sockets, &newConTmp);
	MUTEX_UNLOCK(&socketsMutex, E_INT_ERROR);
	if (newId == -1) {
		return E_NO_SPACE;
	}
	sv_add_int(&sock->info.streamSeqPacket.listening.connections, &newId);
	sockInfo* newCon = sa_getPtr_sockInfo(&sockets, newId);

	struct infoConnected* newConInfCon = &newCon->info.streamSeqPacket.connected;

	// Creating connection file and acquiring key
	time_t curTime = time(NULL);
	memcpy(newConInfCon->connect.addr, CONNECTION_FILE_BEGIN, CONNECTION_FILE_BEGIN_LENGH);
	int res = snprintf((newConInfCon->connect.addr + CONNECTION_FILE_BEGIN_LENGH), (CONNECTION_FILE_MAX_LENGH - CONNECTION_FILE_BEGIN_LENGH), "%lu", curTime);
	RETURN_LOG_IF_MSG(res < 0, E_INT_ERROR, "soc_accept.snprintf");
	newConInfCon->connect.fd = open(newConInfCon->connect.addr, O_RDWR | O_CREAT, PERMS);
	RETURN_LOG_IF_FAILED_MSG(newConInfCon->connect.fd, E_INT_ERROR, "soc_accept.open");
	newConInfCon->connect.key = ftok(newConInfCon->connect.addr, SOCKETS_PROJECT_ID);
	RETURN_LOG_IF_FAILED_MSG(newConInfCon->connect.key, E_INT_ERROR, "soc_accept.ftok");

	// Creating shm and sem
	newConInfCon->semid = semget(newConInfCon->connect.key, 1, PERMS | IPC_CREAT);
	RETURN_LOG_IF_FAILED_MSG(newConInfCon->semid, E_INT_ERROR, "soc_accept.semget");
	SEM_UNLOCK(newConInfCon->semid, E_INT_ERROR);
	newConInfCon->shmid = shmget(newConInfCon->connect.key, sizeof(conOneWayBuf) * 2, PERMS | IPC_CREAT);
	RETURN_LOG_IF_FAILED_MSG(newConInfCon->shmid, E_INT_ERROR, "soc_accept.shmget");
	newConInfCon->shmData = shmat(newConInfCon->shmid, NULL, 0);
	RETURN_LOG_IF_MSG(newConInfCon->shmData == (void*)-1, E_INT_ERROR, "soc_accept.shmat");
	newConInfCon->inBuf = newConInfCon->shmData;
	newConInfCon->outBuf = newConInfCon->inBuf + 1;
	b_clear_conOneWayBuf(newConInfCon->inBuf);
	b_clear_conOneWayBuf(newConInfCon->outBuf);

	// Letting other side know that we are ready

	acceptedConnectionInfo conInf = {
		.addr = newConInfCon->addr,
		.key = newConInfCon->connect.key
	};
	SEM_LOCK(info.semid, E_INT_ERROR);
	sockInf = findSocketInOnlineSockets(sock->bind.key);
	if (sockInf == NULL) {
		SEM_UNLOCK(info.semid, E_INT_ERROR);
		return E_INT_ERROR;
	}
	res = sv_add_acceptedConnectionInfo(&sockInf->data.streamSeqPacket.acceptedConnections, &conInf);
	SEM_UNLOCK(info.semid, E_INT_ERROR);
	if (res == -1)
		return E_NO_SPACE;

	sigset_t sigset;
	res = sigemptyset(&sigset);
	FAIL_FAST_IF_FAILED_MSG(res, "soc_accept.sigemptyset");
	res = sigaddset(&sigset, INT_CONNECTION_SIGNAL);
	FAIL_FAST_IF_FAILED_MSG(res, "soc_accept.sigaddset");
	siginfo_t siginfo;
	struct timespec timeoutSpec = {
		.tv_sec = CONNECTION_TIMEOUT_SEC
	};

	MUTEX_LOCK(&connectCreationMutex, E_INT_ERROR);
	res = kill(incCon.pid, incCon.sigNum);
	if (res == -1) {
		res = errno;
		MUTEX_UNLOCK(&connectCreationMutex, E_INT_ERROR);
		errno = res;
		RETURN_LOG_MSG(E_INT_ERROR, "soc_accept.kill");
	}

	// Awaiting response
	res = sigtimedwait(&sigset, &siginfo, &timeoutSpec);
	int tmpErrno = errno;
	MUTEX_UNLOCK(&connectCreationMutex, E_INT_ERROR);
	errno = tmpErrno;
	RETURN_LOG_IF_MSG((res == -1 && errno != EAGAIN), E_INT_ERROR, "soc_accept.sigtimedwait");

	if (res != -1)
		return newId; // Connected successfully

	RETURN_LOG_IF_MSG(errno != EAGAIN, E_INT_ERROR, "sigtimedwait");

	// Other side didn't acknowledge in time

	// Removing accepted connection from vector
	SEM_LOCK(info.semid, E_INT_ERROR);
	sockInf = findSocketInOnlineSockets(sock->bind.key);
	if (sockInf == NULL) {
		SEM_UNLOCK(info.semid, E_INT_ERROR);
		return E_INT_ERROR;
	}
	staticVector_acceptedConnectionInfo* vec = &sockInf->data.streamSeqPacket.acceptedConnections;
	for (size_t i = 0; i < sv_size_acceptedConnectionInfo(vec); i++) {
		if (sv_getPtr_acceptedConnectionInfo(vec, i)->key == conInf.key) {
			sv_remove_acceptedConnectionInfo(vec, i);
			break;
		}
	}
	SEM_UNLOCK(info.semid, E_INT_ERROR);
	// Removing shm and sem
	res = shmdt(newConInfCon->shmData);
	RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_accept.shmdt");
	res = shmctl(newConInfCon->shmid, IPC_RMID, NULL);
	RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_accept.shmctl");
	res = semctl(newConInfCon->semid, 0, IPC_RMID);
	RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_accept.semctl");
	sa_remove_sockInfo(&sockets, (size_t)sockId);

	close(newConInfCon->connect.fd);
	unlink(newConInfCon->connect.addr);

	return E_CON_ABORTED;

}

int soc_connect(socket sockId, const char* addr) {
	if (addr == NULL)
		return E_ARG_INVAL;
	sockInfo* sock = sa_getPtr_sockInfo(&sockets, sockId);
	if (sock == NULL)
		return E_ARG_INVAL;
	if (sock->status == SST_EMPTY)
		return E_SOCK_NOT_BOUND;
	if (sock->status == SST_LISTENING)
		return E_SOCK_OP_NOT_SUPP;
	if (sock->status == SST_CONNECTED)
		return E_SOCK_ALR_CON;

	key_t conKey = ftok(addr, SOCKETS_PROJECT_ID);
	RETURN_LOG_IF_FAILED_MSG(conKey, E_ADDR_INVAL, "soc_connect.ftok");

	if (sock->type == SOCK_DGRAM) {
		SEM_LOCK(info.semid, E_INT_ERROR);
		gs_onlSockInf* sockInf = findSocketInOnlineSockets(sock->bind.key);
		if (sockInf == NULL) {
			SEM_UNLOCK(info.semid, E_INT_ERROR);
			return E_INT_ERROR;
		}
		sock->info.dgram.defaultSendAddr = conKey;
		sockInf->data.dgram.recvAddr = conKey;
		SEM_UNLOCK(info.semid, E_INT_ERROR);
		return 0;
	}

	// Sock supports connections

	SEM_LOCK(info.semid, E_INT_ERROR);

	gs_onlSockInf* sockInf = findSocketInOnlineSockets(conKey);
	if (sockInf == NULL) {
		SEM_UNLOCK(info.semid, E_INT_ERROR);
		return E_INT_ERROR;
	}

	if (sockInf->type != sock->type) {
		SEM_UNLOCK(info.semid, E_INT_ERROR);
		return E_ENDP_NOT_CON;
	}

	// Pushing connection request into queue

	MUTEX_LOCK(&freeConnectionSignalNumsMutex, E_INT_ERROR);
	int sigNum = SIGRTMIN + freeConnectionSignalNumsNextOffset;
	freeConnectionSignalNumsNextOffset++;
	if (freeConnectionSignalNumsNextOffset > INT_CONNECTION_SIGNAL_MAX)
		freeConnectionSignalNumsNextOffset = INT_CONNECTION_SIGNAL_MIN;
	MUTEX_UNLOCK(&freeConnectionSignalNumsMutex, E_INT_ERROR);

	incomingConnection incCon = {
		.addr = sock->bind.key,
		.pid = getpid(),
		.sigNum = sigNum
	};
	int res = sq_push_incomingConnection(&sockInf->data.streamSeqPacket.connectionQueue, &incCon);
	if (res == -1) {
		SEM_UNLOCK(info.semid, E_INT_ERROR);
		return E_NO_SPACE;
	}

	sigset_t sigset;
	res = sigemptyset(&sigset);
	FAIL_FAST_IF_FAILED_MSG(res, "soc_connect.sigemptyset");
	res = sigaddset(&sigset, incCon.sigNum);
	FAIL_FAST_IF_FAILED_MSG(res, "soc_connect.sigaddset");
	siginfo_t siginfo;
	struct timespec timeoutSpec = {
		.tv_sec = CONNECTION_TIMEOUT_SEC
	};

	SEM_UNLOCK(info.semid, E_INT_ERROR);

	// Awaiting connection accept
	res = sigtimedwait(&sigset, &siginfo, &timeoutSpec);
	if (res == -1) {
		RETURN_LOG_IF_MSG(errno != EAGAIN, E_INT_ERROR, "sigtimedwait");
		return E_CON_TIMED_OUT;
	}

	SEM_LOCK(info.semid, E_INT_ERROR);
	sockInf = findSocketInOnlineSockets(conKey);
	if (sockInf == NULL) {
		SEM_UNLOCK(info.semid, E_INT_ERROR);
		return E_INT_ERROR;
	}
	pid_t pid = sockInf->pid;
	key_t key = -1;
	for (size_t i = 0; i < sv_size_acceptedConnectionInfo(&sockInf->data.streamSeqPacket.acceptedConnections); i++) {
		acceptedConnectionInfo* accCon = sv_getPtr_acceptedConnectionInfo(&sockInf->data.streamSeqPacket.acceptedConnections, i);
		if (accCon->addr == sock->bind.key) {
			key = accCon->key;
			sv_remove_acceptedConnectionInfo(&sockInf->data.streamSeqPacket.acceptedConnections, i);
			break;
		}
	}
	SEM_UNLOCK(info.semid, E_INT_ERROR);
	if (key == -1)
		return E_INT_ERROR;

	struct infoConnected* sockInfCon = &sock->info.streamSeqPacket.connected;

	sockInfCon->connect.fd = -1;
	sockInfCon->addr = conKey;

	sockInfCon->semid = semget(key, 1, PERMS);
	RETURN_LOG_IF_FAILED_MSG(sockInfCon->semid, E_INT_ERROR, "soc_connect.semget");
	sockInfCon->shmid = shmget(key, sizeof(conOneWayBuf) * 2, 0);
	RETURN_LOG_IF_FAILED_MSG(sockInfCon->shmid, E_INT_ERROR, "soc_connect.shmget");
	sockInfCon->shmData = shmat(sockInfCon->shmid, NULL, 0);
	RETURN_LOG_IF_MSG(sockInfCon->shmData == (void*)-1, E_INT_ERROR, "soc_connect.shmat");
	sockInfCon->outBuf = sockInfCon->shmData;
	sockInfCon->inBuf = sockInfCon->outBuf + 1;

	res = kill(pid, INT_CONNECTION_SIGNAL);
	RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_connect.kill");

	sock->status = SST_CONNECTED;

	return 0;
}

int soc_recv(socket sockId, void* buf, size_t size) {
	if (buf == NULL || size == 0)
		return E_ARG_INVAL;
	sockInfo* sock = sa_getPtr_sockInfo(&sockets, sockId);
	if (sock == NULL)
		return E_ARG_INVAL;

	if (sock->type == SOCK_DGRAM && sock->status != SST_BINDED)
		return E_SOCK_NOT_BOUND;

	if (sock->type != SOCK_DGRAM && sock->status == SST_LISTENING)
		return E_SOCK_OP_NOT_SUPP;
	if (sock->type != SOCK_DGRAM && sock->status != SST_CONNECTED)
		return E_SOCK_NOT_CON;

	int shmid;
	int semid;
	conOneWayBuf* in;
	if (sock->type == SOCK_DGRAM) {
		shmid = sock->info.dgram.shmid;
		semid = sock->info.dgram.semid;
		in = sock->info.dgram.inBuf;
	} else {
		shmid = sock->info.streamSeqPacket.connected.shmid;
		semid = sock->info.streamSeqPacket.connected.semid;
		in = sock->info.streamSeqPacket.connected.inBuf;
	}

	struct shmid_ds shminf;
	int res = shmctl(shmid, IPC_STAT, &shminf);
	RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_send.shmctl");
	if (shminf.shm_perm.mode & 01000) // 01000 == SHM_DEST
		return E_CON_CLOSED;

	int result;
	SEM_LOCK(semid, E_INT_ERROR);
	if (sock->type == SOCK_STREAM) {
		result = (int)b_read_conOneWayBuf(in, buf, size);
	} else {
		size_t packetSize;
		size_t res = b_read_conOneWayBuf(in, &packetSize, sizeof(packetSize));
		if (res == 0) {
			result = 0;
		} else {
			size_t readSize = MIN(packetSize, size);
			result = (int)b_read_conOneWayBuf(in, buf, readSize);
			if (readSize < packetSize)
				b_read_conOneWayBuf(in, NULL, (packetSize - readSize));
		}
	}
	SEM_UNLOCK(semid, E_INT_ERROR);
	return result;
}

int soc_send(socket sockId, void* buf, size_t size) {
	if (buf == NULL || size == 0)
		return E_ARG_INVAL;
	sockInfo* sock = sa_getPtr_sockInfo(&sockets, sockId);
	if (sock == NULL)
		return E_ARG_INVAL;

	if (sock->type == SOCK_DGRAM)
		return soc_sendTo(sockId, buf, size, NULL);

	if (sock->status == SST_LISTENING)
		return E_SOCK_OP_NOT_SUPP;
	if (sock->status != SST_CONNECTED)
		return E_SOCK_NOT_CON;

	struct infoConnected* sockInfCon = &sock->info.streamSeqPacket.connected;

	struct shmid_ds shminf;
	int res = shmctl(sockInfCon->shmid, IPC_STAT, &shminf);
	RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_send.shmctl");
	if (shminf.shm_perm.mode & 01000) // 01000 == SHM_DEST
		return E_CON_CLOSED;

	SEM_LOCK(sockInfCon->semid, E_INT_ERROR);
	if (sock->type == SOCK_SEQPACKET)
		b_write_conOneWayBuf(sockInfCon->outBuf, (void*)&size, sizeof(size));
	b_write_conOneWayBuf(sockInfCon->outBuf, buf, size);
	SEM_UNLOCK(sockInfCon->semid, E_INT_ERROR);

	return 0;
}

int soc_sendTo(socket sockId, void* buf, size_t size, const char* addr) {
	if (buf == NULL || size == 0)
		return E_ARG_INVAL;
	sockInfo* sock = sa_getPtr_sockInfo(&sockets, sockId);
	if (sock == NULL)
		return E_ARG_INVAL;

	if (sock->type != SOCK_DGRAM)
		return soc_send(sockId, buf, size);

	key_t key = -1;
	if (addr != NULL) {
		int fd = open(addr, O_RDWR, PERMS);
		RETURN_LOG_IF_FAILED_MSG(fd, E_ADDR_INVAL, "soc_sendTo.open");
		close(fd);
		key = ftok(addr, SOCKETS_PROJECT_ID);
		RETURN_LOG_IF_FAILED_MSG(key, E_INT_ERROR, "soc_sendTo.ftok");
	} else {
		key = sock->info.dgram.defaultSendAddr;
		if (key == -1)
			return E_DEST_ADDR_REQ;
	}

	SEM_LOCK(info.semid, E_INT_ERROR);
	gs_onlSockInf* sockInf = findSocketInOnlineSockets(key);
	if (sockInf == NULL || sockInf->type != SOCK_DGRAM || (sockInf->data.dgram.recvAddr != -1 && sockInf->data.dgram.recvAddr != sock->bind.key)) {
		SEM_UNLOCK(info.semid, E_INT_ERROR);
		return E_ENDP_NOT_CON;
	}
	SEM_UNLOCK(info.semid, E_INT_ERROR);

	int semid = semget(key, 1, PERMS);
	RETURN_LOG_IF_FAILED_MSG(semid, E_INT_ERROR, "soc_sendTo.semget");
	int shmid = shmget(key, sizeof(conOneWayBuf), 0);
	RETURN_LOG_IF_FAILED_MSG(shmid, E_INT_ERROR, "soc_sendTo.shmget");
	conOneWayBuf* out = (conOneWayBuf*)shmat(shmid, NULL, 0);
	RETURN_LOG_IF_MSG(out == (void*)-1, E_INT_ERROR, "soc_sendTo.shmat");

	SEM_LOCK(semid, E_INT_ERROR);
	b_write_conOneWayBuf(out, (void*)&size, sizeof(size));
	b_write_conOneWayBuf(out, buf, size);
	SEM_UNLOCK(semid, E_INT_ERROR);

	int res = shmdt(out);
	RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_close.shmdt");

	return 0;
}

future* soc_recvAsync(socket sockId, size_t size) {
	if (size == 0)
		return NULL; // E_ARG_INVAL
	sockInfo* sock = sa_getPtr_sockInfo(&sockets, sockId);
	if (sock == NULL)
		return NULL; // E_ARG_INVAL

	if (sock->type == SOCK_DGRAM && sock->status != SST_BINDED)
		return NULL; // E_SOCK_NOT_BOUND

	if (sock->type != SOCK_DGRAM && sock->status == SST_LISTENING)
		return NULL; // E_SOCK_OP_NOT_SUPP
	if (sock->type != SOCK_DGRAM && sock->status != SST_CONNECTED)
		return NULL; // E_SOCK_NOT_CON


	int shmid;
	if (sock->type == SOCK_DGRAM) {
		shmid = sock->info.dgram.shmid;

	} else {
		shmid = sock->info.streamSeqPacket.connected.shmid;
	}

	struct shmid_ds shminf;
	int res = shmctl(shmid, IPC_STAT, &shminf);
	RETURN_LOG_IF_FAILED_MSG(res, NULL, "soc_send.shmctl"); // E_INT_ERROR
	if (shminf.shm_perm.mode & 01000) // 01000 == SHM_DEST
		return NULL; // E_CON_CLOSED

	future* newFuture = (future*)malloc(sizeof(future));
	RETURN_LOG_IF_NULL_MSG(newFuture, NULL, "createFuture.malloc"); // E_NO_SPACE
	newFuture->sockId = sockId;
	newFuture->size = size;
	newFuture->data = malloc(size);
	if (newFuture->data == NULL) {
		int tmperrno = errno;
		free(newFuture);
		errno = tmperrno;
		RETURN_LOG_MSG(NULL, "createFuture.malloc2"); // E_NO_SPACE
	}
	res = pthread_mutex_init(&newFuture->mutex, NULL);
	if (res != 0) {
		free(newFuture->data);
		free(newFuture);
		errno = res;
		RETURN_LOG_MSG(NULL, "createFuture.pthread_mutex_init"); // E_INT_ERROR
	}
	res = pthread_cond_init(&newFuture->cv, NULL);
	if (res != 0) {
		free(newFuture->data);
		free(newFuture);
		pthread_mutex_destroy(&newFuture->mutex);
		errno = res;
		RETURN_LOG_MSG(NULL, "createFuture.pthread_cond_init"); // E_INT_ERROR
	}

	res = pthread_create(&newFuture->thread, NULL, recvAsyncThreadRoutine, newFuture);
	if (res != 0) {
		free(newFuture->data);
		pthread_mutex_destroy(&newFuture->mutex);
		pthread_cond_destroy(&newFuture->cv);
		free(newFuture);
		errno = res;
		RETURN_LOG_MSG(NULL, "createFuture.pthread_create"); // E_INT_ERROR
	}

	return newFuture;
}

int soc_sendAsync(socket sockId, void* buf, size_t size) {
	if (buf == NULL || size == 0)
		return E_ARG_INVAL;
	sockInfo* sock = sa_getPtr_sockInfo(&sockets, sockId);
	if (sock == NULL)
		return E_ARG_INVAL;

	if (sock->type == SOCK_DGRAM)
		return soc_sendToAsync(sockId, buf, size, NULL);

	if (sock->status == SST_LISTENING)
		return E_SOCK_OP_NOT_SUPP;
	if (sock->status != SST_CONNECTED)
		return E_SOCK_NOT_CON;

	struct infoConnected* sockInfCon = &sock->info.streamSeqPacket.connected;

	struct shmid_ds shminf;
	int res = shmctl(sockInfCon->shmid, IPC_STAT, &shminf);
	RETURN_LOG_IF_FAILED_MSG(res, E_INT_ERROR, "soc_sendAsync.shmctl");
	if (shminf.shm_perm.mode & 01000) // 01000 == SHM_DEST
		return E_CON_CLOSED;

	sendAsyncData* data = (sendAsyncData*)malloc(sizeof(sendAsyncData));
	RETURN_LOG_IF_NULL_MSG(data, E_NO_SPACE, "soc_sendAsync.malloc");
	data->sock = sockId;
	data->buf = malloc(size);
	if (data->buf == NULL) {
		int tmperrno = errno;
		free(data);
		errno = tmperrno;
		RETURN_LOG_MSG(E_NO_SPACE, "soc_sendAsync.malloc");
	}
	memcpy(&data->buf, buf, size);
	data->size = size;
	pthread_t thread;
	res = pthread_create(&thread, NULL, sendAsyncThreadRoutine, &data);
	if (res != 0) {
		free(data->buf);
		free(data);
		errno = res;
		RETURN_LOG_MSG(E_INT_ERROR, "soc_sendAsync.pthread_create");
	}
	return 0;
}

int soc_sendToAsync(socket sockId, void* buf, size_t size, const char* addr) {
	if (buf == NULL || size == 0)
		return E_ARG_INVAL;
	sockInfo* sock = sa_getPtr_sockInfo(&sockets, sockId);
	if (sock == NULL)
		return E_ARG_INVAL;

	if (sock->type != SOCK_DGRAM)
		return soc_sendAsync(sockId, buf, size);

	key_t key = -1;
	if (addr != NULL) {
		int fd = open(addr, O_RDWR, PERMS);
		RETURN_LOG_IF_FAILED_MSG(fd, E_ADDR_INVAL, "soc_sendTo.open");
		close(fd);
		key = ftok(addr, SOCKETS_PROJECT_ID);
		RETURN_LOG_IF_FAILED_MSG(key, E_INT_ERROR, "soc_sendTo.ftok");
	} else {
		key = sock->info.dgram.defaultSendAddr;
		if (key == -1)
			return E_DEST_ADDR_REQ;
	}

	sendAsyncData* data = (sendAsyncData*)malloc(sizeof(sendAsyncData));
	RETURN_LOG_IF_NULL_MSG(data, E_NO_SPACE, "soc_sendAsync.malloc");
	data->sock = sockId;
	data->buf = malloc(size);
	if (data->buf == NULL) {
		int tmperrno = errno;
		free(data);
		errno = tmperrno;
		RETURN_LOG_MSG(E_NO_SPACE, "soc_sendAsync.malloc");
	}
	memcpy(&data->buf, buf, size);
	data->size = size;
	size_t addrlengh = strlen(addr) + 1;
	data->addr = malloc(addrlengh);
	if (data->addr == NULL) {
		int tmperrno = errno;
		free(data->buf);
		free(data);
		errno = tmperrno;
		RETURN_LOG_MSG(E_NO_SPACE, "soc_sendAsync.malloc2");
	}
	memcpy(&data->addr, addr, addrlengh);
	pthread_t thread;
	int res = pthread_create(&thread, NULL, sendAsyncThreadRoutine, &data);
	if (res != 0) {
		free(data->buf);
		free(data->addr);
		free(data);
		errno = res;
		RETURN_LOG_MSG(E_INT_ERROR, "soc_sendAsync.pthread_create");
	}
	return 0;
}

int soc_futureGet(future* fut, void* buf) {
	MUTEX_LOCK(&fut->mutex, E_INT_ERROR);
	while (fut->isReady != 1) {
		pthread_cond_wait(&fut->cv, &fut->mutex);
	}
	MUTEX_UNLOCK(&fut->mutex, E_INT_ERROR);
	if (fut->actualSize > 0)
		memcpy(buf, fut->data, (size_t)fut->actualSize);
	int res = fut->actualSize;
	free(fut->data);
	pthread_mutex_destroy(&fut->mutex);
	pthread_cond_destroy(&fut->cv);
	free(fut);
	return res;
}

int soc_futureisready(future* fut) {
	pthread_mutex_lock(&fut->mutex);
	int res = fut->isReady;
	pthread_mutex_unlock(&fut->mutex);
	return res;
}

int semPerformAction(int semid, unsigned short semnum, semAction action) {
	if (action == SEM_UNLOCK) {
		union semun un = { .val = 1 };
		return semctl(semid, semnum, SETVAL, un);
	}
	struct sembuf sop = {
		.sem_num = semnum,
		.sem_flg = 0,
		.sem_op = (action == SEM_LOCK ? -1 : 0)
	};
	return semop(semid, &sop, 1);
}

int copyString(char** dest, const char* src) {
	size_t size = strlen(src);
	*dest = (char*)malloc(size + 1);
	RETURN_LOG_IF_NULL(*dest, -1);
	memcpy(*dest, src, size + 1);
	return 0;
}

void* timerThreadRoutine(__attribute__((unused)) void* unused) {
	int sig;
	int res;
	sigset_t sigset;
	res = sigemptyset(&sigset);
	FAIL_FAST_IF_FAILED_MSG(res, "timerThreadRoutine.sigemptyset");
	res = sigaddset(&sigset, INT_ONL_SOCK_TIMER_SIGNAL);
	FAIL_FAST_IF_FAILED_MSG(res, "timerThreadRoutine.sigaddset");
	while (!sigwait(&sigset, &sig)) {
		time_t currentTime = time(NULL);
		pid_t pid = getpid();
		SEM_LOCK(info.semid, NULL);
		staticVector_gs_onlSockInf* vec = (staticVector_gs_onlSockInf*)&(gsData->onlineSockets);
		for (size_t i = 0; i < sv_size_gs_onlSockInf(vec); i++) {
			gs_onlSockInf* sockInf = sv_getPtr_gs_onlSockInf(vec, i);
			if (sockInf->pid == pid) {
				sockInf->lastUpdate = currentTime;
			}
		}
		SEM_UNLOCK(info.semid, NULL);
	}
	// if we are out of the loop that means that an error occured
	FAIL_FAST_MSG("timerThreadRoutine.sigwait");
	return NULL;
}

void tidyOnlineSocketsVector() {
	time_t currentTime = time(NULL);
	size_t i = 0;
	staticVector_gs_onlSockInf* vec = (staticVector_gs_onlSockInf*)&(gsData->onlineSockets);
	while (i < sv_size_gs_onlSockInf(vec)) {
		gs_onlSockInf* sockInf = sv_getPtr_gs_onlSockInf(vec, i);
		if (difftime(sockInf->lastUpdate, currentTime) > (ONLINE_SOCK_TIMEOUT_SEC + ONLINE_SOCK_TIMEOUT_SEC_DEV)) {
			sv_remove_gs_onlSockInf(vec, i);
			continue;
		}
		i++;
	}
}

gs_onlSockInf* findSocketInOnlineSockets(key_t addr) {
	tidyOnlineSocketsVector();
	gs_onlSockInf* sockInf;
	for (size_t i = 0; i < sv_size_gs_onlSockInf(&(gsData->onlineSockets)); i++) {
		sockInf = sv_getPtr_gs_onlSockInf(&(gsData->onlineSockets), i);
		if (sockInf->addrKey == addr)
			break;
		sockInf = NULL;
	}
	return sockInf;
}

void* recvAsyncThreadRoutine(void* arg) {
	future* fut = (future*)arg;
	fut->actualSize = soc_recv(fut->sockId, fut->data, fut->size);
	pthread_mutex_lock(&fut->mutex);
	fut->isReady = 1;
	pthread_mutex_unlock(&fut->mutex);
	pthread_cond_signal(&fut->cv);
	return NULL;
}

void* sendAsyncThreadRoutine(void* arg) {
	sendAsyncData* data = (sendAsyncData*)arg;
	if (data->addr == NULL)
		soc_send(data->sock, data->buf, data->size);
	else
		soc_sendTo(data->sock, data->buf, data->size, data->addr);
	return NULL;
}

void onExit() {
	for (int i = 0; i < SOCK_ARRAY_SIZE; i++) {
		sockInfo* sock = sa_getPtr_sockInfo(&sockets, i);
		if (sock != NULL)
			soc_close((socket)i);
	}
}
