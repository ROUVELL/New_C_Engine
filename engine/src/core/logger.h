#pragma once

#include "defines.h"

#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1

#if ENGINE_DEBUG
#   define LOG_DEBUG_ENABLED 1
#   define LOG_TRACE_ENABLED 1
#elif
#   define LOG_DEBUG_ENABLED 0
#   define LOG_TRACE_ENABLED 0
#endif

typedef enum log_level {
    LOG_LEVEL_FATAL = 0,
    LOG_LEVEL_ERROR = 1,
    LOG_LEVEL_WARN = 2,
    LOG_LEVEL_INFO = 3,
    LOG_LEVEL_DEBUG = 4,
    LOG_LEVEL_TRACE = 5
} log_level;

b8 logging_system_initialize();
void logging_system_shutdown();

MAPI void log_output(log_level level, const char* msg, ...);

#define MFATAL(msg, ...) log_output(LOG_LEVEL_FATAL, msg, ##__VA_ARGS__)

#define MERROR(msg, ...) log_output(LOG_LEVEL_ERROR, msg, ##__VA_ARGS__)

#if LOG_WARN_ENABLED == 1
#define MWARN(msg, ...) log_output(LOG_LEVEL_WARN, msg, ##__VA_ARGS__)
#else
#define MWARN(msg, ...)
#endif

#if LOG_INFO_ENABLED == 1
#define MINFO(msg, ...) log_output(LOG_LEVEL_INFO, msg, ##__VA_ARGS__)
#else
#define MINFO(msg, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
#define MDEBUG(msg, ...) log_output(LOG_LEVEL_DEBUG, msg, ##__VA_ARGS__)
#else
#define MDEBUG(msg, ...)
#endif

#if LOG_TRACE_ENABLED == 1
#define MTRACE(msg, ...) log_output(LOG_LEVEL_TRACE, msg, ##__VA_ARGS__)
#else
#define MTRACE(msg, ...)
#endif
