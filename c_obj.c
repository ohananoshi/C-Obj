#include "c_obj.h"

int8_t obj_std_set(c_obj_t* dst, ...){
    int8_t return_value;

    va_list args;
    va_start(args, dst);

    att_map_t attribute_map = va_arg(args, att_map_t);

    return_value = obj_std_vset(dst, attribute_map, args);

    va_end(args);

    return return_value;
}

int8_t obj_std_vset(c_obj_t* dst, att_map_t attribute_map, va_list args){
  
    int8_t type_id = (int8_t)va_arg(args, int32_t);

    switch(type_id){

        /* -------- UNSIGNED -------- */

        case UINT8_T: {
            uint8_t temp = (uint8_t)va_arg(args, uint32_t);
            dst->value = calloc(1,sizeof(uint8_t));
            memcpy(dst->value, &temp, sizeof(uint8_t));
            dst->obj_type = type_id;
            break;
        }

        case UINT16_T: {
            uint16_t temp = (uint16_t)va_arg(args, uint32_t);
            dst->value = calloc(1,sizeof(uint16_t));
            memcpy(dst->value, &temp, sizeof(uint16_t));
            dst->obj_type = type_id;
            break;
        }

        case UINT32_T: {
            uint32_t temp = va_arg(args, uint32_t);
            dst->value = calloc(1,sizeof(uint32_t));
            memcpy(dst->value, &temp, sizeof(uint32_t));
            dst->obj_type = type_id;
            break;
        }

        case UINT64_T: {
            uint64_t temp = va_arg(args, uint64_t);
            dst->value = calloc(1,sizeof(uint64_t));
            memcpy(dst->value, &temp, sizeof(uint64_t));
            dst->obj_type = type_id;
            break;
        }

        /* -------- SIGNED -------- */
        
        case INT8_T: {
            int8_t temp = (int8_t)va_arg(args, int32_t);
            dst->value = calloc(1,sizeof(int8_t));
            memcpy(dst->value, &temp, sizeof(int8_t));
            dst->obj_type = type_id;
            break;
        }

        case INT16_T: {
            int16_t temp = (int16_t)va_arg(args, int32_t);
            dst->value = calloc(1,sizeof(int16_t));
            memcpy(dst->value, &temp, sizeof(int16_t));
            dst->obj_type = type_id;
            break;
        }

        case INT32_T: {
            int32_t temp = (int32_t)va_arg(args, int32_t);
            dst->value = calloc(1,sizeof(int32_t));
            memcpy(dst->value, &temp, sizeof(int32_t));
            dst->obj_type = type_id;
            break;
        }

        case INT64_T: {
            int64_t temp = (int64_t)va_arg(args, int64_t);
            dst->value = calloc(1,sizeof(int64_t));
            memcpy(dst->value, &temp, sizeof(int64_t));
            dst->obj_type = type_id;
            break;
        }

        /* -------- FLOATING -------- */

        case FLOAT: {
            float temp = (float)va_arg(args, double);
            dst->value = calloc(1,sizeof(float));
            memcpy(dst->value, &temp, sizeof(float));
            dst->obj_type = type_id;
            break;
        }

        case DOUBLE: {
            double temp = (double)va_arg(args, double);
            dst->value = calloc(1,sizeof(double));
            memcpy(dst->value, &temp, sizeof(double));
            dst->obj_type = type_id;
            break;
        }

        #if __HAVE_FLOAT128X
        case LONG_DOUBLE: {
            long double temp = (double)va_arg(args, long double);
            dst->value = calloc(1,sizeof(long double));
            memcpy(dst->value, &temp, sizeof(long double));
            dst->obj_type = type_id;
            break;
        }
        #endif

        /* -------- CHAR -------- */

        case CHAR: {
            char* temp = va_arg(args, char*);
            size_t len = strlen(temp);
            dst->value = calloc(len, sizeof(char));
            strcpy(dst->value, temp);
            dst->obj_type = type_id;
            break;
        }

        case C_OBJ: {
            c_obj_t temp = va_arg(args, c_obj_t);
            dst->obj_type = temp.obj_type;

            size_t alloc_size;

            if(temp.obj_type < 0){
                alloc_size  = -1*temp.obj_type;
                dst->value = calloc(1, alloc_size);
                memcpy(dst->value, temp.value, alloc_size);
                break;
            }
            if(temp.obj_type > 0 && temp.obj_type < 10){
                dst->value = calloc(1, temp.obj_type);
                memcpy(dst->value, temp.value, temp.obj_type);
                break;
            }
            if(temp.obj_type > 10 && temp.obj_type < 30){
                alloc_size = temp.obj_type - 10;
                dst->value = calloc(1, alloc_size);
                memcpy(dst->value, temp.value, alloc_size);
                break;
            }
            if(temp.obj_type == 31){
                alloc_size = strlen((char*)temp.value);
                dst->value = calloc(alloc_size, sizeof(char));
                memcpy(dst->value, temp.value, alloc_size);
                break;
            }
            if(temp.obj_type > 50){
                while(attribute_map.type_id != temp.obj_type) attribute_map = *attribute_map.next;
                attribute_map.att->set(dst, temp.value, attribute_map);
                break;
            }
            break;
        }

        case END:
            return -1; 

        default: {
            void* temp = va_arg(args, void*);

            while(attribute_map.type_id != type_id) attribute_map = *attribute_map.next;

            attribute_map.att->set(dst, temp, attribute_map);

            break;
        }
    }
    return 1;
}

int8_t obj_std_get(c_obj_t src, ...){
    int8_t return_value;

    va_list args;
    va_start(args, src);

    att_map_t attribute_map = va_arg(args, att_map_t);

    return_value = obj_std_vget(src, attribute_map, args);

    va_end(args);
    return return_value;
}

int8_t obj_std_vget(c_obj_t src, att_map_t attribute_map, va_list args){

    switch(src.obj_type){

        /* -------- UNSIGNED -------- */

        case UINT8_T: {
            void* dst = va_arg(args, void*);
            dst = calloc(1,sizeof(uint8_t));
            memcpy(dst, src.value, sizeof(uint8_t));
            break;
        }

        case UINT16_T: {
            void* dst = va_arg(args, void*);
            dst = calloc(1,sizeof(uint16_t));
            memcpy(dst, src.value, sizeof(uint16_t));
            break;
        }

        case UINT32_T: {
            void* dst = va_arg(args, void*);
            dst = calloc(1,sizeof(uint32_t));
            memcpy(dst, src.value, sizeof(uint32_t));
            break;
        }

        case UINT64_T: {
            void* dst = va_arg(args, void*);
            dst = calloc(1,sizeof(uint64_t));
            memcpy(dst, src.value, sizeof(uint64_t));
            break;
        }

        /* -------- SIGNED -------- */
        
        case INT8_T: {
            void* dst = va_arg(args, void*);
            dst = calloc(1,sizeof(int8_t));
            memcpy(dst, src.value, sizeof(int8_t));
            break;
        }

        case INT16_T: {
            void* dst = va_arg(args, void*);
            dst = calloc(1,sizeof(int16_t));
            memcpy(dst, src.value, sizeof(int16_t));
            break;
        }

        case INT32_T: {
            void* dst = va_arg(args, void*);
            dst = calloc(1,sizeof(int32_t));
            memcpy(dst, src.value, sizeof(int32_t));
            break;
        }

        case INT64_T: {
            void* dst = va_arg(args, void*);
            dst = calloc(1,sizeof(int64_t));
            memcpy(dst, src.value, sizeof(int64_t));
            break;
        }

        /* -------- FLOATING -------- */

        case FLOAT: {
            void* dst = va_arg(args, void*);
            dst = calloc(1,sizeof(float));
            memcpy(dst, src.value, sizeof(float));
            break;
        }

        case DOUBLE: {
            void* dst = va_arg(args, void*);
            dst = calloc(1,sizeof(double));
            memcpy(dst, src.value, sizeof(double));
            break;
        }

        #if __HAVE_FLOAT128X
        case LONG_DOUBLE: {
            void* dst = va_arg(args, void*);
            dst = calloc(1,sizeof(long double));
            memcpy(dst, src.value, sizeof(long double));
            break;
        }
        #endif

        /* -------- CHAR -------- */

        case CHAR: {
            size_t len = strlen((char*)src.value);
            void* dst = va_arg(args, void*);
            dst = calloc(len,sizeof(char));
            memcpy(dst, src.value, len*sizeof(char));
            break;
        }

        case END:
            return -1; 

        default: {
            void* dst = va_arg(args, void*);

            while(attribute_map.type_id != src.obj_type) attribute_map = *attribute_map.next;

            attribute_map.att->get(src, dst, attribute_map);

            break;
        }
    }
    return 1;
}

int32_t obj_std_cmp(c_obj_t src, ...){
    int32_t return_value;

    va_list args;
    va_start(args, src);

    att_map_t attribute_map = va_arg(args, att_map_t);

    return_value = obj_std_vcmp(src, attribute_map, args);

    va_end(args);
    return return_value;
}

int32_t obj_std_vcmp(c_obj_t src, att_map_t attribute_map, va_list args){

    //c_obj_t b = *(c_obj_t*)va_arg(args, void*);

    //if(src.obj_type != b.obj_type) return src.obj_type - b.obj_type;

    int8_t type_id = (int8_t)va_arg(args, int32_t);

    switch(type_id){

        /* -------- UNSIGNED -------- */

        case UINT8_T: {
            uint8_t b_val = (uint8_t)va_arg(args, uint32_t);
            return *(uint8_t*)src.value - b_val;
            break;
        }

        case UINT16_T:{
            uint16_t b_val = (uint16_t)va_arg(args, uint32_t);
            return *(uint16_t*)src.value - b_val;
            break;
        }

        case UINT32_T:{
            uint32_t b_val = va_arg(args, uint32_t);
            return *(uint32_t*)src.value - b_val;
            break;
        }

        case UINT64_T:{
            uint64_t b_val = va_arg(args, uint64_t);
            return *(uint64_t*)src.value - b_val;
            break;
        }

        /* -------- SIGNED -------- */
        
        case INT8_T:{
            int8_t b_val = (int8_t)va_arg(args, int32_t);
            return *(int8_t*)src.value - b_val;
            break;
        }

        case INT16_T:{
            int16_t b_val = (int16_t)va_arg(args, int32_t);
            return *(int16_t*)src.value - b_val;
            break;
        }

        case INT32_T:{
            int32_t b_val = va_arg(args, int32_t);
            return *(int32_t*)src.value - b_val;
            break;
        }

        case INT64_T:{
            int64_t b_val = va_arg(args, int64_t);
            return *(int64_t*)src.value - b_val;
            break;
        }

        /* -------- FLOATING -------- */

        case FLOAT:{
            float b_val = (float)va_arg(args, double);
            return *(float*)src.value - b_val;
            break;
        }

        case DOUBLE:{
            double b_val = va_arg(args, double);
            return *(double*)src.value - b_val;
            break;
        }

        #if __HAVE_FLOAT128
        case LONG_DOUBLE:{
            long double b_val = va_arg(args, long double);
            return *(long double*)src.value - b_val;
            break;
        }
        #endif

        /* -------- CHAR -------- */

        case CHAR:{
            char* b_val = va_arg(args, char*);
            return strcmp((char*)src.value, b_val);
            break;
        }

        case C_OBJ: {
            c_obj_t b = va_arg(args, c_obj_t);
            if(src.obj_type != b.obj_type) return src.obj_type - b.obj_type;

            switch(src.obj_type){
                case UINT8_T:{
                    uint8_t b_val = *(uint8_t*)b.value;
                    return *(uint8_t*)src.value - b_val;
                }
                case UINT16_T:{
                    uint16_t b_val = *(uint16_t*)b.value;
                    return *(uint16_t*)src.value - b_val;
                }
                case UINT32_T:{
                    uint32_t b_val = *(uint32_t*)b.value;
                    return *(uint32_t*)src.value - b_val;
                }
                case UINT64_T:{
                    uint64_t b_val = *(uint64_t*)b.value;
                    return *(uint64_t*)src.value - b_val;
                }
                case INT8_T:{
                    int8_t b_val = *(int8_t*)b.value;
                    return *(int8_t*)src.value - b_val;
                }
                case INT16_T:{
                    int16_t b_val = *(int16_t*)b.value;
                    return *(int16_t*)src.value - b_val;
                }
                case INT32_T:{
                    int32_t b_val = *(int32_t*)b.value;
                    return *(uint32_t*)src.value - b_val;
                }
                case INT64_T:{
                    int64_t b_val = *(uint64_t*)b.value;
                    return *(int64_t*)src.value - b_val;
                }
                case FLOAT:{
                    float b_val = *(float*)b.value;
                    return *(float*)src.value - b_val;
                }
                case DOUBLE:{
                    double b_val = *(double*)b.value;
                    return *(double*)src.value - b_val;
                }
                #if __HAVE_FLOAT128
                case LONG_DOUBLE:{
                    uint8_t b_val = *(uint8_t*)b.value;
                    return *(uint8_t*)src.value - b_val;
                }
                #endif
                case CHAR:{
                    char* b_val = (char*)b.value;
                    return strcmp((char*)src.value, b_val);
                }
            
                default:{
                    while(attribute_map.type_id != type_id) attribute_map = *attribute_map.next;
                    return attribute_map.att->cmp(src.value, b.value, attribute_map);
                }
                
            }
        }

        case END:
            return -1;

        default: {
            void* b = va_arg(args, void*);
            att_map_t current_map = attribute_map;

            while(current_map.type_id != src.obj_type) current_map = *current_map.next;

            return current_map.att->cmp(src.value, b, attribute_map);
        }
    }
}

int8_t obj_std_print(c_obj_t src, ...){
    int8_t return_value;

    va_list args;
    va_start(args, src);

    att_map_t attribute_map = va_arg(args, att_map_t);

    return_value = obj_std_vprint(src, attribute_map, args);

    va_end(args);
    return return_value;
}

int8_t obj_std_vprint(c_obj_t src, att_map_t attribute_map, va_list args){

    switch(src.obj_type) {

        /* -------- UNSIGNED -------- */

        case UINT8_T: 
            printf("%hhu", *(uint8_t*)src.value);
            break;
        

        case UINT16_T:
            printf("%hu", *(uint16_t*)src.value);
            break;

        case UINT32_T:
            printf("%u", *(uint32_t*)src.value);
            break;

        case UINT64_T:
            printf("%lu", *(uint64_t*)src.value);
            break;

        /* -------- SIGNED -------- */
        
        case INT8_T: 
            printf("%hhd", *(int8_t*)src.value);
            break;

        case INT16_T:
            printf("%hd", *(int16_t*)src.value);
            break;

        case INT32_T:
            printf("%d", *(int32_t*)src.value);
            break;

        case INT64_T:
            printf("%ld", *(int64_t*)src.value);
            break;

        /* -------- FLOATING -------- */

        case FLOAT:
            printf("%f", *(float*)src.value);
            break;

        case DOUBLE:
            printf("%lf", *(double*)src.value);
            break;

        #if __HAVE_FLOAT128X
        case LONG_DOUBLE:
            printf("%llf\n", *(long double*)src.value);
            break;
        #endif

        /* -------- CHAR -------- */

        case CHAR:
            printf("%s", (char*)src.value);
            break;

        case END:
            return -1; 

        default: {
            att_map_t current_map = attribute_map;

            while(current_map.type_id != src.obj_type) current_map = *current_map.next;

            current_map.att->print(src.value, current_map);
            break;
        }
    }

    return 1;
}

uint8_t add_maps(att_map_t* dst, ...){
    va_list args;
    va_start(args, dst);

    att_map_t *current = dst;
    int8_t map_type;
    while((map_type = (int8_t)va_arg(args, int32_t)) != END){
        common_attributes_t* new_map = va_arg(args, common_attributes_t*);

        if(current->att == NULL){
            current->att = calloc(1, sizeof(common_attributes_t));
            memcpy(current->att, new_map, sizeof(common_attributes_t));
            current->type_id = map_type;
            current->next = NULL;   
        }else{
            while (current->next != NULL) current = current->next;
            current->next = (att_map_t*)calloc(1, sizeof(att_map_t));
            current = current->next;
            current->att = calloc(1, sizeof(common_attributes_t));
            memcpy(current->att, new_map, sizeof(common_attributes_t));
            current->type_id = map_type;
            current->next = NULL;
        }
    }
}