#ifndef C_HASHMAP_H
#define C_HASHMAP_H

/**
 * @file c_hashmap.h
 * @brief C-Hashmap: maps cobj keys to cobj values, in insertion order (like a
 * Python dict). Any hashable value can be a key (see cobj_hashable):
 * numbers, strings, tuples, lists, module types with a hash.
 *
 * @code
 * cobj ages = cobj_hashmap_value(cobj_hashmap_new(0));
 * cobj_hashmap_set(cobj_hashmap_of(&ages), cobj_str("ana"), cobj_i32(31));
 * const cobj* age = cobj_hashmap_get_str(cobj_hashmap_of(&ages), "ana");
 * cobj_print(&ages);                         // {ana: 31}
 * cobj_drop(&ages);
 * @endcode
 *
 * cobj_hashmap_set consumes the key and the value, even when it fails.
 * The get functions lend the value: it stays owned by the map.
 * Iterating a map value (COBJ_FOREACH) yields its keys.
 * The map grows as needed. Builds on c_list and c_tuple: keys, values and
 * items return new lists.
 */

#include "../c_obj.h"
#include "../c_list/c_list.h"
#include "../c_tuple/c_tuple.h"

/// A map (opaque); create it with cobj_hashmap_new() or cobj_hashmap_make()
typedef struct cobj_hashmap cobj_hashmap;

/// The map type descriptor
extern const cobj_type COBJ_HASHMAP;

/// capacity: the number of entries expected (a hint, the map grows past it); 0 picks a default. NULL when allocation fails
cobj_hashmap* cobj_hashmap_new(size_t capacity);

/**
 * Moves alternating keys and values (items[0] is a key, items[1] its value...)
 * into a new map and leaves them empty. When count is odd, a key or value is
 * invalid, or allocation fails, every item is dropped and NULL is returned.
 */
cobj_hashmap* cobj_hashmap_from_pairs(size_t count, cobj* items);

/// A map value from 1 to 16 key/value pairs of C values or cobj values, each passed through cobj_of:
/// cobj_hashmap_make("one", 1, "two", 2). An odd number of arguments doesn't compile
#define cobj_hashmap_make(...)                                                              \
    ((void)sizeof(char[COBJ_ARG_COUNT(__VA_ARGS__) % 2 == 0 ? 1 : -1]),                     \
     cobj_hashmap_value(cobj_hashmap_from_pairs(COBJ_ARG_COUNT(__VA_ARGS__),                \
                                                (cobj[]){COBJ_MAP(cobj_of, __VA_ARGS__)})))

/// Drops every key and value. NULL is ignored
void cobj_hashmap_free(cobj_hashmap* map);

/// Number of entries; 0 for NULL
size_t cobj_hashmap_len(const cobj_hashmap* map);

/// Stores value under key. A new key goes to the end; an existing key keeps its place and gets the new value.
/// COBJ_ETYPE when the key is not hashable
cobj_status cobj_hashmap_set(cobj_hashmap* map, cobj key, cobj value);

/// Borrowed value, NULL when the key is missing
const cobj* cobj_hashmap_get(const cobj_hashmap* map, const cobj* key);

/// cobj_hashmap_get with a string key, without building a key value
const cobj* cobj_hashmap_get_str(const cobj_hashmap* map, const char* key);

/// True when key is in the map
bool cobj_hashmap_contains(const cobj_hashmap* map, const cobj* key);

/// Removes key and moves its value into *out, or drops the value when out is NULL. COBJ_ENOTFOUND when the key is missing
cobj_status cobj_hashmap_remove(cobj_hashmap* map, const cobj* key, cobj* out);

/**
 * Walks the entries in insertion order without allocating. Start with
 * *position = 0; each call lends the next key and value (either pointer may
 * be NULL) and returns false at the end.
 */
bool cobj_hashmap_next(const cobj_hashmap* map, size_t* position, const cobj** key, const cobj** value);

/// New lists of copies, in insertion order (the caller owns them). NULL when allocation fails
cobj_list* cobj_hashmap_keys(const cobj_hashmap* map);

/// A new list with copies of the values, in insertion order (the caller owns it). NULL when allocation fails
cobj_list* cobj_hashmap_values(const cobj_hashmap* map);

/// List of (key, value) tuples
cobj_list* cobj_hashmap_items(const cobj_hashmap* map);

/// Moves the map into a value, which now owns it. Empty value when map is NULL
cobj cobj_hashmap_value(cobj_hashmap* map);

/// The map held by a value (borrowed), NULL for any other type
cobj_hashmap* cobj_hashmap_of(const cobj* value);

#endif //C_HASHMAP_H
