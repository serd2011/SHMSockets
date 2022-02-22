#include <stdlib.h>

#undef STQ_DO_NOT_CREATE

#ifndef T
#error "Static Queue: T is not defined"
#define SQ_DO_NOT_CREATE
#endif

#ifndef CAPACITY
#error	"Static Queue: CAPACITY is not defined"
#define SQ_DO_NOT_CREATE
#endif

#undef	M_CONCATENATE_
#undef	M_CONCATENATE
#define M_CONCATENATE_(A, B) A ## B
#define M_CONCATENATE(A, B) M_CONCATENATE_(A,B)

#undef ST_QUEUE_T_NAME

#undef ST_QUEUE_F_INIT
#undef ST_QUEUE_F_PUSH
#undef ST_QUEUE_F_POP
#undef ST_QUEUE_F_SIZE


#ifndef SQ_DO_NOT_CREATE


// Type names macros

#define ST_QUEUE_T_NAME        M_CONCATENATE(staticQueue_, T)


// Function names macros

#define ST_QUEUE_F_PUSH    M_CONCATENATE(sq_push_, T)
#define ST_QUEUE_F_POP_PTR M_CONCATENATE(sq_popPtr_, T)
#define ST_QUEUE_F_SIZE    M_CONCATENATE(sq_size_, T)


// Some Typedefs

typedef struct {
	size_t front;
	size_t count;
	T data[CAPACITY];
} ST_QUEUE_T_NAME;


// Function Declarations

static int ST_QUEUE_F_PUSH(ST_QUEUE_T_NAME* queue, T* elem);
static T* ST_QUEUE_F_POP_PTR(ST_QUEUE_T_NAME* queue);
static size_t ST_QUEUE_F_SIZE(ST_QUEUE_T_NAME* queue);


// Function Definitions

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static int ST_QUEUE_F_PUSH(ST_QUEUE_T_NAME* q, T* elem) {
	if (q->count == CAPACITY)
		return -1;
	size_t index = q->front + q->count;
	q->count++;
	if (index >= CAPACITY)
		index -= CAPACITY;
	q->data[index] = *elem;
	return 0;
};

static T* ST_QUEUE_F_POP_PTR(ST_QUEUE_T_NAME* q) {
	if (q->count == 0)
		return NULL;
	q->count--;
	return &(q->data[q->front++]);
}

static size_t ST_QUEUE_F_SIZE(ST_QUEUE_T_NAME* q) {
	return q->count;
}

#pragma GCC diagnostic pop

#endif
