#include <stddef.h>

#ifndef BUFFER_SIZE
#error	"Buffer: BUFFER_SIZE is not defined"
#define B_DO_NOT_CREATE
#endif

#undef	M_CONCATENATE_
#undef	M_CONCATENATE
#define M_CONCATENATE_(A, B) A ## B
#define M_CONCATENATE(A, B) M_CONCATENATE_(A,B)

#undef BUFFER_T_NAME
#undef BUFFER_N_POSTFIX
#undef BUFFER_F_READ
#undef BUFFER_F_WRITE

#undef	MIN
#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))


#ifndef B_DO_NOT_CREATE

// Type names macros

#ifdef  BUFFER_NAME
#define BUFFER_T_NAME		BUFFER_NAME
#define BUFFER_N_POSTFIX	BUFFER_NAME
#else
#define BUFFER_T_NAME		M_CONCATENATE(buf_, BUFFER_SIZE)
#define BUFFER_N_POSTFIX	BUFFER_SIZE
#endif


// Function names macros

#define BUFFER_F_CLEAR	M_CONCATENATE(b_clear_, BUFFER_N_POSTFIX)
#define BUFFER_F_READ	M_CONCATENATE(b_read_, BUFFER_N_POSTFIX)
#define BUFFER_F_WRITE	M_CONCATENATE(b_write_, BUFFER_N_POSTFIX)


// Some Typedefs

typedef struct {
	size_t readOffset;
	size_t writeOffset;
	char data[BUFFER_SIZE];
} BUFFER_T_NAME;


// Function Declarations

// Clears buffer
static void BUFFER_F_CLEAR(BUFFER_T_NAME* buf);
// Reads up to 'count' bytes from buffer into 'dest'
// 'dest' can be NULL - up to 'count' bytes will be dropped from the buffer
// Returns number of read/dropped bytes 
static size_t BUFFER_F_READ(BUFFER_T_NAME* buf, void* dest, size_t count);
// Writes 'count' bytes into buffer from 'src'
static void BUFFER_F_WRITE(BUFFER_T_NAME* buf, void* src, size_t count);


// Function Definitions

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static void BUFFER_F_CLEAR(BUFFER_T_NAME* buf) {
	buf->readOffset = 0;
	buf->writeOffset = 0;
}

static size_t BUFFER_F_READ(BUFFER_T_NAME* buf, void* dest, size_t count) {
	if (buf->writeOffset == buf->readOffset)
		return 0; // Nothing to read

	// Calculating how much will be read
	size_t avaliable = 0;
	if (buf->writeOffset > buf->readOffset) {
		avaliable = buf->writeOffset - buf->readOffset;
	} else {
		avaliable = buf->writeOffset + (BUFFER_SIZE - buf->readOffset);
	}
	size_t readCount = MIN(avaliable, count);

	// If we dont need to cross boundary to read 'readCount' bytes
	if ((buf->readOffset + readCount) < BUFFER_SIZE) {
		if (dest != NULL) {
			memcpy(dest, (buf->data + buf->readOffset), readCount);
		}
		buf->readOffset += readCount;
		return readCount;
	}

	// Need to cross boundary
	size_t firstPartSize = BUFFER_SIZE - readCount;
	size_t lastPartSize = readCount - firstPartSize;
	if (dest != NULL) {
		memcpy(dest, (buf->data + buf->readOffset), firstPartSize);
		memcpy((dest + firstPartSize), buf->data, lastPartSize);
	}
	buf->readOffset = lastPartSize;
	return readCount;
}

static void BUFFER_F_WRITE(BUFFER_T_NAME* buf, void* src, size_t count) {
	// If we dont need to cross boundary to write 'count' bytes
	if (buf->writeOffset + count < BUFFER_SIZE) {
		memcpy((buf->data + buf->writeOffset), src, count);
		buf->writeOffset += count;
		return;
	}

	// Need to cross boundary
	size_t firstPartSize = BUFFER_SIZE - count;
	size_t lastPartSize = count - firstPartSize;
	memcpy((buf->data + buf->writeOffset), src, firstPartSize);
	memcpy(buf->data, (src + firstPartSize), lastPartSize);
	buf->writeOffset = lastPartSize;
}

#pragma GCC diagnostic pop

#undef BUFFER_T_NAME

#endif