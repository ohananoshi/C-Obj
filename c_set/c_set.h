#ifndef C_SET_H
#define C_SET_H

/**
 * @file c_set.h
 * @brief C-Set: a collection of distinct hashable values, in insertion order.
 *
 * @code
 * cobj tags = cobj_set_value(cobj_set_new());
 * cobj_set_add(cobj_set_of(&tags), cobj_str("c"));
 * cobj_set_add(cobj_set_of(&tags), cobj_str("c"));   // already there: nothing changes
 * cobj_print(&tags);                                  // {c}
 * cobj_drop(&tags);
 * @endcode
 *
 * cobj_set_add consumes the value, even when it fails.
 * Builds on c_hashmap: the elements are the keys of a map.
 */

#include "../c_obj.h"
#include "../c_hashmap/c_hashmap.h"

/// A set (opaque); create it with cobj_set_new() or cobj_set_make()
typedef struct cobj_set cobj_set;

/// The set type descriptor
extern const cobj_type COBJ_SET;

/// A new empty set; NULL when allocation fails
cobj_set* cobj_set_new(void);

/// Drops every element. NULL is ignored
void cobj_set_free(cobj_set* set);

/// Number of elements; 0 for NULL
size_t cobj_set_len(const cobj_set* set);

/// Adds value, or drops it when an equal element is already there. COBJ_ETYPE when it isn't hashable
cobj_status cobj_set_add(cobj_set* set, cobj value);

/// COBJ_ENOTFOUND when value isn't in the set
cobj_status cobj_set_remove(cobj_set* set, const cobj* value);

/// True when value is in the set
bool cobj_set_contains(const cobj_set* set, const cobj* value);

/// Moves items[0..count) into a new set (duplicates are dropped) and leaves them empty. When an item
/// is empty or not hashable, or allocation fails, every item is dropped and NULL is returned
cobj_set* cobj_set_from_values(size_t count, cobj* items);

/// A set value from 1 to 32 C values or cobj values, each passed through cobj_of: cobj_set_make(3, 1, 3)
#define cobj_set_make(...) \
    cobj_set_value(cobj_set_from_values(COBJ_ARG_COUNT(__VA_ARGS__), (cobj[]){COBJ_MAP(cobj_of, __VA_ARGS__)}))

/// A new set with copies of the elements of any iterable, duplicates removed.
/// NULL when it isn't iterable, an element isn't hashable, or allocation fails
cobj_set* cobj_set_from(const cobj* iterable);

/// A new set with copies of a's elements, then b's. NULL when an argument is NULL or allocation fails
cobj_set* cobj_set_union(const cobj_set* a, const cobj_set* b);

/// A new set with copies of a's elements that are in b. NULL when an argument is NULL or allocation fails
cobj_set* cobj_set_intersection(const cobj_set* a, const cobj_set* b);

/// A new set with copies of a's elements that are not in b. NULL when an argument is NULL or allocation fails
cobj_set* cobj_set_difference(const cobj_set* a, const cobj_set* b);

/// True when every element of a is in b
bool cobj_set_is_subset(const cobj_set* a, const cobj_set* b);

/// Moves the set into a value, which now owns it. Empty value when set is NULL
cobj cobj_set_value(cobj_set* set);

/// The set held by a value (borrowed), NULL for any other type
cobj_set* cobj_set_of(const cobj* value);

#endif //C_SET_H
