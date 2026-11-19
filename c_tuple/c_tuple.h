#ifndef C_TUPLE_H
#define C_TUPLE_H

/**
 * @file c_tuple.h
 * @brief C-Tuple: an immutable, fixed-length sequence of cobj values.
 *
 * @code
 * cobj pair = cobj_tuple_value(cobj_tuple_new(2, (cobj[]){cobj_str("one"), cobj_i32(1)}));
 * cobj_print(&pair);                         // (one, 1)
 * cobj_drop(&pair);
 * @endcode
 *
 * cobj_tuple_get lends the element: it stays owned by the tuple.
 */

#include "../c_obj.h"

/// A tuple (opaque); create it with cobj_tuple_new() or cobj_tuple_make()
typedef struct cobj_tuple cobj_tuple;

/// The tuple type descriptor
extern const cobj_type COBJ_TUPLE;

/**
 * Moves items[0..len) into a new tuple and leaves them empty. items may be
 * NULL when len is 0. When an item is empty or allocation fails, every item
 * is dropped and NULL is returned.
 */
cobj_tuple* cobj_tuple_new(size_t len, cobj* items);

/// A tuple value from 1 to 32 C values or cobj values, each passed through cobj_of: cobj_tuple_make("x", 2)
#define cobj_tuple_make(...) \
    cobj_tuple_value(cobj_tuple_new(COBJ_ARG_COUNT(__VA_ARGS__), (cobj[]){COBJ_MAP(cobj_of, __VA_ARGS__)}))

/// Drops every element. NULL is ignored
void cobj_tuple_free(cobj_tuple* tuple);

/// Number of elements; 0 for NULL
size_t cobj_tuple_len(const cobj_tuple* tuple);

/// Borrowed element, NULL when index is out of range
const cobj* cobj_tuple_get(const cobj_tuple* tuple, size_t index);

/// Moves the tuple into a value, which now owns it. Empty value when tuple is NULL
cobj cobj_tuple_value(cobj_tuple* tuple);

/// The tuple held by a value (borrowed), NULL for any other type
cobj_tuple* cobj_tuple_of(const cobj* value);

#endif //C_TUPLE_H
