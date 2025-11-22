/* C-Obj - A Generic Object Handling Library in C
*  Created on: Nov 22, 2025
*  Creator: Ohananoshi - Guilherme Arruda
*  Description: Defines structures and functions for handling generic objects with metadata in C.
*
*  OBS: THIS LIB NEEDS c_primitives.h TO WORK PROPERLY. https://github.com/ohananoshi/C-Primitives
*/

#include "../C-Primitives/c_primitives.h"

#ifndef C_OBJ_H
#define C_OBJ_H

#define DEFAULT_MAX_POINTER_LEVEL 3

typedef struct metadata{
    uint8_t shape[DEFAULT_MAX_POINTER_LEVEL + 1];
    uint8_t type_id;
}metadata_t;

typedef struct obj{
    void* data;
    metadata_t obj_metadata;
}obj_t;

DATA_SIZE_TYPE obj_set_metadata(obj_t* dest_obj, const char* fmt){  //UINT8[1,2,3]
    char st1[10];
    char *st2 = (char*)calloc(20, sizeof(char));
    uint8_t pointer_level = 1;
    DATA_SIZE_TYPE size = 1;

    sscanf(fmt, "%[^\[]\[%[^]]", st1, st2);

    dest_obj->obj_metadata.type_id = get_type_id(st1);

    if(sscanf(st2, "%hhu,", &(dest_obj->obj_metadata.shape[pointer_level])) != 1) fprintf(stderr,"_fmt_parse: missing dimension.\n");
    pointer_level++;

    if(dest_obj->obj_metadata.shape[1] == 0) {
        dest_obj->obj_metadata.shape[0] = 0;
        free(st2);
        return _type_map[dest_obj->obj_metadata.type_id - 1].type_size;
    }
    size *= _type_map[dest_obj->obj_metadata.type_id - 1].type_size*dest_obj->obj_metadata.shape[1];

    while((st2 = strchr(st2 + 1, ',')) != NULL){
        pointer_level++;
        sscanf(st2, ",%hhu", &(dest_obj->obj_metadata.shape[pointer_level - 1]));

        size *= _type_map[dest_obj->obj_metadata.type_id - 1].type_size*dest_obj->obj_metadata.shape[pointer_level - 1];
    }

    dest_obj->obj_metadata.shape[0] = pointer_level - 1;
    
    free(st2);

    return size;
}

void obj_set(obj_t* obj, const char* fmt, void* data){
    DATA_SIZE_TYPE data_size;
    char st1[10];

    if(obj == NULL) obj = (obj_t*)calloc(1, sizeof(obj_t));
    obj->data = NULL;
    
    sscanf(fmt, "%[^\[]", st1);

    if(get_type_id(st1) > L_DOUBLE) data_size = obj_set_metadata(obj, fmt);
    else data_size = obj_set_metadata(obj, fmt);

    obj->data = calloc(1, data_size);

    memcpy(obj->data, data, data_size);
}

void obj_free(obj_t* obj){
    free(obj->data);
    obj = NULL;
}

DATA_SIZE_TYPE obj_get_data_size(obj_t obj){
    DATA_SIZE_TYPE size = 1;
    for(uint8_t i = 0 ; i < obj.obj_metadata.shape[0]; i++)
        size *= _type_map[obj.obj_metadata.type_id - 1].type_size*obj.obj_metadata.shape[i+1];

    return size;
}

void obj_print(obj_t obj){

    switch (obj.obj_metadata.shape[0])
    {
    case 0:
        _type_map[obj.obj_metadata.type_id - 1].print_fn(obj.data);
        break;
    case 1:
        printf("(");
        for(uint8_t i = 0; i < obj.obj_metadata.shape[1]; i++){
            _type_map[obj.obj_metadata.type_id - 1].print_fn(obj.data + i*(_type_map[obj.obj_metadata.type_id - 1].type_size));
            printf(",");
        }
        printf("\b)\n");
        break;
    case 2:
        printf("(");
        for(uint8_t i = 0; i < obj.obj_metadata.shape[1]; i++){
            printf("(");
            for(uint8_t j = 0; j < obj.obj_metadata.shape[2]; j++){
                _type_map[obj.obj_metadata.type_id - 1].print_fn(obj.data + (i*obj.obj_metadata.shape[2] + j)*(_type_map[obj.obj_metadata.type_id - 1].type_size));
            printf(",");
            }
            printf("\b),");
        }
        printf("\b)\n");
        break;
    case 3:
        printf("(");
        for(uint8_t i = 0; i < obj.obj_metadata.shape[1]; i++){
            printf("(");
            for(uint8_t j = 0; j < obj.obj_metadata.shape[2]; j++){
                printf("(");
                for(uint8_t k = 0; k < obj.obj_metadata.shape[3]; k++){
                    _type_map[obj.obj_metadata.type_id - 1].print_fn(obj.data + (i*obj.obj_metadata.shape[2]*obj.obj_metadata.shape[3] + j*obj.obj_metadata.shape[3] + k)*(_type_map[obj.obj_metadata.type_id - 1].type_size));
                    printf(",");
                }
                printf("\b),");
            }
            printf("\b,");
        }
        printf("\b)\n");
        break;
    
    default:
        break;
    }
}

DATA_CMP_TYPE obj_cmp(obj_t a, obj_t b){
    if(a.obj_metadata.type_id != b.obj_metadata.type_id)
        return a.obj_metadata.type_id - b.obj_metadata.type_id;
    
    uint8_t elemen_count = 1;
    uint8_t i;
    DATA_CMP_TYPE j;
    
    for(i = 1; i < a.obj_metadata.shape[0]; i++) 
        if(a.obj_metadata.shape[i] != b.obj_metadata.shape[i]) 
            return a.obj_metadata.shape[i] - b.obj_metadata.shape[i];
        elemen_count *= a.obj_metadata.shape[i];

    for(i = 0; i < elemen_count; i++){
        j = _type_map[a.obj_metadata.type_id - 1].cmp_fn(a.data + i*(_type_map[a.obj_metadata.type_id - 1].type_size), b.data + i*(_type_map[b.obj_metadata.type_id - 1].type_size));
        if(j != 0) return j;            
    }

    return 0;
}

void _obj_print(void* obj){
    obj_print(*(obj_t*)obj);
}

DATA_CMP_TYPE _obj_cmp(void* a, void* b){
    return obj_cmp(*(obj_t*)a, *(obj_t*)b);
}

void init_obj_map(){
    add_to_type_map("OBJ", sizeof(obj_t), _obj_print, _obj_cmp);
}

#endif /*C_VOID_H*/