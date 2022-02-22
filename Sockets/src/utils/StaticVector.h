#include <stdlib.h>

#undef	V_DO_NOT_CREATE

#ifndef T
#error	"Static Vector: T is not defined"
#define SV_DO_NOT_CREATE
#endif

#ifndef CAPACITY
#error	"Static Vector: CAPACITY is not defined"
#define SV_DO_NOT_CREATE
#endif

#undef	M_CONCATENATE_
#undef	M_CONCATENATE
#define M_CONCATENATE_(A, B) A ## B
#define M_CONCATENATE(A, B) M_CONCATENATE_(A,B)

#undef	ST_VECTOR_T_NAME

#undef	ST_VECTOR_F_ADD
#undef	ST_VECTOR_F_REMOVE
#undef	ST_VECTOR_F_GET
#undef	ST_VECTOR_F_GETPTR
#undef	ST_VECTOR_F_SIZE
#undef	ST_VECTOR_F_SORT


#ifndef SV_DO_NOT_CREATE


// Type names macros

#define ST_VECTOR_T_NAME		M_CONCATENATE(staticVector_, T)
#define T_COMPARATOR			M_CONCATENATE(comparatorV_, T)


// Function names macros

#define ST_VECTOR_F_ADD			M_CONCATENATE(sv_add_, T)
#define ST_VECTOR_F_REMOVE		M_CONCATENATE(sv_remove_, T)
#define ST_VECTOR_F_GET			M_CONCATENATE(sv_get_, T)
#define ST_VECTOR_F_GETPTR		M_CONCATENATE(sv_getPtr_, T)
#define ST_VECTOR_F_SIZE		M_CONCATENATE(sv_size_, T)
#define ST_VECTOR_F_SORT		M_CONCATENATE(sv_sort_, T)


// Some Typedefs

// Vector that is allocated staticaly
typedef struct {
	size_t size;
	T data[CAPACITY];
} ST_VECTOR_T_NAME;

typedef int(*T_COMPARATOR)(T*, T*);


// Function Declarations

// Adds new item to the end of a vector
static int ST_VECTOR_F_ADD(ST_VECTOR_T_NAME* vec, const T* newElement);
// Removes item from a vector on specified position
static void ST_VECTOR_F_REMOVE(ST_VECTOR_T_NAME* vec, size_t index);
// Returns item from a vector on specified position
static T ST_VECTOR_F_GET(ST_VECTOR_T_NAME* vec, size_t index);
// Returns pointer to an item from a vector on specified position
static T* ST_VECTOR_F_GETPTR(ST_VECTOR_T_NAME* vec, size_t index);
// Returns size of a vector
static size_t ST_VECTOR_F_SIZE(ST_VECTOR_T_NAME* vec);
// Sorts vector with specified comparator
static void	ST_VECTOR_F_SORT(ST_VECTOR_T_NAME* vec, T_COMPARATOR);


// Function Definitions

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

static int ST_VECTOR_F_ADD(ST_VECTOR_T_NAME* vec, const T* newElement) {
	if (vec->size == CAPACITY)
		return -1;
	vec->data[vec->size++] = *newElement;
	return 0;
}

static void ST_VECTOR_F_REMOVE(ST_VECTOR_T_NAME* vec, size_t index) {
	for (size_t i = index; i < vec->size; i++) {
		vec->data[i] = vec->data[i + 1];
	}
	vec->size--;
}

static T ST_VECTOR_F_GET(ST_VECTOR_T_NAME* vec, size_t index) {
	return vec->data[index];
}

static T* ST_VECTOR_F_GETPTR(ST_VECTOR_T_NAME* vec, size_t index) {
	return vec->data + index;
}

static size_t ST_VECTOR_F_SIZE(ST_VECTOR_T_NAME* vec) {
	return vec->size;
}

static void ST_VECTOR_F_SORT(ST_VECTOR_T_NAME* vec, T_COMPARATOR comparator) {
	T tmp;
	for (size_t i = 0; i < vec->size - 1; i++) {
		for (size_t j = 0; j < vec->size - i - 1; j++) {
			if (comparator(vec->data + j, vec->data + j + 1)) {
				tmp = vec->data[j];
				vec->data[j] = vec->data[j + 1];
				vec->data[j + 1] = tmp;
			}
		}
	}
}

#pragma GCC diagnostic pop

#endif
