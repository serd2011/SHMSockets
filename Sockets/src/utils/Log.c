#include "Log.h"

#include <stdio.h>

void reportFailure_LastError(failureType_t type, const char* message) {
	perror(message);
	if (type == FAILURE_TYPE_FAIL_FAST)
		INT_FAIL_FAST();
}
