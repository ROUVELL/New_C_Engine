#include "string.h"

#include <stdarg.h>  // for variadic functions
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>   // vsnprintf, sscanf, sprintf

#define USE_STD_STR 1

#if USE_STD_STR
#   include <string.h>
#endif

#include "containers/darray.h"
#include "memory/memory.h"
#include "core/logger.h"


u64 cstr_len(const char* str) {
#if USE_STD_STR
    return strlen(str);
#else
    return cstr_nlen(str, U64_MAX);
#endif
}

u64 cstr_nlen(const char* str, u64 max_len) {
#if USE_STD_STR
    return strnlen(str, max_len);
#else
    if (!str) {
        return 0;
    }

    u64 len = 0;
    for (; len < max_len; ++len) {
        if (!str[len]) {
            break;
        }
    }

    return len;
#endif
}

char* cstr_duplicate(const char* str) {
    if (!str) {
        MWARN("cstr_duplicate - Called with an empty string! nullptr will be retured!");
        return nullptr;
    }

    u64 len = cstr_len(str);
    char* copy = memory_allocate(len + 1, MEMORY_TAG_STRING);
    memory_copy(copy, str, len);
    copy[len] = '\0';

    return copy;
}

void cstr_free(const char* str) {
    if (!str) {
        MWARN("cstr_free - Called with an empty string! Nothing was done");
        return;
    }

    u64 len = cstr_len(str);
    memory_free((char*)str, len + 1, MEMORY_TAG_STRING);
}

i64 cstr_ncmp(const char* str1, const char* str2, u64 max_len) {
    u64 str1_len = cstr_nlen(str1, max_len);
    u64 str2_len = cstr_nlen(str2, max_len);

    u64 min_len = MMIN(str1_len, str2_len);

    if (min_len < U64_MAX) {
        ++min_len;
    }

    for (u64 i = 0; i < min_len; ++i) {
        if ((!str1[i] || !str2[i]) && i == max_len) {
            return 0;
        }

        i64 result = str1[i] - str2[i];
        if (result) {
            return result;
        }
    }

    return 0;
}

i64 cstr_ncmpi(const char* str1, const char* str2, u64 max_len) {
#if USE_STD_STR
#if PLATFORM_WINDOWS
    return (i64)_strnicmp(str1, str2, max_len);
#else
    return (i64)strncasecmp(str1, str2, max_len);
#endif
#else
    char* lower1 = nullptr;
    char* lower2 = nullptr;

    if (str1) {
        lower1 = cstr_duplicate(str1);
        cstr_to_lower(lower1);
    }

    if (str2) {
        lower2 = cstr_duplicate(str2);
        cstr_to_lower(lower2);
    }

    i64 result = cstr_ncmp(lower1, lower2, max_len);

    if (lower1) {
        cstr_free(lower1);
    }

    if (lower2) {
        cstr_free(lower2);
    }

    return result;
#endif
}

b8 cstr_equal(const char* str1, const char* str2) {
    return cstr_ncmp(str1, str2, U64_MAX) == 0;
}

b8 cstr_equali(const char* str1, const char* str2) {
    return cstr_ncmpi(str1, str2, U64_MAX) == 0;
}

b8 cstr_nequal(const char* str1, const char* str2, u64 max_len) {
    return cstr_ncmp(str1, str2, max_len) == 0;
}

b8 cstr_nequali(const char* str1, const char* str2, u64 max_len) {
    return cstr_ncmpi(str1, str2, max_len) == 0;
}

char* cstr_format(const char* format, ...) {
    if (!format) {
        return nullptr;
    }

    __builtin_va_list arg_ptr;
    va_start(arg_ptr, format);
    char* result = cstr_format_v(format, arg_ptr);
    va_end(arg_ptr);

    return result;
}

char* cstr_format_v(const char* format, void* va_listp) {
    if (!format) {
        return nullptr;
    }

    va_list list_copy;
#ifdef _MSC_VER
    list_copy = va_listp;
#else
    va_copy(list_copy, va_listp);
#endif
    i32 len = vsnprintf(nullptr, 0, format, list_copy);
    va_end(list_copy);

    char* buffer = memory_allocate(len + 1, MEMORY_TAG_STRING);
    if (!buffer) {
        return nullptr;
    }

    vsnprintf(buffer, len + 1, format, va_listp);
    buffer[len] = '\0';
    return buffer;
}

char* cstr_empty(char* str) {
    if (str) {
        str[0] = '\0';
    }

    return str;
}

char* cstr_copy(char* dst, const char* src) {
    return cstr_ncopy(dst, src, U64_MAX);
}

char* cstr_ncopy(char* dst, const char* src, u64 max_len) {
    if (!dst) {
        MERROR("cstr_ncopy - Called without dst, wich is required! nullptr will be returned!");
        return nullptr;
    }

    if (!src) {
        MERROR("cstr_ncopy - Called without src, wich is required! nullptr will be returned!");
        return nullptr;
    }

    if (max_len == 0) {
        return dst;
    }

    u64 src_len = MMIN(cstr_len(src) + 1, max_len);
    memory_copy(dst, src, src_len);

    if (max_len != U64_MAX) {
        i64 diff = (i64)max_len - (i64)src_len;
        if (diff > 0) {
            memory_zero(dst + max_len, (u64)diff);
        }
    }

    return dst;
}

char* cstr_trim(char* str) {
    if (!str) {
        return 0;
    }

    while (codepoint_is_space((i32)*str)) {
        ++str;
    }

    if (*str) {
        char* p = str;
        while (*p) {
            ++p;
        }

        while (p > str && codepoint_is_space((i32)*(--p)));

        *(p + 1) = '\0';
    }

    return str;
}

void cstr_sub(char* dst, const char* src, u64 start, u64 len) {
    if (!dst || !src) {
        return;
    }

    if (len == 0) {
        dst[0] = '\0';
        return;
    }

    u64 src_len = cstr_len(src);
    if (start >= src_len) {
        dst[0] = '\0';
        return;
    }

    // NOTE: memory_copy ?

    // NOTE: Can be optimized
    u64 j = 0;
    for (u64 i = start; j < len && src[i]; ++i, ++j) {
        dst[j] = src[i];
    }
    dst[j] = '\0';
}

u64 cstr_index_of(const char* str, char c) {
    if (!str) {
        return U64_MAX;
    }

    u64 len = cstr_len(str);
    for (u64 i = 0; i < len; ++i) {
        if (str[i] == c) {
            return i;
        }
    }

    return U64_MAX;
}

u64 cstr_last_index_of(const char* str, char c) {
    if (!str) {
        return U64_MAX;
    }

    u64 len = cstr_len(str);
    for (u64 i = len - 1; i > 0; --i) {
        if (str[i] == c) {
            return i;
        }
    }

    return U64_MAX;
}

u64 cstr_index_of_str(const char* str, const char* sub) {
    if (!str || !sub) {
        return U64_MAX;
    }

    u64 str_len = cstr_len(str);
    u64 sub_len = cstr_len(sub);
    const char* a = str;
    const char* b = sub;
    if (sub_len > str_len) {
        u64 tmp = str_len;
        str_len = sub_len;
        sub_len = tmp;
        a = sub;
        b = str;
    }

    if (str_len == 0 || sub_len == 0) {
        return U64_MAX;
    }

    for (u64 i = 0; i < str_len; ++i) {
        if (a[i] == b[0]) {
            b8 keep_looking = false;
            for (u64 j = 0; j < sub_len; ++j) {
                if (a[i + j] != b[j]) {
                    keep_looking = true;
                    break;
                }
            }
            if (!keep_looking) {
                return i;
            }
        }
    }

    return U64_MAX;
}

b8 cstr_starts_with(const char* str, const char* sub) {
    if (!str || !sub) {
        return false;
    }

    u64 str_len = cstr_len(str);
    u64 sub_len = cstr_len(sub);
    if (str_len < sub_len) {
        return false;
    }

    return cstr_nequal(str, sub, sub_len);
}

b8 cstr_starts_withi(const char* str, const char* sub) {
    if (!str || !sub) {
        return false;
    }

    u64 str_len = cstr_len(str);
    u64 sub_len = cstr_len(sub);
    if (str_len < sub_len) {
        return false;
    }

    return cstr_nequali(str, sub, sub_len);
}

void cstr_insert_char_at(char* dst, const char* src, u64 pos, char c) {
    if (!dst || !src) {
        return;
    }

    u64 src_len = cstr_len(src);
    u64 remaining = src_len - pos;
    if (pos > 0) {
        memory_copy(dst, src, sizeof(char) * pos);
    }

    if (pos < src_len) {
        memory_copy(dst + pos + 1, src + pos, sizeof(char) * remaining);
    }

    dst[pos] = c;
    
}

void cstr_insert_str_at(char* dst, const char* src, u64 pos, const char* str) {
    if (!dst || !src) {
        return;
    }

    u64 src_len = cstr_len(src);
    u64 str_len = cstr_len(str);
    u64 remaining = src_len - pos;
    if (pos > 0) {
        memory_copy(dst, src, sizeof(char) * pos);
    }

    if (pos < src_len) {
        memory_copy(dst + pos + str_len, src + pos, sizeof(char) * remaining);
    }

    memory_copy(dst + pos, str, sizeof(char) * str_len);
}

void cstr_append_str(char* dst, const char* src, const char* str) {
    sprintf(dst, "%s%s", src, str);
}

void cstr_append_float(char* dst, const char* src, f64 f) {
    sprintf(dst, "%s%lf", src, f);
}

void cstr_append_int(char* dst, const char* src, i64 i) {
    sprintf(dst, "%s%lli", src, i);
}

void cstr_append_bool(char* dst, const char* src, b8 b) {
    sprintf(dst, "%s%s", src, b ? "true" : "false");
}

void cstr_append_char(char* dst, const char* src, char c) {
    sprintf(dst, "%s%c", src, c);
}

void cstr_remove_at(char* dst, const char* src, u64 start, u64 len) {
    if (!dst || !src) {
        return;
    }

    u64 src_len = cstr_len(src);
    u64 remaining = src_len - start - len;
    if (start > 0) {
        memory_copy(dst, src, sizeof(char) * start);
    }

    if (start < src_len) {
        memory_copy(dst + start, src + start + len, sizeof(char) * remaining);
    }

    dst[src_len - len] = '\0';
}

u64 cstr_split(const char* str, char delimiter, char*** str_darray, b8 trim_entries, b8 include_empty) {
    return cstr_nsplit(str, delimiter, U64_MAX, str_darray, trim_entries, include_empty);
}

u64 cstr_nsplit(const char* str, char delimiter, u64 max_count, char*** str_darray, b8 trim_entries, b8 include_empty) {
    if (!str || !str_darray || max_count == 0) {
        return 0;
    }

    char* result = nullptr;
    u64 trimmed_len = 0;
    u64 entry_count = 0;
    char buffer[16384] = {0};
    u64 current_len = 0;

    u64 str_len = cstr_len(str);
    for (u64 i = 0; i < str_len; ++i) {
        char c = str[i];

        if (c == delimiter) {
            buffer[current_len] = '\0';
            result = buffer;
            trimmed_len = current_len;
            if (trim_entries && current_len > 0) {
                result = cstr_trim(result);
                trimmed_len = cstr_len(result);
            }

            if (trimmed_len > 0 || include_empty) {
                char* entry = memory_allocate(sizeof(char) * (trimmed_len + 1), MEMORY_TAG_STRING);
                if (trimmed_len > 0) {
                    cstr_ncopy(entry, result, trimmed_len);
                }
                entry[trimmed_len] = '\0';

                char** a = *str_darray;
                darray_push(a, entry);
                *str_darray = a;
                entry_count++;
            }

            if (entry_count == max_count) {
                return entry_count;
            }

            memory_zero(buffer, sizeof(char) * 16384);
            current_len = 0;
            continue;
        }

        buffer[current_len] = c;
        current_len++;
    }

    result = buffer;
    trimmed_len = current_len;
    if (trim_entries && current_len > 0) {
        result = cstr_trim(result);
        trimmed_len = cstr_len(result);
    }

    if (trimmed_len > 0 || include_empty) {
        char* entry = memory_allocate(sizeof(char) * (trimmed_len + 1), MEMORY_TAG_STRING);
        if (trimmed_len > 0) {
            cstr_ncopy(entry, result, trimmed_len);
        }
        entry[trimmed_len] = '\0';

        char** a = *str_darray;
        darray_push(a, entry);
        *str_darray = a;
        entry_count++;
    }

    return entry_count;
}

void cstr_cleanup_split_darray(char** str_darray) {
    if (str_darray) {
        u64 count = darray_length(str_darray);
        for (u64 i = 0; i < count; ++i) {
            u64 len = cstr_len(str_darray[i]);
            memory_free(str_darray[i], sizeof(char) * (len + 1), MEMORY_TAG_STRING);
        }

        darray_clear(str_darray);
    }
}

void cstr_to_lower(char* str) {
    for (u32 i = 0; str[i]; ++i) {
        if (codepoint_is_upper(str[i])) {
            str[i] += ('a' - 'A');
        }
    }
}

void cstr_to_upper(char* str) {
    for (u32 i = 0; str[i]; ++i) {
        if (codepoint_is_lower(str[i])) {
            str[i] -= ('a' - 'A');
        }
    }
}

b8 char_is_whitespace(char c) {
    switch (c) {
        case ' ':
        case '\r':
        case '\t':
        case '\n':
        case '\v':
        case '\f':
            return true;
        default:
            return false;
    }
}

b8 cstr_to_mat4(const char* str, mat4* out_mat) {
    if (!str || !out_mat) {
        return false;
    }

    memory_zero(out_mat, sizeof(mat4));
    i32 result = sscanf(str, "%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
                        &out_mat->data[0], &out_mat->data[1], &out_mat->data[2], &out_mat->data[3],
                        &out_mat->data[4], &out_mat->data[5], &out_mat->data[6], &out_mat->data[7],
                        &out_mat->data[8], &out_mat->data[9], &out_mat->data[10], &out_mat->data[11],
                        &out_mat->data[12], &out_mat->data[13], &out_mat->data[14], &out_mat->data[15]);
    
    return result != -1;
}

b8 cstr_to_mat3(const char* str, mat3* out_mat) {
    if (!str || !out_mat) {
        return false;
    }

    memory_zero(out_mat, sizeof(mat3));
    i32 result = sscanf(str, "%f %f %f %f %f %f %f %f %f",
                        &out_mat->data[0], &out_mat->data[1], &out_mat->data[2],
                        &out_mat->data[3], &out_mat->data[4], &out_mat->data[5],
                        &out_mat->data[6], &out_mat->data[7], &out_mat->data[8]);
    
    return result != -1;
}

b8 cstr_to_vec4(const char* str, vec4* out_vec) {
    if (!str || !out_vec) {
        return false;
    }

    memory_zero(out_vec, sizeof(vec4));
    i32 result = sscanf(str, "%f %f %f %f", &out_vec->x, &out_vec->y, &out_vec->z, &out_vec->w);
    return result != -1;
}

b8 cstr_to_vec3(const char* str, vec3* out_vec) {
    if (!str || !out_vec) {
        return false;
    }

    memory_zero(out_vec, sizeof(vec3));
    i32 result = sscanf(str, "%f %f %f", &out_vec->x, &out_vec->y, &out_vec->z);
    return result != -1;
}

b8 cstr_to_vec2(const char* str, vec2* out_vec) {
    if (!str || !out_vec) {
        return false;
    }

    memory_zero(out_vec, sizeof(vec2));
    i32 result = sscanf(str, "%f %f", &out_vec->x, &out_vec->y);
    return result != -1;
}

b8 cstr_to_f32(const char* str, f32* out_f) {
    if (!str || !out_f) {
        return false;
    }

    *out_f = 0.0f;
    i32 result = sscanf(str, "%f", out_f);
    return result != -1;
}

b8 cstr_to_f64(const char* str, f64* out_f) {
    if (!str || !out_f) {
        return false;
    }

    *out_f = 0.0;
    i32 result = sscanf(str, "%lf", out_f);
    return result != -1;
}

b8 cstr_to_i64(const char* str, i64* out_i) {
    if (!str || !out_i) {
        return false;
    }

    *out_i = 0;
    i32 result = sscanf(str, "%lli", out_i);
    return result != -1;
}

b8 cstr_to_i32(const char* str, i32* out_i) {
    if (!str || !out_i) {
        return false;
    }

    *out_i = 0;
    i32 result = sscanf(str, "%i", out_i);
    return result != -1;
}

b8 cstr_to_i16(const char* str, i16* out_i) {
    if (!str || !out_i) {
        return false;
    }

    *out_i = 0;
    i32 result = sscanf(str, "%hi", out_i);
    return result != -1;
}

b8 cstr_to_i8(const char* str, i8* out_i) {
    if (!str || !out_i) {
        return false;
    }

    *out_i = 0;
    i32 result = sscanf(str, "%hhi", out_i);
    return result != -1;
}

b8 cstr_to_u64(const char* str, u64* out_u) {
    if (!str || !out_u) {
        return false;
    }

    *out_u = 0;
    i32 result = sscanf(str, "%llu", out_u);
    return result != -1;
}

b8 cstr_to_u32(const char* str, u32* out_u) {
    if (!str || !out_u) {
        return false;
    }

    *out_u = 0;
    i32 result = sscanf(str, "%u", out_u);
    return result != -1;
}

b8 cstr_to_u16(const char* str, u16* out_u) {
    if (!str || !out_u) {
        return false;
    }

    *out_u = 0;
    i32 result = sscanf(str, "%hu", out_u);
    return result != -1;
}

b8 cstr_to_u8(const char* str, u8* out_u) {
    if (!str || !out_u) {
        return false;
    }

    *out_u = 0;
    i32 result = sscanf(str, "%hhu", out_u);
    return result != -1;
}

b8 cstr_to_bool(const char* str, b8* out_b) {
    if (!str || !out_b) {
        return false;
    }

    *out_b = cstr_equal(str, "1") || cstr_equal(str, "true");
    return true;
}

const char* mat4_to_cstr(mat4 m) {
    return cstr_format("%f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f",
        m.data[0], m.data[1], m.data[2], m.data[3],
        m.data[4], m.data[5], m.data[6], m.data[7],
        m.data[8], m.data[9], m.data[10], m.data[11],
        m.data[12], m.data[13], m.data[14], m.data[15]);
}

const char* mat3_to_cstr(mat3 m) {
    return cstr_format("%f %f %f %f %f %f %f %f %f",
        m.data[0], m.data[1], m.data[2],
        m.data[3], m.data[4], m.data[5],
        m.data[6], m.data[7], m.data[8]);
}

const char* vec4_to_cstr(vec4 v) {
    return cstr_format("%f %f %f %f", v.x, v.y, v.z, v.w);
}

const char* vec3_to_cstr(vec3 v) {
    return cstr_format("%f %f %f", v.x, v.y, v.z);
}

const char* vec2_to_cstr(vec2 v) {
    return cstr_format("%f %f", v.x, v.y);
}

const char* f32_to_cstr(f32 f) {
    return cstr_format("%f", f);
}

const char* f64_to_cstr(f64 f) {
    return cstr_format("%lf", f);
}

const char* i64_to_cstr(i64 i) {
    return cstr_format("%lli", i);
}

const char* i32_to_cstr(i32 i) {
    return cstr_format("%i", i);
}

const char* i16_to_cstr(i16 i) {
    return cstr_format("%hi", i);
}

const char* i8_to_cstr(i8 i) {
    return cstr_format("%hhi", i);
}

const char* u64_to_cstr(u64 u) {
    return cstr_format("%llu", u);
}

const char* u32_to_cstr(u32 u) {
    return cstr_format("%u", u);
}

const char* u16_to_cstr(u16 u) {
    return cstr_format("%hu", u);
}

const char* u8_to_cstr(u8 u) {
    return cstr_format("%hhu", u);
}

const char* bool_to_cstr(b8 b) {
    return cstr_duplicate(b ? "true" : "false");
}

// UTF-8

u64 cstr_utf8_len(const char* str) {
    return cstr_utf8_nlen(str, U64_MAX);
}

u64 cstr_utf8_nlen(const char* str, u64 max_len) {
    u64 len = 0;
    for (u64 i = 0; len < max_len; ++i, ++len) {
        i32 c = (i32)str[i];
        if (c == 0) {
            break;
        }

        if (c >= 0 && c < 127) {
            // Normal ascii character, don't increment again
        } else if ((c & 0xE0) == 0xC0) {
            // Double-byte character, increment once more
            i += 1;
        } else if ((c & 0xF0) == 0xE0) {
            // Triple-byte character
            i += 2;
        } else if ((c & 0xF8) == 0xF0) {
            // 4-byte character
            i += 3;
        } else {
            MERROR("cstr_utf8_nlen - Not supporting 5 and 6-byte characters! Invalid UTF-8!");
            return 0;
        }
    }

    return len;
}

b8 bytes_to_codepoint(const char* bytes, u32 offset, i32* out_codepoint, u8* out_advance) {
    i32 codepoint = (i32)bytes[offset];

    if (codepoint >= 0 && codepoint < 0x7F) {
        // Single-byte
        *out_advance = 1;
    } else if ((codepoint & 0xE0) == 0xC0) {
        // Double-byte
        codepoint = ((bytes[offset + 0] & 0b00011111) << 6) +
                    (bytes[offset + 1] & 0b00111111);
        *out_advance = 2;
    } else if ((codepoint & 0xF0) == 0xE0) {
        // Triple-byte
        codepoint = ((bytes[offset + 0] & 0b00001111) << 12) +
                    ((bytes[offset + 1] & 0b00111111) << 6) +
                    (bytes[offset + 2] & 0b00111111);
        *out_advance = 3;
    } else if ((codepoint & 0xF8) == 0xF0) {
        // 4-byte
        codepoint = ((bytes[offset + 0] & 0b00000111) << 18) +
                    ((bytes[offset + 1] & 0b00111111) << 12) +
                    ((bytes[offset + 2] & 0b00111111) << 6) +
                    (bytes[offset + 3] & 0b00111111);
        *out_advance = 4;
    } else {
        *out_advance = 0;
        *out_codepoint = 0;
        MERROR("bytes_to_codepoint = Not supporting 5 and 6-byte characters! Invalid UTF-8!");
        return false;
    }

    *out_codepoint = codepoint;
    return true;
}

b8 codepoint_is_whitespace(i32 codepoint) {
    // Source of whitespace characters:
    switch (codepoint) {
        case 0x0009: //  character tabulation (\t)
        case 0x000A: // line feed (\n)
        case 0x000B: // line tabulation/vertical tab (\v)
        case 0x000C: // form feed (\f)
        case 0x000D: // carriage return (\r)
        case 0x0020: // space (' ')
        case 0x0085: // next line
        case 0x00A0: // no-break space
        case 0x1680: // ogham space mark
        case 0x180E: // mongolian vowel separator
        case 0x2000: // en quad
        case 0x2001: // em quad
        case 0x2002: // en space
        case 0x2003: // em space
        case 0x2004: // three-per-em space
        case 0x2005: // four-per-em space
        case 0x2006: // six-per-em space
        case 0x2007: // figure space
        case 0x2008: // punctuation space
        case 0x2009: // thin space
        case 0x200A: // hair space
        case 0x200B: // zero width space
        case 0x200C: // zero width non-joiner
        case 0x200D: // zero width joiner
        case 0x2028: // line separator
        case 0x2029: // paragraph separator
        case 0x202F: // narrow no-break space
        case 0x205F: // medium mathematical space
        case 0x2060: // word joiner
        case 0x3000: // ideographic space
        case 0xFEFF: // zero width non-breaking space
            return true;
        default:
            return false;
        }
}

b8 codepoint_is_lower(i32 codepoint) {
    return (codepoint >= 'a' && codepoint <= 'z') ||
           (codepoint >= 0xE0 && codepoint <= 0xFF);
}

b8 codepoint_is_upper(i32 codepoint) {
    return (codepoint >= 'A' && codepoint <= 'Z') ||
           (codepoint >= 0xC0 && codepoint <= 0xDF);
}

b8 codepoint_is_alpha(i32 codepoint) {
    return (codepoint >= 'a' && codepoint <= 'z') ||
           (codepoint >= 'A' && codepoint <= 'Z') ||
           (codepoint >= 0xC0 && codepoint <= 0xFF);
}

b8 codepoint_is_numeric(i32 codepoint) {
    return codepoint >= '0' && codepoint <= '9';
}

b8 codepoint_is_space(i32 codepoint) {
    switch (codepoint)
    {
    case ' ':   // regular space
    case '\n':  // newline
    case '\r':  // carriage return
    case '\f':  // form feed
    case '\t':  // horizontal tab
    case '\v':  // vertical tab
        return true;
    default:
        return false;
    }
}

void cstr_directory_from_path(char* dst, const char* path) {
    u64 path_len = cstr_len(path);
    for (i64 i = (i64)path_len; i >= 0; --i) {
        char c = path[i];
        if (c == '/' || c == '\\') {
            cstr_ncopy(dst, path, (u64)i + 1);
            dst[i + 1] = '\0';
            return;
        }
    }
}

void cstr_dirname_from_path(char* dst, const char* path) {
    u64 path_len = cstr_len(path);
    u64 left = 0;
    u64 right = 0;

    for (i64 i = (i64)path_len; i >= 0; --i) {
        char c = path[i];
        if (right == 0 && (c == '/' || c == '\\')) {
            right = i;
        }
        if (left == 0 && (c == '/' || c == '\\')) {
            left = i + 1;
            break;
        }
    }

    cstr_sub(dst, path, left, right - left);
}

void cstr_filename_from_path(char* dst, const char* path) {
    u64 path_len = cstr_len(path);
    for (i64 i = (i64)path_len; i >= 0; --i) {
        char c = path[i];
        if (c == '/' || c == '\\') {
            cstr_copy(dst, path + i + 1);
            return;
        }
    }
}

void cstr_filename_no_ext_from_path(char* dst, const char* path) {
    u64 path_len = cstr_len(path);
    u64 start = 0;
    u64 end = 0;

    for (i64 i = (i64)path_len; i >= 0; --i) {
        char c = path[i];
        if (end == 0 && c == '.') {
            end = i;
        }
        if (start == 0 && (c == '/' || c == '\\')) {
            start = i + 1;
            break;
        }
    }

    cstr_sub(dst, path, start, end - start);
}

const char* cstr_ext_from_path(const char* path, b8 include_dot) {
    if (!path) {
        return nullptr;
    }

    u64 start = cstr_last_index_of(path, '.');
    if (start == U64_MAX) {
        return nullptr;
    }

    if (!include_dot) {
        start++;
    }

    u64 ext_len = cstr_len(path) - start;
    char* ext = memory_allocate(sizeof(char) * (ext_len + 1), MEMORY_TAG_STRING);
    cstr_sub(ext, path, start, ext_len); // TODO: just memory_copy ?
    return ext;
}
