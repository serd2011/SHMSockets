#include <stdlib.h>

#undef	A_DO_NOT_CREATE

#ifndef T
#error	"Static Array: T is not defined"
#define SA_DO_NOT_CREATE
#endif

#ifndef CAPACITY
#error	"Static Array: CAPACITY is not defined"
#define SA_DO_NOT_CREATE
#endif

#undef	M_CONCATENATE_
#undef	M_CONCATENATE
#define M_CONCATENATE_(A, B) A ## B
#define M_CONCATENATE(A, B) M_CONCATENATE_(A,B)

#undef	ST_ARRAY_T_NAME

#undef	ST_ARRAY_F_ADD
#undef	ST_ARRAY_F_REMOVE
#undef	ST_ARRAY_F_GET
#undef	ST_ARRAY_F_GETPTR
#undef	ST_ARRAY_F_SIZE
#undef	ST_ARRAY_F_SORT


#ifndef SV_DO_NOT_CREATE


// Type names macros

#define ST_ARRAY_T_NAME			M_CONCATENATE(staticArray_, T)
#define ST_ARRAY_T_ELEM_NAME	M_CONCATENATE(sa_elem_, T)


// Function names macros

#define ST_ARRAY_F_ADD			M_CONCATENATE(sa_add_, T)
#define ST_ARRAY_F_REMOVE		M_CONCATENATE(sa_remove_, T)
#define ST_ARRAY_F_GETPTR		M_CONCATENATE(sa_getPtr_, T)


// Some Typedefs

typedef struct {
	int isOccupied;
	T elem;
} ST_ARRAY_T_ELEM_NAME;

// Array that is allocated staticaly
typedef struct {
	ST_ARRAY_T_ELEM_NAME data[CAPACITY];
} ST_ARRAY_T_NAME;


// Function Declarations

// Adds new item to the end of an array
static int ST_ARRAY_F_ADD(ST_ARRAY_T_NAME* arr, const T* newElement);
// Removes item from an array on specified position
static void ST_ARRAY_F_REMOVE(ST_ARRAY_T_NAME* arr, int index);
// Returns pointer to an item from an array on specified position
static T* ST_ARRAY_F_GETPTR(ST_ARRAY_T_NAME* arr, int index);


// Function Definitions

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static int ST_ARRAY_F_ADD(ST_ARRAY_T_NAME* arr, const T* newElement) {
	for (int i = 0; i < CAPACITY; i++) {
		if (arr->data[i].isOccupied == 0) {
			arr->data[i].isOccupied = 1;
			arr->data[i].elem = *newElement;
			return i;
		}
	}
	return -1;
}

static void ST_ARRAY_F_REMOVE(ST_ARRAY_T_NAME* arr, int index) {
	arr->data[index].isOccupied = 0;
}

static T* ST_ARRAY_F_GETPTR(ST_ARRAY_T_NAME* arr, int index) {
	if (arr->data[index].isOccupied == 0)
		return NULL;
	return &arr->data[index].elem;
}

#pragma GCC diagnostic pop

#endif
