#include <stdint.h>
#include <stdlib.h>

#include "c_tuple.h"

//One allocation: the elements follow the length
struct cobj_tuple{
    size_t len;
    cobj items[];
};

static cobj_tuple* tuple_alloc(size_t len){
    if(len > (SIZE_MAX - sizeof(cobj_tuple)) / sizeof(cobj)) return NULL;
    return (cobj_tuple*)cobj_malloc(sizeof(cobj_tuple) + len * sizeof(cobj));
}

cobj_tuple* cobj_tuple_new(size_t len, cobj* items){
    if(len > 0 && items == NULL) return NULL;

    int valid = 1;
    for(size_t i = 0; i < len; i++){
        if(cobj_is_empty(&items[i])) valid = 0;
    }

    cobj_tuple* tuple = valid ? tuple_alloc(len) : NULL;
    if(tuple == NULL){
        for(size_t i = 0; i < len; i++) cobj_drop(&items[i]);
        return NULL;
    }

    tuple->len = len;
    for(size_t i = 0; i < len; i++){
        tuple->items[i] = items[i];
        items[i] = cobj_empty();
    }
    return tuple;
}

void cobj_tuple_free(cobj_tuple* tuple){
    if(tuple == NULL) return;

    for(size_t i = 0; i < tuple->len; i++) cobj_drop(&tuple->items[i]);
    cobj_free(tuple);
}

size_t cobj_tuple_len(const cobj_tuple* tuple){
    return tuple != NULL ? tuple->len : 0;
}

const cobj* cobj_tuple_get(const cobj_tuple* tuple, size_t index){
    return tuple != NULL && index < tuple->len ? &tuple->items[index] : NULL;
}

cobj cobj_tuple_value(cobj_tuple* tuple){
    return cobj_box(&COBJ_TUPLE, tuple);
}

cobj_tuple* cobj_tuple_of(const cobj* value){
    return (cobj_tuple*)cobj_unbox(value, &COBJ_TUPLE);
}

//======================================= PROPERTIES ==========================================

static cobj_status tuple_clone(cobj* dst, const cobj* src){
    const cobj_tuple* source = (const cobj_tuple*)src->as.p;
    cobj_tuple* copy = tuple_alloc(source->len);
    if(copy == NULL) return COBJ_ENOMEM;

    copy->len = 0;
    for(size_t i = 0; i < source->len; i++){
        copy->items[i] = cobj_clone(&source->items[i]);
        if(cobj_is_empty(&copy->items[i])){
            cobj_tuple_free(copy);
            return COBJ_ENOMEM;
        }
        copy->len++;
    }

    dst->as.p = copy;
    return COBJ_OK;
}

static void tuple_drop(cobj* self){
    cobj_tuple_free((cobj_tuple*)self->as.p);
}

//Lexicographic: the first different element decides, then the shorter tuple comes first
static int tuple_cmp(const cobj* a, const cobj* b){
    const cobj_tuple* tuple_a = (const cobj_tuple*)a->as.p;
    const cobj_tuple* tuple_b = (const cobj_tuple*)b->as.p;
    size_t common = tuple_a->len < tuple_b->len ? tuple_a->len : tuple_b->len;

    for(size_t i = 0; i < common; i++){
        int result = cobj_cmp(&tuple_a->items[i], &tuple_b->items[i]);
        if(result != 0) return result;
    }

    return (tuple_a->len > tuple_b->len) - (tuple_a->len < tuple_b->len);
}

static size_t tuple_hash(const cobj* self){
    const cobj_tuple* tuple = (const cobj_tuple*)self->as.p;
    size_t hash = tuple->len;

    for(size_t i = 0; i < tuple->len; i++) hash = cobj_hash_combine(hash, cobj_hash(&tuple->items[i]));
    return hash;
}

//"(a, b)", and "(a,)" for a single element
static cobj_status tuple_fmt(const cobj* self, cobj_buf* out){
    const cobj_tuple* tuple = (const cobj_tuple*)self->as.p;
    cobj_status status = cobj_buf_puts(out, "(");

    for(size_t i = 0; i < tuple->len && status == COBJ_OK; i++){
        if(i > 0) status = cobj_buf_puts(out, ", ");
        if(status == COBJ_OK) status = cobj_fmt(&tuple->items[i], out);
    }

    if(status == COBJ_OK && tuple->len == 1) status = cobj_buf_puts(out, ",");
    return status == COBJ_OK ? cobj_buf_puts(out, ")") : status;
}

static size_t tuple_len(const cobj* self){
    return ((const cobj_tuple*)self->as.p)->len;
}

static bool tuple_next(const cobj* self, cobj_iter* it, cobj* out){
    const cobj_tuple* tuple = (const cobj_tuple*)self->as.p;
    if(it->index >= tuple->len) return false;

    *out = tuple->items[it->index++];
    return true;
}

const cobj_type COBJ_TUPLE = {.name = "tuple", .clone = tuple_clone, .drop = tuple_drop,
                              .cmp = tuple_cmp, .hash = tuple_hash, .fmt = tuple_fmt,
                              .len = tuple_len, .next = tuple_next};
