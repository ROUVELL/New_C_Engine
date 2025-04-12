#include "../test_manager.h"
#include "../expect.h"

#include <defines.h>
#include <memory/memory.h>
#include <strings/string.h>

static u8 cstr_ncmp_tests(void) {
    const char* str1 = "texture";
    const char* str2 = "text";

    // set max len of shorter string

    // Same strings
    i64 result = cstr_ncmp(str1, str1, 4);
    expect_be(0, result);

    // First longer than second
    result = cstr_ncmp(str1, str2, 4);
    expect_be(0, result);

    // Second longer than first
    result = cstr_ncmp(str2, str1, 4);
    expect_be(0, result);

    // u64 max len

    // Same strings
    result = cstr_ncmp(str1, str1, U64_MAX);
    expect_be(0, result);

    // First longer than second
    result = cstr_ncmp(str1, str2, U64_MAX);
    expect_be(117, result);

    // Second longer than first
    result = cstr_ncmp(str2, str1, U64_MAX);
    expect_be(-117, result);

    return true;
}

static u8 cstr_trim_tests(void) {
    char s1[20] = "string";
    char s2[20] = " \n\t  string";
    char s3[20] = "string   \n";
    char s4[35] = "  \f\r \v\nstring   \n  \r\t\f\v";
    char s5[20] = "";
    char s6[20] = "  \r\n\t\v\f ";

    expect_string("string", cstr_trim(s1));
    expect_string("string", cstr_trim(s2));
    expect_string("string", cstr_trim(s3));
    expect_string("string", cstr_trim(s4));
    expect_string("", cstr_trim(s5));
    expect_string("", cstr_trim(s6));

    return true;
}

static u8 cstr_sub_tests(void) {
    const char* str1 = "Long string for tests!";  // 22
    char str2[25];

    // start
    cstr_sub(str2, str1, 0, 8);
    expect_string("Long str", str2);

    // middle
    cstr_sub(str2, str1, 8, 7);
    expect_string("ing for", str2);

    // end
    cstr_sub(str2, str1, 15, 7);
    expect_string(" tests!", str2);

    // exact match
    cstr_sub(str2, str1, 0, 22);
    expect_string(str1, str2);

    // check null terminators
    cstr_sub(str2, str1, 0, 25);
    expect_string(str1, str2);
    expect_be(22, cstr_len(str2));

    // u64 max
    cstr_sub(str2, str1, 0, U64_MAX);
    expect_string(str2, str1);

    // start overhead
    cstr_sub(str2, str1, 30, 5);
    expect_be(0, str2[0]);

    // zero len
    cstr_sub(str2, str1, 4, 0);
    expect_be(0, str2[0]);

    return true;
}

void string_register_tests(void) {
    test_manager_register_test(cstr_ncmp_tests, "cstr_ncmp tests");
    test_manager_register_test(cstr_trim_tests, "cstr_trim tests");
    test_manager_register_test(cstr_sub_tests, "cstr_sub tests");
}