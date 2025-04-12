#pragma once

#include "defines.h"

#define ASSERTIONS_ENABLED

#ifdef ASSERTIONS_ENABLED
#if _MSC_VER
#include <intrin.h>
#define debugBreak() __debugbreak()
#else
#define debugBreak() __builtin_trap()
#endif

MAPI void report_assertion_failure(const char* expr, const char* msg, const char* file, i32 line);

#define MASSERT(expr) \
    { \
        if (expr) { \
        } else { \
            report_assertion_failure(#expr, "", __FILE__, __LINE__); \
            debugBreak(); \
        } \
    }

#define MASSERT_MSG(expr, msg) \
    { \
        if (expr) { \
        } else { \
            report_assertion_failure(#expr, msg, __FILE__, __LINE__); \
            debugBreak(); \
        } \
    }

#ifdef ENGINE_DEBUG
#define MASSERT_DEBUG(expr, msg) \
{ \
    if (expr) { \
    } else { \
        report_assertion_failure(#expr, "", __FILE__, __LINE__); \
        debugBreak(); \
    } \
}
#else
#define MASSERT_DEBUG(expr, msg)
#endif

#else
#define MASSERT(expr)
#define MASSERT_MSG(expr, msg)
#define MASSERT_DEBUG(expr)
#endif
