#ifndef _C_LIST_HEADER
#define _C_LIST_HEADER

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../c_obj.h"

#define LIST_ID 52

#define list_init {{NULL, 0}, NULL}

typedef struct list_t{
    c_obj_t obj;
    struct list_t *next;    
}list_t;

/*
typedef struct list_attributes_t{
    common_attributes_t common;
    size_t (*len)(list_t src_list);
    list_t* (*append)(list_t* dst_list, att_map_t attribute_map, ...);
}list_attributes_t;
*/


extern common_attributes_t list_att;

int8_t list_set_obj(c_obj_t* dst, ...);

int8_t list_get_obj(c_obj_t src, ...);

int32_t list_cmp(void* src, ...);

int8_t list_print(void* src, ...);

size_t list_len(list_t src_list);

list_t* list_append(list_t* dst_list, att_map_t attribute_map, ...);

//void list_map_add(att_map_t* attribute_map);

#endif //End of c_list.h