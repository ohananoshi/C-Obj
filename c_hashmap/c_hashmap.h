#ifndef _C_HASHMAP_HEADER
#define _C_HASHMAP_HEADER

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "../c_obj.h"

#define HASHAMP_ID 54

typedef size_t (*hash_fn_t)(void* key);

typedef struct map_bucket_t{
    c_obj_t key;
    c_obj_t value;
    struct map_bucket_t* next;
}map_bucket_t;

typedef struct hashmap_t{
    size_t capacity;
    hash_fn_t hash;
    uint8_t optimize_hash_key;
    map_bucket_t* buckets;
}hashmap_t;

extern common_attributes_t hashmap_att;

size_t str_hash(void* key);

void hashmap_init(hashmap_t* dst, size_t capacity, hash_fn_t hash_fn);

void hashmap_insert(hashmap_t* dst, att_map_t attribute_map, ...);

void* hashmap_get(hashmap_t src, att_map_t attribute_map, ...);

size_t hashmap_keys(hashmap_t src, att_map_t attribute_map, c_obj_t** dst);

size_t hashmap_values(hashmap_t src, att_map_t attribute_map, c_obj_t** dst);

size_t hashmap_items(hashmap_t src, att_map_t attribute_map, c_obj_t*** dst);

int8_t hashmap_obj_set(c_obj_t* dst, ...);

int8_t hashmap_obj_get(c_obj_t src, ...);

int32_t hashmap_cmp(void* src, ...);

int8_t hashmap_print(void* src, ...);

#endif //End of c_hashmap.h