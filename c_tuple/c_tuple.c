#include "c_tuple.h"

int8_t tuple_obj_set(c_obj_t* dst, ...){
    va_list args;
    va_start(args, dst);

    tuple_t src = *(tuple_t*)va_arg(args, void*);
    att_map_t attribute_map = va_arg(args, att_map_t);

    dst->value = calloc(1, sizeof(tuple_t));
    tuple_t *dst_tuple = (tuple_t*)dst->value;
    dst_tuple->len = src.len;
    dst_tuple->tuple = (c_obj_t*)calloc(dst_tuple->len, sizeof(c_obj_t));
    for(size_t i = 0; i < dst_tuple->len; i++) 
        obj_std_set(&(dst_tuple->tuple[i]), attribute_map, C_OBJ, src.tuple[i]);

    dst->obj_type = TUPLE_ID;

    va_end(args);
}

int8_t tuple_obj_get(c_obj_t src, ...){
    va_list args;
    va_start(args, src);

    tuple_t *source = (tuple_t*)src.value;
    tuple_t *dst = (tuple_t*)va_arg(args, void*);
    att_map_t attribute_map = va_arg(args, att_map_t);

    dst->len = source->len;
    dst->tuple = (c_obj_t*)calloc(dst->len, sizeof(c_obj_t));
    for(size_t i = 0; i < dst->len; i++) 
        obj_std_set(&(dst->tuple[i]), attribute_map, C_OBJ, source->tuple[i]);

    va_end(args);
}

int32_t tuple_cmp(void* src, ...){
    va_list args;
    va_start(args, src);

    tuple_t a = *(tuple_t*)src;
    tuple_t b = *(tuple_t*)va_arg(args, void*);
    att_map_t attribute_map = va_arg(args, att_map_t);

    if(a.len != b.len) return a.len - b.len;

    int32_t result = 0;
    size_t i = 0;
    while((result = obj_std_cmp(a.tuple[i], attribute_map, C_OBJ, b.tuple[i])) == 0);

    va_end(args);
    return result;
}

int8_t tuple_print(void* src, ...){
    va_list args;
    va_start(args, src);

    tuple_t tmp = *(tuple_t*)src;
    att_map_t attribute_map = va_arg(args, att_map_t);

    printf("(");
    for(size_t i = 0; i < tmp.len; i++){
        if(tmp.tuple[i].obj_type > 50){
            att_map_t current = attribute_map;
            while(current.type_id != tmp.tuple[i].obj_type) current = *current.next;

            current.att->print(tmp.tuple[i].value, attribute_map);
            printf(",");
            continue;
        }
        
        obj_std_print(tmp.tuple[i], attribute_map);

        printf(",");
    }
    printf("\b)");
    va_end(args);
}

void tuple_create(tuple_t* dst, att_map_t attribute_map,...){
    va_list args;
    va_start(args, attribute_map);
    
    int8_t set_fl;

    dst->tuple = (c_obj_t*)realloc(dst->tuple, (dst->len + 1)*sizeof(c_obj_t));
    set_fl = obj_std_vset(&(dst->tuple[dst->len]), attribute_map, args);
    //dst->len++;
    while(set_fl > 0){
        dst->len++;
        dst->tuple = (c_obj_t*)realloc(dst->tuple, (dst->len + 1)*sizeof(c_obj_t));
        set_fl = obj_std_vset(&(dst->tuple[dst->len]), attribute_map, args);
    };

    //dst->len -= 1;

}

common_attributes_t tuple_att = {tuple_obj_set,tuple_obj_get,tuple_cmp,tuple_print};

/*
void tuple_map_add(att_map_t* attribute_map){
    att_map_t *current = attribute_map;

    if(current->att == NULL){
        current->att = calloc(1, sizeof(common_attributes_t));
        memcpy(current->att, &tuple_att, 32);
        current->type_id = TUPLE_ID;
        current->next = NULL;   
    }else{
        while (current->next != NULL) current = current->next;
        current->next = (att_map_t*)calloc(1, sizeof(att_map_t));
        current = current->next;
        current->att = calloc(1, sizeof(common_attributes_t));
        memcpy(current->att, &tuple_att, 32);
        current->type_id = TUPLE_ID;
        current->next = NULL;
    }
      
}

*/