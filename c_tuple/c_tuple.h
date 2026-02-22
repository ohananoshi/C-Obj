#ifndef _C_TUPLE_HEADER
#define _C_TUPLE_HEADER

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "../c_obj.h"

#define TUPLE_ID 53

#define start_tuple {NULL, 0}

typedef struct tuple_t{
    c_obj_t* tuple;
    size_t len;
}tuple_t;

/*
typedef struct tuple_attributes_t{
    common_attributes_t common;
    void (*create)(tuple_t* dst, att_map_t attribute_map, ...);
    void (*unpack)();
}tuple_attributes_t;
*/

extern common_attributes_t tuple_att;

int8_t tuple_obj_set(c_obj_t* dst, ...);

int8_t tuple_obj_get(c_obj_t src, ...);

int32_t tuple_cmp(void* src, ...);

int8_t tuple_print(void* src, ...);

void tuple_create(tuple_t* dst, att_map_t attriube_map, ...);

//void tuple_map_add(att_map_t* attribute_map);

#endif // End of c_tuple.h