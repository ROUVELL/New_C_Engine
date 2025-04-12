#pragma once

// Compiler check
#if !defined(__clang__) && !defined(__gcc__) && !defined(_MSC_VER)
#error "Unsupported compiler - use clang, gcc or MSCV!"
#endif

// Build mode
#ifdef _DEBUG
#define ENGINE_DEBUG 1
#define ENGINE_RELEASE 0
#else
#define ENGINE_DEBUG 0
#define ENGINE_RELEASE 1
#endif

// Static assertions
#if defined(__clang__) || defined(__gcc__)
#define STATIC_ASSERT _Static_assert
#else
#define STATIC_ASSERT static_assert
#endif

// Platform detection
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#define PLATFORM_WINDOWS
#ifndef _WIN64
#error "64-bit is required on Windows!"
#endif
#elif defined(__linux__) || defined(__gnu_linux__)
#define PLATFORM_LINUX
#if defined(__ANDROID__)
#error "Android unsupported yet!"
#endif
#elif defined(__unix__) || defined(_POSIX_VERSION)
#error "Unix/Posix unsupported yet!"
#elif __APPLE__
#error "Apple/Mac/Ios unsupported yet!"
#else
#error "Unknown platform!"
#endif

// Export/Import
#ifdef _EXPORT
#ifdef _MSC_VER
#define MAPI __declspec(dllexport)
#else
#define MAPI __attribute__((visibility("default")))
#endif
#else
#ifdef _MSC_VER
#define MAPI __declspec(dllimport)
#else
#define MAPI
#endif
#endif

// Inlining
#if defined(__clang__) || defined(__gcc__)
#define MINLINE __attribute__((always_inline)) inline
#define MNOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define MINLINE __forceinline
#define MNOINLINE __declspec(noinline)
#else
#define MINLINE static inline
#define MNOINLINE
#endif

// Deprecation
#if defined(__clang__) || defined(__gcc__)
#define MDEPRECATED(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER)
#define MDEPRECATED(msg) __declspec(deprecated(msg))
#endif

// Typedefines

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

typedef float f32;
typedef double f64;

typedef int b32;
typedef _Bool b8;

#define true 1
#define false 0

#define nullptr ((void*)0)

// Ensure all types are of the correct size.

STATIC_ASSERT(sizeof(u8) == 1, "Expected u8 to be 1 byte.");
STATIC_ASSERT(sizeof(u16) == 2, "Expected u16 to be 2 bytes.");
STATIC_ASSERT(sizeof(u32) == 4, "Expected u32 to be 4 bytes.");
STATIC_ASSERT(sizeof(u64) == 8, "Expected u64 to be 8 bytes.");

STATIC_ASSERT(sizeof(i8) == 1, "Expected i8 to be 1 byte.");
STATIC_ASSERT(sizeof(i16) == 2, "Expected i16 to be 2 bytes.");
STATIC_ASSERT(sizeof(i32) == 4, "Expected i32 to be 4 bytes.");
STATIC_ASSERT(sizeof(i64) == 8, "Expected i64 to be 8 bytes.");

STATIC_ASSERT(sizeof(f32) == 4, "Expected f32 to be 4 bytes.");
STATIC_ASSERT(sizeof(f64) == 8, "Expected f64 to be 8 bytes.");

// Global constants

#define I8_MAX 127
#define I16_MAX 32767
#define I32_MAX 2147483647
#define I64_MAX 9223372036854775807L
#define I8_MIN -128
#define I16_MIN -32768
#define I32_MIN -2147483648
#define I64_MIN -9223372036854775808L

#define U64_MAX 18446744073709551615UL
#define U32_MAX 4294967295U
#define U16_MAX 65535U
#define U8_MAX 255U
#define U64_MIN 0UL
#define U32_MIN 0U
#define U16_MIN 0U
#define U8_MIN 0U

#define INVALID_ID_U64 U64_MAX
#define INVALID_ID_U32 U32_MAX
#define INVALID_ID_U16 U16_MAX
#define INVALID_ID_U8 U8_MAX
#define INVALID_ID INVALID_ID_U32

/** @brief Gets the number of bytes from amount of gibibytes (GiB) (1024*1024*1024) */
#define GIBIBYTES(amount) ((amount) * 1024ULL * 1024ULL * 1024ULL)
/** @brief Gets the number of bytes from amount of mebibytes (MiB) (1024*1024) */
#define MEBIBYTES(amount) ((amount) * 1024ULL * 1024ULL)
/** @brief Gets the number of bytes from amount of kibibytes (KiB) (1024) */
#define KIBIBYTES(amount) ((amount) * 1024ULL)

/** @brief Gets the number of bytes from amount of gigabytes (GB) (1000*1000*1000) */
#define GIGABYTES(amount) ((amount) * 1000ULL * 1000ULL * 1000ULL)
/** @brief Gets the number of bytes from amount of megabytes (MB) (1000*1000) */
#define MEGABYTES(amount) ((amount) * 1000ULL * 1000ULL)
/** @brief Gets the number of bytes from amount of kilobytes (KB) (1000) */
#define KILOBYTES(amount) ((amount) * 1000ULL)

// Some functions

MINLINE u64 get_aligned(u64 operand, u64 granularity) {
    return ((operand + (granularity - 1)) & ~(granularity - 1));
}

#define MMIN(a, b) ((a) < (b) ? a : b)
#define MMAX(a, b) ((a) > (b) ? a : b)
#define MCLAMP(x, min, max) (((x) < (min)) ? min : ((x) > (max)) ? max : x)

#define ARRAY_LENGTH(arr) (sizeof(arr) / sizeof(arr[0]))
#define MSIGN(x) (((x) < 0) ? -1 : ((x) > 0) ? +1 : 0)

#define BIT(x) (1 << (x))