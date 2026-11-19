#ifndef C_LIST_H
#define C_LIST_H

/**
 * @file c_list.h
 * @brief C-List: a growable list of cobj values, stored contiguously.
 *
 * @code
 * cobj fruits = cobj_list_value(cobj_list_new());
 * cobj_list_push(cobj_list_of(&fruits), cobj_str("banana"));
 * cobj_print(&fruits);                       // [banana]
 * cobj_drop(&fruits);
 * @endcode
 *
 * Insert functions consume the value they receive, even when they fail.
 * cobj_list_get lends the element: it stays owned by the list.
 * A list can't contain itself.
 */

#include "../c_obj.h"

/// A list (opaque); create it with cobj_list_new()
typedef struct cobj_list cobj_list;

/// The list type descriptor
extern const cobj_type COBJ_LIST;

/// A new empty list; NULL when allocation fails
cobj_list* cobj_list_new(void);

/// Drops every element. NULL is ignored
void cobj_list_free(cobj_list* list);

/// Number of elements; 0 for NULL
size_t cobj_list_len(const cobj_list* list);

/// Appends value. COBJ_EINVAL for a NULL list or an empty value
cobj_status cobj_list_push(cobj_list* list, cobj value);

/// Borrowed element, NULL when index is out of range
const cobj* cobj_list_get(const cobj_list* list, size_t index);

/// Replaces the element at index and drops the old one. COBJ_ERANGE when index is out of range
cobj_status cobj_list_set(cobj_list* list, size_t index, cobj value);

/// Moves the last element into *out. COBJ_ERANGE when the list is empty
cobj_status cobj_list_pop(cobj_list* list, cobj* out);

/// Moves items[0..count) into a new list and leaves them empty. When an item is empty or
/// allocation fails, every item is dropped and NULL is returned
cobj_list* cobj_list_from_values(size_t count, cobj* items);

/// A list value from 1 to 32 C values or cobj values, each passed through cobj_of: cobj_list_make(1, "two", 3.5)
#define cobj_list_make(...) \
    cobj_list_value(cobj_list_from_values(COBJ_ARG_COUNT(__VA_ARGS__), (cobj[]){COBJ_MAP(cobj_of, __VA_ARGS__)}))

/// A new list with copies of the elements of any iterable value. NULL when it isn't iterable or allocation fails
cobj_list* cobj_list_from(const cobj* iterable);

/// Sorts the elements in ascending cobj_cmp order (not stable). NULL is ignored
void cobj_list_sort(cobj_list* list);

/// Moves the list into a value, which now owns it. Empty value when list is NULL
cobj cobj_list_value(cobj_list* list);

/// The list held by a value (borrowed), NULL for any other type
cobj_list* cobj_list_of(const cobj* value);

#endif //C_LIST_H
