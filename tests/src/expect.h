#pragma once

#include <math/math.h>
#include <strings/string.h>
#include <core/logger.h>


// Expects expected to be equal to actual
#define expect_be(expected, actual) \
    if (actual != expected) { \
        MERROR("--> Expected %lld, but got: %lld! File: %s:%d", expected, actual, __FILE__, __LINE__); \
        return false; \
    }

// Expects expected to NOT be equal to actual
#define expect_not_be(expected, actual) \
    if (actual == expected) { \
        MERROR("--> Expected %d != %d, but they are equal! File: %s:%d", expected, actual, __FILE__, __LINE__); \
        return false; \
    }

// Expects expected to be actual given a tolerance of 0.0001f
#define expect_float(expected, actual) \
    if (mabs(expected - actual) > 0.0001f) { \
        MERROR("--> Expected %f, but got: %f! File: %s:%d", expected, actual, __FILE__, __LINE__); \
        return false; \
    }

// Expects actual to be true
#define expect_true(actual) \
    if (actual != true) { \
        MERROR("--> Expected true, but got: false! File: %s:%d", __FILE__, __LINE__); \
        return false; \
    }

// Expects actual to be false
#define expect_false(actual) \
    if (actual != false) { \
        MERROR("--> Expected false, but got: true! File: %s:%d", __FILE__, __LINE__); \
        return false; \
    }

// Expects expected string to be equal to actual
#define expect_string(expected, actual) \
    if (!cstr_equal(expected, actual)) { \
        MERROR("--> Expected %s, but got: %s! File: %s:%d", expected, actual, __FILE__, __LINE__); \
        return false; \
    }
