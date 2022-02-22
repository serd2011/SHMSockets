#include <stdlib.h>

#undef Q_DO_NOT_CREATE

#ifndef T
#error "Queue: T is not defined"
#define Q_DO_NOT_CREATE
#endif

#undef	M_CONCATENATE_
#undef	M_CONCATENATE
#define M_CONCATENATE_(A, B) A ## B
#define M_CONCATENATE(A, B) M_CONCATENATE_(A,B)

#undef QUEUE_T_NAME

#undef QUEUE_F_INIT
#undef QUEUE_F_PUSH
#undef QUEUE_F_POP
#undef QUEUE_F_SIZE


#ifndef Q_DO_NOT_CREATE


// Type names macros

#define QUEUE_T_NAME        M_CONCATENATE(queue_, T)
#define QUEUE_T_ELEM_NAME   M_CONCATENATE(queueElem_, T)
#define QUEUE_T_ELEM_       M_CONCATENATE(int_queueElem, T)


// Function names macros

#define QUEUE_F_INIT    M_CONCATENATE(q_init_, T)
#define QUEUE_F_PUSH    M_CONCATENATE(q_push_, T)
#define QUEUE_F_POP     M_CONCATENATE(q_pop_, T)
#define QUEUE_F_SIZE    M_CONCATENATE(q_size_, T)


// Some Typedefs

typedef struct QUEUE_T_ELEM_ {
	T data;
	struct QUEUE_T_ELEM_* prev;
	struct QUEUE_T_ELEM_* next;
} QUEUE_T_ELEM_NAME;

typedef struct {
	QUEUE_T_ELEM_NAME* front;
	QUEUE_T_ELEM_NAME* back;
	int size;
} QUEUE_T_NAME;


// Function Declarations

static void QUEUE_F_INIT(QUEUE_T_NAME* queue);
static void QUEUE_F_PUSH(QUEUE_T_NAME* queue, T* elem);
static T    QUEUE_F_POP(QUEUE_T_NAME* queue);
static int  QUEUE_F_SIZE(QUEUE_T_NAME* queue);


// Function Definitions

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static void QUEUE_F_INIT(QUEUE_T_NAME* queue) {
	queue->front = NULL;
	queue->back = NULL;
	queue->size = 0;
};

static void QUEUE_F_PUSH(QUEUE_T_NAME* queue, T* elem) {
	QUEUE_T_ELEM_NAME* queueElem = (QUEUE_T_ELEM_NAME*)malloc(sizeof(QUEUE_T_ELEM_NAME));
	queueElem->data = *elem;
	if (queue->size == 0) queue->front = queueElem;
	else queue->back->next = queueElem;
	queueElem->prev = queue->back;
	queueElem->next = NULL;
	queue->back = queueElem;
	queue->size++;
	return;
};

static T QUEUE_F_POP(QUEUE_T_NAME* queue) {
	queue->size--;
	QUEUE_T_ELEM_NAME* queueElem = queue->front;
	queue->front = queueElem->next;
	if (queue->size == 0) queue->back = NULL;
	T data = queueElem->data;
	free(queueElem);
	return data;
}

static int QUEUE_F_SIZE(QUEUE_T_NAME* queue) {
	return queue->size;
}

#pragma GCC diagnostic pop

#endif
