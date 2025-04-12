#pragma once

#include "math/math_types.h"


// Gets the number of bytes of the given string, minus the null terminator.
// NOTE: For strings without a null terminator, use cstr_nlen instead.
MAPI u64 cstr_len(const char* str);

// Gets the number of bytes of the given string, minus the null temrimator, but at most max_len.
// This function only ever looks at the bytes pointed to in str up until, but never beyond, max_len - 1.
MAPI u64 cstr_nlen(const char* str, u64 max_len);

// Duplicates the provided string. Note that this allocates new memory.
MAPI char* cstr_duplicate(const char* str);

// Frees the memory of the given string.
MAPI void cstr_free(const char* str);

// Case-sensetive string comparasion, if strings not equal return difference of first two diffent chars, 0 otherwise
MAPI i64 cstr_ncmp(const char* str1, const char* str2, u64 max_len);

// Case-insensetive string comparasion, if strings not equal return difference of first two diffent chars, 0 otherwise
MAPI i64 cstr_ncmpi(const char* str1, const char* str2, u64 max_len);

// Case-sensitive string comparison
MAPI b8 cstr_equal(const char* str1, const char* str2);

// Case-insensitive string comparison
MAPI b8 cstr_equali(const char* str1, const char* str2);

// Case-sensitive string comparison, where comparison stops at max_len
MAPI b8 cstr_nequal(const char* str1, const char* str2, u64 max_len);

// Case-insensitive string comparison, where comparison stops at max_len
MAPI b8 cstr_nequali(const char* str1, const char* str2, u64 max_len);

// Performs string formatting against the given format string and parameters.
// NOTE: This performs a dynamic allocation and should be freed by the caller.
MAPI char* cstr_format(const char* format, ...);

// Performs variadic string formatting against the given format string and va_listp.
// NOTE: This performs a dynamic allocation and should be freed by the caller.
MAPI char* cstr_format_v(const char* format, void* va_listp);

// Empties the provided string by setting the first character to 0
MAPI char* cstr_empty(char* str);

// Copies the string in src to dst. Does not perform any allocations
MAPI char* cstr_copy(char* dst, const char* src);

// Copies the bytes in the src buffer into the dst buffer up to the given length. Does not perform any allocations.
// Any remaining length after a null terminator will be zero-padded unless max_len is U64_MAX.
MAPI char* cstr_ncopy(char* dst, const char* src, u64 max_len);

// Performs an in-place trim of the provided string. This removes all whitespace from both ends of the string
MAPI char* cstr_trim(char* str);

// Gets a substring of the src string between start and len or to the end of the string.
// Done by placing zeros in the string at relevant points.
MAPI void cstr_sub(char* dst, const char* src, u64 start, u64 len);

// Returns the index of the first occurance of c in str. Otherwise U64_MAX
MAPI u64 cstr_index_of(const char* str, char c);

// Returns the index of the last occurance of c in str. Otherwise U64_MAX
MAPI u64 cstr_last_index_of(const char* str, char c);

// Returns the index of the first occurance of sub in str. Otherwise U64_MAX
MAPI u64 cstr_index_of_str(const char* str, const char* sub);

// Indicates if str starts with sub. Case-sensitive.
MAPI b8 cstr_starts_with(const char* str, const char* sub);

// Indicates if str starts with sub. Case-insensitive.
MAPI b8 cstr_starts_withi(const char* str, const char* sub);

// Inserts the supplied char to src in given pos and outputs to dst
MAPI void cstr_insert_char_at(char* dst, const char* src, u64 pos, char c);

// Inserts the supplied str to src in given pos and outputs to dst
MAPI void cstr_insert_str_at(char* dst, const char* src, u64 pos, const char* str);

// Appends sub to src and outputs to dst
MAPI void cstr_append_str(char* dst, const char* src, const char* str);

// Appends the supplied 64-bit float to src and outputs to dst
MAPI void cstr_append_float(char* dst, const char* src, f64 f);

// Appends the supplied 64-bit signed integer to src and outputs to dst
MAPI void cstr_append_int(char* dst, const char* src, i64 i);

// Appends the supplied boolean (as either "true" or "false") to src and outputs to dst
MAPI void cstr_append_bool(char* dst, const char* src, b8 b);

// Appends the supplied character to src and outputs to dst
MAPI void cstr_append_char(char* dst, const char* src, char c);

// Remove the supplied range of characters fom src and outputs to dst.
MAPI void cstr_remove_at(char* dst, const char* src, u64 start, u64 len);

// Splits the given string by the delimiter provided and stores in the provided darray. Optionally trims each entry.
// NOTE: A string allocation occurs for each entry, an MUST be freed by the caller.
MAPI u64 cstr_split(const char* str, char delimiter, char*** str_darray, b8 trim_entries, b8 include_empty);

// Splits the given string by the delimiter provided and stores in the provided darray. Optionally trims each entry.
// NOTE: A string allocation occurs for each entry, an MUST be freed by the caller.
MAPI u64 cstr_nsplit(const char* str, char delimiter, u64 max_count, char*** str_darray, b8 trim_entries, b8 include_empty);

// Clean up string allocations in str_darray, but does not free the void* itself.
MAPI void cstr_cleanup_split_darray(char** str_darray);

// Converts string in-place to lowercase. Regular ASCII and western European high-ascii characters only
MAPI void cstr_to_lower(char* str);

// Converts string in-place to uppercase. Regular ASCII and western European high-ascii characters only
MAPI void cstr_to_upper(char* str);

// Indicates if the provided character is considered whitespace.
MAPI b8 char_is_whitespace(char c);

// Parsing

// Attempts to parse a 4x4 matrix from the provided string.
// NOTE: string should be space delimited (i.e "1.0 1.0 ... 1.0")
MAPI b8 cstr_to_mat4(const char* str, mat4* out_mat);

// Attempts to parse a 3x3 matrix from the provided string.
// NOTE: string should be space delimited (i.e "1.0 1.0 ... 1.0")
MAPI b8 cstr_to_mat3(const char* str, mat3* out_mat);

// Attempts to parse a vector from the provided string.
// NOTE: string should be space delimited (i.e "1.0 2.0 3.0 4.0")
MAPI b8 cstr_to_vec4(const char* str, vec4* out_vec);

// Attempts to parse a vector from the provided string.
// NOTE: string should be space delimited (i.e "1.0 2.0 3.0")
MAPI b8 cstr_to_vec3(const char* str, vec3* out_vec);

// Attempts to parse a vector from the provided string.
// NOTE: string should be space delimited (i.e "1.0 2.0")
MAPI b8 cstr_to_vec2(const char* str, vec2* out_vec);

// Attempts to parse a 32-bit floating-point number from the provided string
MAPI b8 cstr_to_f32(const char* str, f32* out_f);

// Attempts to parse a 64-bit floating-point number from the provided string
MAPI b8 cstr_to_f64(const char* str, f64* out_f);

// Attempts to parse a 64-bit signed integer from the provided string
MAPI b8 cstr_to_i64(const char* str, i64* out_i);

// Attempts to parse a 32-bit signed integer from the provided string
MAPI b8 cstr_to_i32(const char* str, i32* out_i);

// Attempts to parse a 16-bit signed integer from the provided string
MAPI b8 cstr_to_i16(const char* str, i16* out_i);

// Attempts to parse a 8-bit signed integer from the provided string
MAPI b8 cstr_to_i8(const char* str, i8* out_i);

// Attempts to parse a 64-bit unsigned integer from the provided string
MAPI b8 cstr_to_u64(const char* str, u64* out_u);

// Attempts to parse a 32-bit unsigned integer from the provided string
MAPI b8 cstr_to_u32(const char* str, u32* out_u);

// Attempts to parse a 16-bit unsigned integer from the provided string
MAPI b8 cstr_to_u16(const char* str, u16* out_u);

// Attempts to parse a 8-bit unsigned integer from the provided string
MAPI b8 cstr_to_u8(const char* str, u8* out_u);

// Attempts to parse a boolean from the provided string. "true" or "1" are considered true, anything else is false
MAPI b8 cstr_to_bool(const char* str, b8* out_b);

// To string

// Creates a string representation of the provided matrix.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* mat4_to_cstr(mat4 m);

// Creates a string representation of the provided matrix.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* mat3_to_cstr(mat3 m);

// Creates a string representation of the provided vector.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* vec4_to_cstr(vec4 v);

// Creates a string representation of the provided vector.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* vec3_to_cstr(vec3 v);

// Creates a string representation of the provided vector.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* vec2_to_cstr(vec2 v);

// Creates a string representation of the provided 32-bit float.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* f32_to_cstr(f32 f);

// Creates a string representation of the provided 64-bit float.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* f64_to_cstr(f64 f);

// Creates a string representation of the provided 64-bit signed integer.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* i64_to_cstr(i64 i);

// Creates a string representation of the provided 32-bit signed integer.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* i32_to_cstr(i32 i);

// Creates a string representation of the provided 16-bit signed integer.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* i16_to_cstr(i16 i);

// Creates a string representation of the provided 8-bit signed integer.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* i8_to_cstr(i8 i);

// Creates a string representation of the provided 64-bit unsigned integer.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* u64_to_cstr(u64 u);

// Creates a string representation of the provided 32-bit unsigned integer.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* u32_to_cstr(u32 u);

// Creates a string representation of the provided 16-bit unsigned integer.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* u16_to_cstr(u16 u);

// Creates a string representation of the provided 8-bit unsigned integer.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* u8_to_cstr(u8 u);

// Creates a string representation of the provided boolean, i.e. "false" for false/0 and "true" for everything else.
// NOTE: string is dynamically allocated, so the caller should free it
MAPI const char* bool_to_cstr(b8 b);

// UTF-8

// Gets the length of a string in UTF-8 (potentially multibyte) characters, minus the null terminator.
// NOTE: For strings without a null terminator, use cstr_utf8_nlen instead.
MAPI u64 cstr_utf8_len(const char* str);

// Gets the length of a string in UTF-8 (potentially multibyte) characters, minus the null terminator, but at most max_len.
// This function only ever looks at the bytes pointed to in str up until, but never beyond, max_len - 1.
MAPI u64 cstr_utf8_nlen(const char* str, u64 max_len);

// Obtain bytes needed from the byte array to form a UTF-8 codepoint,
// also providing how many bytes the current character is.
MAPI b8 bytes_to_codepoint(const char* bytes, u32 offset, i32* out_codepoint, u8* out_advance);

// Indicates if the provided codepoint is considered whitespace.
MAPI b8 codepoint_is_whitespace(i32 codepoint);

// Indicates if the provided codepoint is lower-case. Regular ASCII and western European high-ascii characters only
MAPI b8 codepoint_is_lower(i32 codepoint);

// Indicates if the provided codepoint is upper-case. Regular ASCII and western European high-ascii characters only
MAPI b8 codepoint_is_upper(i32 codepoint);

// Indicates if the provided codepoint is alpha-numeric. Regular ASCII and western European high-ascii characters only
MAPI b8 codepoint_is_alpha(i32 codepoint);

// Indicates if the provided codepoint is numeric. Regular ASCII and western European high-ascii characters only
MAPI b8 codepoint_is_numeric(i32 codepoint);

// Indicates if the given codepoint is considered to be a space. Includes ' ', \f, \r, \n \t and \v
MAPI b8 codepoint_is_space(i32 codepoint);

// Path utils

// Extracts the directory path from al file path
// I.e "assets/textures/image.png" -> "assets/textures"
MAPI void cstr_directory_from_path(char* dst, const char* path);

// Extracts the directory name from a file path
// I.e "assets/textures/image.png" -> "textures"
MAPI void cstr_dirname_from_path(char* dst, const char* path);

// Extracts the filename (including file extension) from a file path
// I.e "assets/textures/image.png" -> "image.png"
MAPI void cstr_filename_from_path(char* dst, const char* path);

// Extracts the filename (excluding file extension) from a file path
// I.e "assets/textures/image.png" -> "image"
MAPI void cstr_filename_no_ext_from_path(char* dst, const char* path);

// Attempts to get the file extension from the given path
// I.e "assets/textures/image.png" -> "png" or ".png"
MAPI const char* cstr_ext_from_path(const char* path, b8 include_dot);