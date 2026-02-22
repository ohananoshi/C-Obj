#include "c_list.h"

int8_t list_set_obj(c_obj_t* dst, ...){
    va_list args;
    va_start(args, dst);

    list_t src = *(list_t*)va_arg(args, void*);
    att_map_t attribute_map = va_arg(args, att_map_t);

    dst->value = calloc(1, sizeof(list_t));
    list_t *dest_list = (list_t*)dst->value;

    while(src.obj.value != NULL || src.next != NULL){
        //obj_std_set(&(dest_current->obj), attribute_map, C_OBJ, src.obj);
        //dest_current->next = (list_t*)calloc(1, sizeof(list_t));

        //dest_current = dest_current->next;
        //src = *src.next;
        list_append(dest_list, attribute_map, C_OBJ, src.obj, END);
        src = *src.next;
    }

    dst->obj_type = LIST_ID;

    va_end(args);
}

int8_t list_get_obj(c_obj_t src, ...){
    va_list args;
    va_start(args, src);

    void* dst = va_arg(args, void*);

    dst = realloc(dst, sizeof(list_t));
    memcpy(dst, src.value, sizeof(list_t));

    va_end(args);
    return 1;
}

int32_t list_cmp(void* src, ...){
    va_list args;
    va_start(args, src);

    list_t src_list = *(list_t*)src;
    list_t b_list = *(list_t*)va_arg(args, void*);
    att_map_t attribute_map = va_arg(args, att_map_t);

    size_t len_src = list_len(src_list);
    size_t b_len = list_len(b_list);

    if(len_src != b_len) return len_src - b_len;

    int32_t result = 0;
    
    while((result = obj_std_cmp(src_list.obj, attribute_map, b_list.obj.obj_type, b_list.obj.value)) == 0);

    va_end(args);
    return result;
}

int8_t list_print(void* src, ...){

    va_list args;
    va_start(args, src);

    att_map_t attribute_map = va_arg(args, att_map_t);

    printf("[");
    
    list_t current = *(list_t*)src;
    while(current.obj.value != NULL){
        obj_std_print(current.obj, attribute_map);
        printf(",");

        if(current.next == NULL) break;
        current = *(current.next);
    }
    printf("\b]");
    va_end(args);
}

size_t list_len(list_t src_list){
    size_t len = 0;
    list_t tmp = src_list;
    while(src_list.next != NULL){
        tmp = *(tmp.next);
        len++;
    }

    return len;
}

list_t* list_append(list_t* dst_list, att_map_t attribute_map, ...){
    va_list args;
    va_start(args, attribute_map);

    list_t *current = dst_list;
    
    while(current->next != NULL) current = current->next;
    if(current->obj.value != NULL){
        current->next = (list_t*)calloc(1, sizeof(list_t));
        current = current->next;
        current->next = NULL;
    }

    while(obj_std_vset(&(current->obj), attribute_map, args) != -1){
        current->next = (list_t*)calloc(1, sizeof(list_t));
        current = current->next;
        current->obj.value = NULL;
        current->next = NULL;
    }

    va_end(args);
    return NULL;
}

common_attributes_t list_att = {list_set_obj,list_get_obj,list_cmp,list_print};

/*
void list_map_add(att_map_t* attribute_map){
    att_map_t *current = attribute_map;

    if(current->att == NULL){
        current->att = calloc(1, sizeof(common_attributes_t));
        memcpy(current->att, &list_att, 32);
        current->type_id = LIST_ID;
        current->next = NULL;   
    }else{
        while (current->next != NULL) current = current->next;
        current->next = (att_map_t*)calloc(1, sizeof(att_map_t));
        current = current->next;
        current->att = calloc(1, sizeof(common_attributes_t));
        memcpy(current->att, &list_att, 32);
        current->type_id = LIST_ID;
        current->next = NULL;
    }
      
}
    */


