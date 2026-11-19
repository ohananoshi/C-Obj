/*
 * A custom type in a few lines: describe person_t's properties in a
 * cobj_type, and person values work everywhere a built-in type does.
 */

#include <string.h>

#include "c_hashmap/c_hashmap.h"

typedef struct person_t{
    char name[32];
    uint8_t age;
}person_t;

static int person_cmp(const cobj* a, const cobj* b){
    const person_t* pa = (const person_t*)a->as.p;
    const person_t* pb = (const person_t*)b->as.p;

    int by_name = strcmp(pa->name, pb->name);
    return by_name != 0 ? by_name : pa->age - pb->age;
}

static size_t person_hash(const cobj* self){
    const person_t* p = (const person_t*)self->as.p;
    return cobj_hash_combine(cobj_hash_bytes(p->name, strlen(p->name)), p->age);
}

static cobj_status person_fmt(const cobj* self, cobj_buf* out){
    const person_t* p = (const person_t*)self->as.p;
    return cobj_buf_printf(out, "%s (%d)", p->name, p->age);
}

//size > 0: each value holds its own copy of the struct, cloned and freed by the defaults
static const cobj_type PERSON = {
    .name = "person",
    .size = sizeof(person_t),
    .cmp = person_cmp,
    .hash = person_hash,
    .fmt = person_fmt,
};

int main(void){
    cobj people = cobj_list_value(cobj_list_new());
    cobj_list* list = cobj_list_of(&people);

    cobj_list_push(list, cobj_new(&PERSON, &(person_t){"Ivan", 32}));
    cobj_list_push(list, cobj_new(&PERSON, &(person_t){"Ze", 26}));
    cobj_print(&people);
    printf("\n");

    //person has a hash, so persons can be map keys
    cobj roles = cobj_hashmap_value(cobj_hashmap_new(0));
    const cobj* ivan = cobj_list_get(list, 0);
    cobj_hashmap_set(cobj_hashmap_of(&roles), cobj_clone(ivan), cobj_str("admin"));

    const person_t* p = (const person_t*)cobj_unbox(ivan, &PERSON);
    printf("%s is %s\n", p->name, cobj_as_cstr(cobj_hashmap_get(cobj_hashmap_of(&roles), ivan)));

    cobj_drop(&people);
    cobj_drop(&roles);
    return 0;
}
