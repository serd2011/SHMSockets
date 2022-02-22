#pragma once

#include <stddef.h>
#include <stdlib.h>

#define INT_FAIL_FAST() {exit(-1);}

#ifndef NO_LOG

#define INT_REPORT_MSG(message, type)					{reportFailure_LastError(type, message);}
#define INT_REPORT_IF_MSG(condition, message, type)		{if(condition) INT_REPORT_MSG(message, type);}
#define INT_REPORT_IF_NULL_MSG(ptr, message, type)		{if(ptr == NULL) INT_REPORT_MSG(message, type);}
#define INT_REPORT_IF_FAILED_MSG(result, message, type)	{if(result == -1) INT_REPORT_MSG(message, type);}

#define RETURN_LOG_MSG(val, message)					{INT_REPORT_MSG(message,FAILURE_TYPE_LOG); return val;}
#define RETURN_LOG_IF_MSG(condition, val, message)		{if(condition){ INT_REPORT_MSG(message, FAILURE_TYPE_LOG); return val;}}
#define RETURN_LOG_IF_NULL_MSG(ptr, val, message)		{if(ptr == NULL){ INT_REPORT_MSG(message, FAILURE_TYPE_LOG); return val;}}
#define RETURN_LOG_IF_FAILED_MSG(result, val, message)	{if(result == -1){ INT_REPORT_MSG(message, FAILURE_TYPE_LOG); return val;}}

#define RETURN_LOG(val)									RETURN_LOG_MSG(val, "")
#define RETURN_LOG_IF(condition, val)					RETURN_LOG_IF_MSG(condition, val, "")
#define RETURN_LOG_IF_NULL(ptr, val)					RETURN_LOG_IF_NULL_MSG(ptr, val, "")
#define RETURN_LOG_IF_FAILED(result, val)				RETURN_LOG_IF_FAILED_MSG(result, val, "")

#define LOG_MSG(message)								INT_REPORT_MSG(message, FAILURE_TYPE_LOG)
#define LOG_IF_MSG(condition, message)					INT_REPORT_IF_MSG(condition, message, FAILURE_TYPE_LOG)
#define LOG_IF_NULL_MSG(ptr, message)					INT_REPORT_IF_NULL_MSG(ptr, message, FAILURE_TYPE_LOG)
#define LOG_IF_FAILED_MSG(result, message)				INT_REPORT_IF_FAILED_MSG(result, message, FAILURE_TYPE_LOG)

#define LOG()											LOG_MSG("")
#define LOG_IF(condition)								LOG_IF_MSG(condition, "")
#define LOG_IF_NULL(ptr)								LOG_IF_NULL_MSG(ptr, "")
#define LOG_IF_FAILED(result)							LOG_IF_FAILED_MSG(result, "")

#define FAIL_FAST_MSG(message)							INT_REPORT_MSG(message, FAILURE_TYPE_FAIL_FAST)
#define FAIL_FAST_IF_MSG(condition, message)			INT_REPORT_IF_MSG(condition, message, FAILURE_TYPE_FAIL_FAST)
#define FAIL_FAST_IF_NULL_MSG(ptr, message)				INT_REPORT_IF_NULL_MSG(ptr, message, FAILURE_TYPE_FAIL_FAST)
#define FAIL_FAST_IF_FAILED_MSG(result, message)		INT_REPORT_IF_FAILED_MSG(result, message, FAILURE_TYPE_FAIL_FAST)

#define FAIL_FAST()										FAIL_FAST_MSG("")
#define FAIL_FAST_IF(condition)							FAIL_FAST_IF_MSG(condition, "")
#define FAIL_FAST_IF_NULL(ptr)							FAIL_FAST_IF_NULL_MSG(ptr, "")
#define FAIL_FAST_IF_FAILED(result)						FAIL_FAST_IF_FAILED_MSG(result, "")

#else

#define RETURN_LOG_MSG(val, message)					{return(val);}
#define RETURN_LOG_IF_MSG(condition, val, message)		{if(condition) return(val);}
#define RETURN_LOG_IF_NULL_MSG(ptr, val, message)		{if(ptr == NULL) return(val);}
#define RETURN_LOG_IF_FAILED_MSG(result, val, message)	{if(result == -1)  return(val);}

#define RETURN_LOG(val)									RETURN_LOG_MSG(val, "")
#define RETURN_LOG_IF(condition, val)					RETURN_LOG_IF_MSG(condition, val, "")
#define RETURN_LOG_IF_NULL(ptr, val)					RETURN_LOG_IF_NULL_MSG(ptr, val, "")
#define RETURN_LOG_IF_FAILED(result, val)				RETURN_LOG_IF_FAILED_MSG(result, val, "")

#define LOG_MSG(message)								{}
#define LOG_IF_MSG(condition, message)					{}
#define LOG_IF_NULL_MSG(ptr, message)					{}
#define LOG_IF_FAILED_MSG(result, message)				{}

#define LOG()											{}
#define LOG_IF(condition)								{}
#define LOG_IF_NULL(ptr)								{}
#define LOG_IF_FAILED(result)							{}

#define FAIL_FAST_MSG(message)							INT_FAIL_FAST()
#define FAIL_FAST_IF_MSG(condition, message)			{if(condition) INT_FAIL_FAST();}
#define FAIL_FAST_IF_NULL_MSG(ptr, message)				{if(ptr == NULL) INT_FAIL_FAST();}
#define FAIL_FAST_IF_FAILED_MSG(result, message)		{if(result == -1)  INT_FAIL_FAST();}

#define FAIL_FAST()										FAIL_FAST_MSG("")
#define FAIL_FAST_IF(condition)							FAIL_FAST_IF_MSG(condition, "")
#define FAIL_FAST_IF_NULL(ptr)							FAIL_FAST_IF_NULL_MSG(ptr, "")
#define FAIL_FAST_IF_FAILED(result)						FAIL_FAST_IF_FAILED_MSG(result, "")

#endif

typedef enum FailureType {
	FAILURE_TYPE_LOG,
	FAILURE_TYPE_FAIL_FAST
} failureType_t;

void reportFailure_LastError(failureType_t, const char* message);
