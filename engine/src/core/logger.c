#include "logger.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "asserts.h"

#include "platform/platform.h"

#include "strings/string.h"


b8 logging_system_initialize() {
    return true;
}

void logging_system_shutdown() {

}

void log_output(log_level level, const char* msg, ...) {
    static const char* level_strings[6] = {
        "[FATAL]: ",
        "[ERROR]: ",
        "[WARN]:  ",
        "[INFO]:  ",
        "[DEBUG]: ",
        "[TRACE]: "
    };

    __builtin_va_list arg_ptr;
    va_start(arg_ptr, msg);
    char* formatted = cstr_format_v(msg, arg_ptr);
    va_end(arg_ptr);

    char* out_msg = cstr_format("%s%s\n", level_strings[level], formatted);
    cstr_free(formatted);

    platform_console_write(level, out_msg);

    cstr_free(out_msg);
}

void report_assertion_failure(const char* expr, const char* msg, const char* file, i32 line) {
    log_output(LOG_LEVEL_FATAL, "Assertion failure: %s, message: '%s', in file: %s:%d\n", expr, msg, file, line);
}