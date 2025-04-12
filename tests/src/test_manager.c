#include "test_manager.h"

#include <containers/darray.h>
#include <core/logger.h>
#include <strings/string.h>
#include <time/clock.h>


typedef struct test_entry {
    PFN_test func;
    char* desc;
} test_entry;

static test_entry* tests;

void test_manager_init(void) {
    tests = darray_reserve(test_entry, 32);

}

void test_manager_register_test(PFN_test test, char* desc) {
    test_entry e;
    e.func = test;
    e.desc = desc;
    darray_push(tests, e);
}

void test_manager_run_tests(void) {
    u32 passed = 0;
    u32 failed = 0;
    u32 skipped = 0;

    u64 count = darray_length(tests);

    clock total_time;
    clock_start(&total_time);

    for (u64 i = 0; i < count; ++i) {
        clock test_time;
        clock_start(&test_time);

        u8 result = tests[i].func();

        clock_update(&test_time);

        if (result == true) {
            ++passed;
        } else if (result == BYPASS) {
            MWARN("[SKIPPED]: %s", tests[i].desc);
            ++skipped;
        } else {
            MERROR("[FAILED]: %s", tests[i].desc);
            ++failed;
        }

        // TODO: allocation every time ...
        char* status = cstr_format(failed ? "*** %d FAILED ***": "SUCCESS", failed);
        clock_update(&total_time);
        MINFO("Executed %d of %d (skipped %d) %s (%.6f sec / %.6f sec total)",
            i + 1,
            count,
            skipped,
            status,
            test_time.elapsed,
            total_time.elapsed);
        cstr_free(status);
    }

    clock_stop(&total_time);

    MINFO("Results: %d passed, %d failed, %d skipped", passed, failed, skipped);
}