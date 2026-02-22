
//==================================== HEADERS ==============================================
#ifndef _C_STRING_HEADER
#define _C_STRING_HEADER

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

#include "../c_obj.h"

//========================================= DATA STRUCTURES ====================================

// MACROS

#define _STR_ID 1
#define END_LIST "-->END_LIST<--"

typedef struct str_attributes_t{
    common_attributes_t common;
    size_t (*len)(const char* src_str);
    size_t (*count)(const char* src_str, const char* value);
    size_t (*split)(const char* src_str, const char* value, char*** dest_list);
    size_t (*join)(char** dest_str, char* join_str, char** str_list, size_t item_count);
    size_t (*vjoin)(char** dest_str, char* join_str, ...);
    size_t (*search)(const char* src_str, const char* value, size_t** dest_pos, size_t ocurrency_num);
    size_t (*replace)(char** dest_str, const char* src_str, char* old_value, char* new_value);
    size_t (*from_file)(char** dest_str, char* file_path);
    size_t (*to_file)(const char* src_str, char* file_path, char* mode);
}str_attributes_t;

extern str_attributes_t str_att;

//=================================== FUNCTIONS ==============================================

void str_obj_set(void* src_str, c_obj_t* dst_obj);

void* str_obj_get(c_obj_t src_obj, void* dst_str);

int32_t str_obj_cmp(void* a, void* b, ...);

int32_t str_obj_print(void* src, ...);

size_t str_count(const char* src_str, const char* value);

size_t str_split(const char* src_str, const char* value, char*** dest_list);

size_t str_join(char** dest_str, char* join_str, char** str_list, size_t item_count);

size_t str_vjoin(char** dest_str, char* join_str, ...);

size_t str_search(const char* src_str, const char* value, size_t** dest_pos, size_t occurrence_num);

size_t str_replace(char** dest_str, const char* src_str, char* old_value, char* new_value);

size_t str_from_file(char** dest_str, char* file_path);

size_t str_to_file(const char* src_str, char* file_path, char* mode);

void str_map_add(att_map_t* attribute_map);


#endif // END C_strings