#include <stdlib.h>

#include "c_list.h"

struct cobj_list{
    cobj* items;
    size_t len;
    size_t cap;
};

static cobj_status list_reserve(cobj_list* list, size_t cap){
    if(cap <= list->cap) return COBJ_OK;

    size_t new_cap = list->cap > 0 ? list->cap * 2 : 4;
    while(new_cap < cap) new_cap *= 2;

    cobj* grown = (cobj*)cobj_realloc(list->items, new_cap * sizeof(cobj));
    if(grown == NULL) return COBJ_ENOMEM;

    list->items = grown;
    list->cap = new_cap;
    return COBJ_OK;
}

cobj_list* cobj_list_new(void){
    return (cobj_list*)cobj_calloc(1, sizeof(cobj_list));
}

void cobj_list_free(cobj_list* list){
    if(list == NULL) return;

    for(size_t i = 0; i < list->len; i++) cobj_drop(&list->items[i]);
    cobj_free(list->items);
    cobj_free(list);
}

size_t cobj_list_len(const cobj_list* list){
    return list != NULL ? list->len : 0;
}

cobj_status cobj_list_push(cobj_list* list, cobj value){
    if(list == NULL || cobj_is_empty(&value)){
        cobj_drop(&value);
        return COBJ_EINVAL;
    }

    if(list_reserve(list, list->len + 1) != COBJ_OK){
        cobj_drop(&value);
        return COBJ_ENOMEM;
    }

    list->items[list->len++] = value;
    return COBJ_OK;
}

const cobj* cobj_list_get(const cobj_list* list, size_t index){
    return list != NULL && index < list->len ? &list->items[index] : NULL;
}

cobj_status cobj_list_set(cobj_list* list, size_t index, cobj value){
    if(list == NULL || cobj_is_empty(&value)){
        cobj_drop(&value);
        return COBJ_EINVAL;
    }

    if(index >= list->len){
        cobj_drop(&value);
        return COBJ_ERANGE;
    }

    cobj_drop(&list->items[index]);
    list->items[index] = value;
    return COBJ_OK;
}

cobj_status cobj_list_pop(cobj_list* list, cobj* out){
    if(out != NULL) *out = cobj_empty();
    if(list == NULL || out == NULL) return COBJ_EINVAL;
    if(list->len == 0) return COBJ_ERANGE;

    *out = list->items[--list->len];
    return COBJ_OK;
}

cobj_list* cobj_list_from_values(size_t count, cobj* items){
    if(count > 0 && items == NULL) return NULL;

    cobj_list* list = cobj_list_new();
    bool valid = list != NULL && list_reserve(list, count) == COBJ_OK;
    for(size_t i = 0; i < count; i++){
        if(cobj_is_empty(&items[i])) valid = false;
    }

    if(!valid){
        for(size_t i = 0; i < count; i++) cobj_drop(&items[i]);
        cobj_list_free(list);
        return NULL;
    }

    for(size_t i = 0; i < count; i++){
        list->items[list->len++] = items[i];
        items[i] = cobj_empty();
    }
    return list;
}

cobj_list* cobj_list_from(const cobj* iterable){
    if(!cobj_iterable(iterable)) return NULL;

    cobj_list* list = cobj_list_new();
    if(list == NULL || list_reserve(list, cobj_len(iterable)) != COBJ_OK){
        cobj_list_free(list);
        return NULL;
    }

    COBJ_FOREACH(item, iterable){
        if(cobj_list_push(list, cobj_clone(&item)) != COBJ_OK){
            cobj_list_free(list);
            return NULL;
        }
    }
    return list;
}

static int item_cmp(const void* a, const void* b){
    return cobj_cmp((const cobj*)a, (const cobj*)b);
}

void cobj_list_sort(cobj_list* list){
    if(list != NULL && list->len > 1) qsort(list->items, list->len, sizeof(cobj), item_cmp);
}

cobj cobj_list_value(cobj_list* list){
    return cobj_box(&COBJ_LIST, list);
}

cobj_list* cobj_list_of(const cobj* value){
    return (cobj_list*)cobj_unbox(value, &COBJ_LIST);
}

//======================================= PROPERTIES ==========================================

static cobj_status list_clone(cobj* dst, const cobj* src){
    const cobj_list* source = (const cobj_list*)src->as.p;
    cobj_list* copy = cobj_list_new();

    if(copy == NULL || list_reserve(copy, source->len) != COBJ_OK){
        cobj_list_free(copy);
        return COBJ_ENOMEM;
    }

    for(size_t i = 0; i < source->len; i++){
        cobj item = cobj_clone(&source->items[i]);
        if(cobj_is_empty(&item)){
            cobj_list_free(copy);
            return COBJ_ENOMEM;
        }
        copy->items[copy->len++] = item;
    }

    dst->as.p = copy;
    return COBJ_OK;
}

static void list_drop(cobj* self){
    cobj_list_free((cobj_list*)self->as.p);
}

//Lexicographic: the first different element decides, then the shorter list comes first
static int list_cmp(const cobj* a, const cobj* b){
    const cobj_list* list_a = (const cobj_list*)a->as.p;
    const cobj_list* list_b = (const cobj_list*)b->as.p;
    size_t common = list_a->len < list_b->len ? list_a->len : list_b->len;

    for(size_t i = 0; i < common; i++){
        int result = cobj_cmp(&list_a->items[i], &list_b->items[i]);
        if(result != 0) return result;
    }

    return (list_a->len > list_b->len) - (list_a->len < list_b->len);
}

static size_t list_hash(const cobj* self){
    const cobj_list* list = (const cobj_list*)self->as.p;
    size_t hash = list->len;

    for(size_t i = 0; i < list->len; i++) hash = cobj_hash_combine(hash, cobj_hash(&list->items[i]));
    return hash;
}

static cobj_status list_fmt(const cobj* self, cobj_buf* out){
    const cobj_list* list = (const cobj_list*)self->as.p;
    cobj_status status = cobj_buf_puts(out, "[");

    for(size_t i = 0; i < list->len && status == COBJ_OK; i++){
        if(i > 0) status = cobj_buf_puts(out, ", ");
        if(status == COBJ_OK) status = cobj_fmt(&list->items[i], out);
    }

    return status == COBJ_OK ? cobj_buf_puts(out, "]") : status;
}

static size_t list_len(const cobj* self){
    return ((const cobj_list*)self->as.p)->len;
}

static bool list_next(const cobj* self, cobj_iter* it, cobj* out){
    const cobj_list* list = (const cobj_list*)self->as.p;
    if(it->index >= list->len) return false;

    *out = list->items[it->index++];
    return true;
}

const cobj_type COBJ_LIST = {.name = "list", .clone = list_clone, .drop = list_drop,
                             .cmp = list_cmp, .hash = list_hash, .fmt = list_fmt,
                             .len = list_len, .next = list_next};
