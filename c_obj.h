#ifndef _C_OBJ_HEADER
#define _C_OBJ_HEADER

#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

enum std_id{
    UINT8_T = -1,
    UINT16_T = -2,
    UINT32_T = -4,
    UINT64_T = -8,
    INT8_T = 1,
    INT16_T = 2,
    INT32_T = 4,
    INT64_T = 8,
    FLOAT = 14,
    DOUBLE = 18,
    LONG_DOUBLE = 26,
    CHAR = 31,
    C_OBJ,
    SAME,
    VOIDP,
    END
};

typedef struct c_obj_t{
    void* value;
    int8_t obj_type;
}c_obj_t;

typedef int8_t (*obj_set_fn)(c_obj_t* dst, ...);

typedef int8_t (*obj_get_fn)(c_obj_t src, ...);

typedef int32_t (*obj_cmp_fn)(void* src, ...);

typedef int8_t (*obj_print_fn)(void* src, ...);

typedef struct common_attributes_t{
    obj_set_fn set;
    obj_get_fn get;
    obj_cmp_fn cmp;
    obj_print_fn print;
}common_attributes_t;

typedef struct att_map_t{
    uint8_t type_id;
    common_attributes_t* att;
    struct att_map_t* next;
}att_map_t;

//extern common_attributes_t common_fn;

int8_t obj_std_set(c_obj_t* dst, ...);

int8_t obj_std_vset(c_obj_t* dst, att_map_t attribute_map,va_list args);

int8_t obj_std_get(c_obj_t src, ...);

int8_t obj_std_vget(c_obj_t src, att_map_t attribute_map, va_list args);

int32_t obj_std_cmp(c_obj_t src, ...);

int32_t obj_std_vcmp(c_obj_t src, att_map_t attribute_map, va_list args);

int8_t obj_std_print(c_obj_t src, ...);

int8_t obj_std_vprint(c_obj_t src, att_map_t attribute_map, va_list args);

uint8_t add_maps(att_map_t* dst, ...);

#endif //C_OBJ end