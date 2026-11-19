#include <stdlib.h>

#include "c_set.h"

//Every element is a key of the map; all keys share the same inline value
struct cobj_set{
    cobj_hashmap* map;
};

static cobj member(void){
    return cobj_u8(1);
}

cobj_set* cobj_set_new(void){
    cobj_set* set = (cobj_set*)cobj_malloc(sizeof(cobj_set));
    if(set == NULL) return NULL;

    set->map = cobj_hashmap_new(0);
    if(set->map == NULL){
        cobj_free(set);
        return NULL;
    }
    return set;
}

void cobj_set_free(cobj_set* set){
    if(set == NULL) return;

    cobj_hashmap_free(set->map);
    cobj_free(set);
}

size_t cobj_set_len(const cobj_set* set){
    return set != NULL ? cobj_hashmap_len(set->map) : 0;
}

cobj_status cobj_set_add(cobj_set* set, cobj value){
    if(set == NULL){
        cobj_drop(&value);
        return COBJ_EINVAL;
    }

    //An existing equal key stays in place and the new one is dropped
    return cobj_hashmap_set(set->map, value, member());
}

cobj_status cobj_set_remove(cobj_set* set, const cobj* value){
    if(set == NULL) return COBJ_EINVAL;
    return cobj_hashmap_remove(set->map, value, NULL);
}

bool cobj_set_contains(const cobj_set* set, const cobj* value){
    return set != NULL && cobj_hashmap_contains(set->map, value);
}

cobj_set* cobj_set_from_values(size_t count, cobj* items){
    if(count > 0 && items == NULL) return NULL;

    cobj_set* set = cobj_set_new();
    for(size_t i = 0; i < count; i++){
        //cobj_set_add consumes the item; once a step fails, the rest are dropped
        if(set == NULL) cobj_drop(&items[i]);
        else if(cobj_set_add(set, items[i]) != COBJ_OK){
            cobj_set_free(set);
            set = NULL;
        }
        items[i] = cobj_empty();
    }
    return set;
}

cobj_set* cobj_set_from(const cobj* iterable){
    if(!cobj_iterable(iterable)) return NULL;

    cobj_set* set = cobj_set_new();
    if(set == NULL) return NULL;

    COBJ_FOREACH(item, iterable){
        if(cobj_set_add(set, cobj_clone(&item)) != COBJ_OK){
            cobj_set_free(set);
            return NULL;
        }
    }
    return set;
}

//Adds copies of src's elements to dst; with a filter, only those whose presence in it equals wanted
static bool add_copies(cobj_set* dst, const cobj_set* src, const cobj_set* filter, bool wanted){
    size_t position = 0;
    const cobj* element;

    while(cobj_hashmap_next(src->map, &position, &element, NULL)){
        if(filter != NULL && cobj_set_contains(filter, element) != wanted) continue;
        if(cobj_set_add(dst, cobj_clone(element)) != COBJ_OK) return false;
    }
    return true;
}

cobj_set* cobj_set_union(const cobj_set* a, const cobj_set* b){
    if(a == NULL || b == NULL) return NULL;

    cobj_set* result = cobj_set_new();
    if(result == NULL || !add_copies(result, a, NULL, true) || !add_copies(result, b, NULL, true)){
        cobj_set_free(result);
        return NULL;
    }
    return result;
}

cobj_set* cobj_set_intersection(const cobj_set* a, const cobj_set* b){
    if(a == NULL || b == NULL) return NULL;

    cobj_set* result = cobj_set_new();
    if(result == NULL || !add_copies(result, a, b, true)){
        cobj_set_free(result);
        return NULL;
    }
    return result;
}

cobj_set* cobj_set_difference(const cobj_set* a, const cobj_set* b){
    if(a == NULL || b == NULL) return NULL;

    cobj_set* result = cobj_set_new();
    if(result == NULL || !add_copies(result, a, b, false)){
        cobj_set_free(result);
        return NULL;
    }
    return result;
}

bool cobj_set_is_subset(const cobj_set* a, const cobj_set* b){
    if(a == NULL || b == NULL || cobj_set_len(a) > cobj_set_len(b)) return false;

    size_t position = 0;
    const cobj* element;
    while(cobj_hashmap_next(a->map, &position, &element, NULL)){
        if(!cobj_set_contains(b, element)) return false;
    }
    return true;
}

cobj cobj_set_value(cobj_set* set){
    return cobj_box(&COBJ_SET, set);
}

cobj_set* cobj_set_of(const cobj* value){
    return (cobj_set*)cobj_unbox(value, &COBJ_SET);
}

//======================================= PROPERTIES ==========================================

//A borrowed map value: only read, never dropped
static cobj map_view(const cobj* self){
    cobj view = {&COBJ_HASHMAP, {.p = ((const cobj_set*)self->as.p)->map}};
    return view;
}

static cobj_status set_clone(cobj* dst, const cobj* src){
    cobj_set* copy = cobj_set_new();
    if(copy == NULL || !add_copies(copy, (const cobj_set*)src->as.p, NULL, true)){
        cobj_set_free(copy);
        return COBJ_ENOMEM;
    }

    dst->as.p = copy;
    return COBJ_OK;
}

static void set_drop(cobj* self){
    cobj_set_free((cobj_set*)self->as.p);
}

//The maps order and hash their keys independently of insertion order, which is what sets need
static int set_cmp(const cobj* a, const cobj* b){
    cobj map_a = map_view(a);
    cobj map_b = map_view(b);
    return cobj_cmp(&map_a, &map_b);
}

static size_t set_hash(const cobj* self){
    cobj map = map_view(self);
    return cobj_hash(&map);
}

//"{a, b}", and "set()" when empty as in Python ("{}" would read as an empty map)
static cobj_status set_fmt(const cobj* self, cobj_buf* out){
    const cobj_set* set = (const cobj_set*)self->as.p;
    if(cobj_set_len(set) == 0) return cobj_buf_puts(out, "set()");

    cobj_status status = cobj_buf_puts(out, "{");
    size_t position = 0;
    const cobj* element;
    for(int first = 1; status == COBJ_OK && cobj_hashmap_next(set->map, &position, &element, NULL); first = 0){
        if(!first) status = cobj_buf_puts(out, ", ");
        if(status == COBJ_OK) status = cobj_fmt(element, out);
    }

    return status == COBJ_OK ? cobj_buf_puts(out, "}") : status;
}

static size_t set_len(const cobj* self){
    return cobj_set_len((const cobj_set*)self->as.p);
}

static bool set_next(const cobj* self, cobj_iter* it, cobj* out){
    const cobj* element;
    if(!cobj_hashmap_next(((const cobj_set*)self->as.p)->map, &it->index, &element, NULL)) return false;

    *out = *element;
    return true;
}

static bool set_contains(const cobj* self, const cobj* value){
    return cobj_set_contains((const cobj_set*)self->as.p, value);
}

const cobj_type COBJ_SET = {.name = "set", .clone = set_clone, .drop = set_drop,
                            .cmp = set_cmp, .hash = set_hash, .fmt = set_fmt,
                            .len = set_len, .next = set_next, .contains = set_contains};
