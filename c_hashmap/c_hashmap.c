#include "c_hashmap.h"

int8_t hashmap_obj_set(c_obj_t* dst, ...){
    va_list args;
    va_start(args, dst);

    hashmap_t src = *(hashmap_t*)va_arg(args, void*);
    att_map_t attribute_map = va_arg(args, att_map_t);

    dst->value = calloc(1, sizeof(hashmap_t));
    hashmap_t *dst_map = (hashmap_t*)dst->value;

    hashmap_init(dst_map, src.capacity, src.hash);


    for(size_t i = 0; i < src.capacity; i++){
        map_bucket_t current_src_bucket = src.buckets[i];
        map_bucket_t *current_dst_bucket = &dst_map->buckets[i];

        while(current_src_bucket.key.value != NULL){

            obj_std_set(&(current_dst_bucket->key), attribute_map, C_OBJ, current_src_bucket.key);
            obj_std_set(&(current_dst_bucket->value), attribute_map, C_OBJ, current_src_bucket.value);

            if(current_src_bucket.next == NULL){
                current_dst_bucket->next = (map_bucket_t*)calloc(1, sizeof(map_bucket_t));
                current_dst_bucket->next = NULL;
                break;
            }
            current_src_bucket = *current_src_bucket.next;
            current_dst_bucket->next = (map_bucket_t*)calloc(1, sizeof(map_bucket_t));
            current_dst_bucket = current_dst_bucket->next;
        }
    }
    
}

int8_t hashmap_obj_get(c_obj_t src, ...){
    va_list args;
    va_start(args, src);

    
    hashmap_t src_map = *(hashmap_t*)src.value;
    hashmap_t *dst_map = (hashmap_t*)va_arg(args, void*);
    att_map_t attribute_map = va_arg(args, att_map_t);

    hashmap_init(dst_map, src_map.capacity, src_map.hash);


    for(size_t i = 0; i < src_map.capacity; i++){
        map_bucket_t current_src_bucket = src_map.buckets[i];
        map_bucket_t *current_dst_bucket = &dst_map->buckets[i];

        while(current_src_bucket.key.value != NULL){

            obj_std_set(&(current_dst_bucket->key), attribute_map, C_OBJ, current_src_bucket.key);
            obj_std_set(&(current_dst_bucket->value), attribute_map, C_OBJ, current_src_bucket.value);

            if(current_src_bucket.next == NULL){
                current_dst_bucket->next = (map_bucket_t*)calloc(1, sizeof(map_bucket_t));
                current_dst_bucket->next = NULL;
                break;
            }
            current_src_bucket = *current_src_bucket.next;
            current_dst_bucket->next = (map_bucket_t*)calloc(1, sizeof(map_bucket_t));
            current_dst_bucket = current_dst_bucket->next;
        }
    }
}

int32_t hashmap_cmp(void* src, ...){
    va_list args;
    va_start(args, src);

    hashmap_t b_map = *(hashmap_t*)va_arg(args, void*);
    att_map_t attribute_map = va_arg(args, att_map_t);
    hashmap_t src_map = *(hashmap_t*)src;

    int32_t key_cmp = 0, value_cmp = 0;
    for(size_t i = 0; i < src_map.capacity; i++){
        map_bucket_t current_src_bucket = src_map.buckets[i];
        map_bucket_t current_b_bucket = b_map.buckets[i];

        while(current_src_bucket.key.value != NULL){


            key_cmp = obj_std_cmp(current_src_bucket.key, attribute_map, C_OBJ, current_b_bucket.key);
            value_cmp = obj_std_cmp(current_src_bucket.value, attribute_map, C_OBJ, current_b_bucket.value);

            if(key_cmp != 0 || value_cmp != 0){
                if((key_cmp + value_cmp) != 0) return key_cmp + value_cmp;
                else return key_cmp;
            }

            if(current_src_bucket.next == NULL) break;
            current_src_bucket = *current_src_bucket.next;
            current_b_bucket = *current_b_bucket.next;
        }
    }
    
    return key_cmp + value_cmp;
}

int8_t hashmap_print(void* src, ...){
    size_t key_count = 0;
    hashmap_t src_map = *(hashmap_t*)src;

    va_list args;
    va_start(args, src);

    att_map_t attribute_map = va_arg(args, att_map_t);

    printf("{");
    for(size_t i = 0; i < src_map.capacity; i++){
        map_bucket_t current_bucket = src_map.buckets[i];

        while(current_bucket.key.value != NULL){

            obj_std_print(current_bucket.key, attribute_map);
            printf(":");
            obj_std_print(current_bucket.value, attribute_map);
            printf(",");

            if(current_bucket.next == NULL) break;
            current_bucket = *current_bucket.next;
        }
    }
    printf("\b}");
}

size_t str_hash(void* key){
    const char* str = (const char*)key;
    size_t hash = 5381;
    int32_t c;
    while((c = *str++)) hash = ((hash << 5) + hash) + c;

    return hash;
}

void hashmap_init(hashmap_t* dst, size_t capacity, hash_fn_t hash_fn){
    dst->capacity = capacity;
    dst->hash = hash_fn;
    dst->buckets = (map_bucket_t*)calloc(capacity, sizeof(map_bucket_t));
}

void hashmap_insert(hashmap_t* dst, att_map_t attribute_map, ...){
    va_list args;
    va_start(args, attribute_map);

    c_obj_t tmp_key = {NULL, 0};
    obj_std_vset(&tmp_key, attribute_map, args);

    size_t index = dst->hash(tmp_key.value) % dst->capacity;
    map_bucket_t *current_bucket = &(dst->buckets[index]);

    while(current_bucket->key.value != NULL){
        if(obj_std_cmp(current_bucket->key, attribute_map, tmp_key.obj_type, tmp_key.value) == 0){
            free(current_bucket->value.value);
            obj_std_vset(&(current_bucket->value), attribute_map, args);
            return;
        }
        current_bucket = current_bucket->next;
    }

    obj_std_set(&(current_bucket->key), attribute_map, C_OBJ, tmp_key);
    obj_std_vset(&(current_bucket->value), attribute_map, args);
    current_bucket->next = (map_bucket_t*)calloc(1, sizeof(map_bucket_t));
    current_bucket->next = NULL;

    va_end(args);
}

void* hashmap_get(hashmap_t src, att_map_t attribute_map, ...){
    va_list args;
    va_start(args, attribute_map);

    c_obj_t tmp_key;
    obj_std_vset(&tmp_key, attribute_map, args);

    size_t index = src.hash(tmp_key.value) % src.capacity;
    map_bucket_t current_bucket = src.buckets[index];

    while(current_bucket.key.value != NULL){
        if(obj_std_cmp(current_bucket.key, attribute_map, tmp_key.obj_type, tmp_key.value) == 0)
            return current_bucket.value.value;

        current_bucket = *current_bucket.next;
    }

    va_end(args);
    return NULL;
}

size_t hashmap_keys(hashmap_t src, att_map_t attribute_map, c_obj_t** dst){
    size_t key_count = 0;
    //c_obj_t *dst_obj = *dst;

    for(size_t i = 0; i < src.capacity; i++){
        map_bucket_t current_bucket = src.buckets[i];

        while(current_bucket.key.value != NULL){
            key_count++;
            *dst = (c_obj_t*)realloc(*dst, key_count*sizeof(c_obj_t));

            obj_std_set(&((*dst)[key_count - 1]), attribute_map, C_OBJ, current_bucket.key);

            if(current_bucket.next == NULL) break;
            current_bucket = *current_bucket.next;
        }
    }

    return key_count;
}

size_t hashmap_values(hashmap_t src, att_map_t attribute_map, c_obj_t** dst){
    size_t key_count = 0;
    //c_obj_t *dst_obj = *dst;

    for(size_t i = 0; i < src.capacity; i++){
        map_bucket_t current_bucket = src.buckets[i];

        while(current_bucket.key.value != NULL){
            key_count++;
            *dst = (c_obj_t*)realloc(*dst, key_count*sizeof(c_obj_t));

            obj_std_set(&((*dst)[key_count - 1]), attribute_map, C_OBJ, current_bucket.value);

            if(current_bucket.next == NULL) break;
            current_bucket = *current_bucket.next;
        }
    }

    return key_count;
}

size_t hashmap_items(hashmap_t src, att_map_t attribute_map, c_obj_t*** dst){
    size_t key_count = 0;
    //c_obj_t *dst_obj = *dst;

    for(size_t i = 0; i < src.capacity; i++){
        map_bucket_t current_bucket = src.buckets[i];

        while(current_bucket.key.value != NULL){
            key_count++;
            *dst = (c_obj_t**)realloc(*dst, key_count*sizeof(c_obj_t*));
            (*dst)[key_count - 1] = (c_obj_t*)calloc(2, sizeof(c_obj_t));

            obj_std_set(&((*dst)[key_count - 1][0]), attribute_map, C_OBJ, current_bucket.key);
            obj_std_set(&((*dst)[key_count - 1][1]), attribute_map, C_OBJ, current_bucket.value);

            if(current_bucket.next == NULL) break;
            current_bucket = *current_bucket.next;
        }
    }

    return key_count;
}

common_attributes_t hashmap_att = {hashmap_obj_set, hashmap_obj_get, hashmap_cmp, hashmap_print};

