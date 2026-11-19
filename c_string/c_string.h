#ifndef C_STRING_H
#define C_STRING_H

/**
 * @file c_string.h
 * @brief C-String: Python-style string functions that produce COBJ_STR values.
 *
 * @code
 * cobj parts = cobj_list_value(cobj_string_split("a,b,c", ","));
 * cobj line = cobj_string_join(&parts, " | ");             // a | b | c
 * cobj message = cobj_string_format("%zu parts", cobj_len(&parts));
 * @endcode
 *
 * Inputs are plain C strings: literals, or `cobj_as_cstr(&value)`.
 * Functions that return a cobj give an owned COBJ_STR, or an empty value when
 * an argument is NULL or allocation fails. Case conversion and whitespace are
 * ASCII. This is a utility module: string values are the core's COBJ_STR.
 * Builds on c_list (split) and the iteration protocol (join).
 */

#include "../c_obj.h"
#include "../c_list/c_list.h"

/// printf-style formatting into a new string
cobj cobj_string_format(const char* fmt, ...) COBJ_PRINTF_FMT(1, 2);

/// a followed by b
cobj cobj_string_concat(const char* a, const char* b);

/// s repeated times times; "" for 0
cobj cobj_string_repeat(const char* s, size_t times);

/// Characters from start to end (excluded), Python style: negative positions count from the end
/// and positions out of range are clamped
cobj cobj_string_slice(const char* s, ptrdiff_t start, ptrdiff_t end);

/// s in upper case (ASCII)
cobj cobj_string_upper(const char* s);

/// s in lower case (ASCII)
cobj cobj_string_lower(const char* s);

/// Without the leading and trailing characters found in chars, or whitespace when chars is NULL
cobj cobj_string_strip(const char* s, const char* chars);

/// Every non-overlapping old_value replaced; an empty old_value leaves the string unchanged
cobj cobj_string_replace(const char* s, const char* old_value, const char* new_value);

/// Position of the first sub, -1 when it's missing (or an argument is NULL)
ptrdiff_t cobj_string_find(const char* s, const char* sub);

/// Non-overlapping occurrences of sub; 0 when sub is empty
size_t cobj_string_count(const char* s, const char* sub);

/// True when s begins with prefix
bool cobj_string_starts_with(const char* s, const char* prefix);

/// True when s ends with suffix
bool cobj_string_ends_with(const char* s, const char* suffix);

/**
 * A new list of strings. With a separator, every piece is kept: "a,,b" gives
 * [a, , b], "a," gives [a, ] and "" gives one empty piece. With sep NULL,
 * splits on runs of whitespace and keeps no empty pieces.
 * NULL when s is NULL, sep is "" or allocation fails.
 */
cobj_list* cobj_string_split(const char* s, const char* sep);

/// The elements of any iterable joined with sep. Empty value when an element isn't a string
cobj cobj_string_join(const cobj* iterable, const char* sep);

/// A whole file as a string. Empty value when it can't be read
cobj cobj_string_read_file(const char* path);

/// Writes text to path with fopen's mode ("w", "a"...). COBJ_EIO when the file can't be written
cobj_status cobj_string_write_file(const char* path, const char* text, const char* mode);

#endif //C_STRING_H
