#include <stdint.h>
#include <stdlib.h>

#include "c_hashmap.h"

/*
 * Compact layout, as in CPython's dict: entries[] keeps the entries in
 * insertion order, and index[] is an open-addressing table (linear probing)
 * of positions in entries[]. Removing an entry empties its key and marks
 * its slot DELETED; both are cleaned up when the map is rebuilt.
 */

#define DEFAULT_CAPACITY 8
#define EMPTY_SLOT 0
#define DELETED_SLOT SIZE_MAX
#define NOT_FOUND SIZE_MAX
#define CMP3(a, b) (((a) > (b)) - ((a) < (b)))

typedef struct entry{
    cobj key;        //empty once the entry is removed
    cobj value;
    size_t hash;
}entry;

struct cobj_hashmap{
    entry* entries;
    size_t used;          //entries written so far, removed ones included
    size_t entries_cap;   //2/3 of index_cap: the table never gets fuller than that
    size_t len;           //live entries
    size_t* index;        //EMPTY_SLOT, DELETED_SLOT, or the entry's position + 1
    size_t index_cap;     //a power of two
};

//The slot holding key, or NOT_FOUND; *free_slot gets the slot where key would be inserted
static size_t find_slot(const cobj_hashmap* map, const cobj* key, size_t hash, size_t* free_slot){
    size_t mask = map->index_cap - 1;
    size_t first_deleted = NOT_FOUND;

    for(size_t slot = hash & mask;; slot = (slot + 1) & mask){
        size_t position = map->index[slot];

        if(position == EMPTY_SLOT){
            if(free_slot != NULL) *free_slot = first_deleted != NOT_FOUND ? first_deleted : slot;
            return NOT_FOUND;
        }

        if(position == DELETED_SLOT){
            if(first_deleted == NOT_FOUND) first_deleted = slot;
            continue;
        }

        const entry* candidate = &map->entries[position - 1];
        if(candidate->hash == hash && cobj_equals(&candidate->key, key)) return slot;
    }
}

//Rebuilds the table for at least `needed` entries, dropping removed entries and keeping the order
static cobj_status rebuild(cobj_hashmap* map, size_t needed){
    size_t index_cap = 8;
    while(index_cap / 3 * 2 < needed){
        if(index_cap > SIZE_MAX / 4 / sizeof(entry)) return COBJ_ENOMEM;
        index_cap *= 2;
    }

    size_t entries_cap = index_cap / 3 * 2;
    size_t* index = (size_t*)cobj_calloc(index_cap, sizeof(size_t));
    entry* entries = (entry*)cobj_malloc(entries_cap * sizeof(entry));
    if(index == NULL || entries == NULL){
        cobj_free(index);
        cobj_free(entries);
        return COBJ_ENOMEM;
    }

    size_t mask = index_cap - 1;
    size_t count = 0;
    for(size_t i = 0; i < map->used; i++){
        if(cobj_is_empty(&map->entries[i].key)) continue;

        entries[count] = map->entries[i];
        size_t slot = entries[count].hash & mask;
        while(index[slot] != EMPTY_SLOT) slot = (slot + 1) & mask;
        index[slot] = count + 1;
        count++;
    }

    cobj_free(map->entries);
    cobj_free(map->index);
    map->entries = entries;
    map->entries_cap = entries_cap;
    map->index = index;
    map->index_cap = index_cap;
    map->used = count;
    return COBJ_OK;
}

cobj_hashmap* cobj_hashmap_new(size_t capacity){
    cobj_hashmap* map = (cobj_hashmap*)cobj_calloc(1, sizeof(cobj_hashmap));
    if(map == NULL) return NULL;

    if(rebuild(map, capacity > 0 ? capacity : DEFAULT_CAPACITY) != COBJ_OK){
        cobj_free(map);
        return NULL;
    }
    return map;
}

cobj_hashmap* cobj_hashmap_from_pairs(size_t count, cobj* items){
    if(count > 0 && items == NULL) return NULL;

    cobj_hashmap* map = count % 2 == 0 ? cobj_hashmap_new(count / 2) : NULL;
    for(size_t i = 0; i < count; i += 2){
        //cobj_hashmap_set consumes the pair; once a step fails, the rest are dropped
        if(map == NULL || i + 1 == count){
            cobj_drop(&items[i]);
            if(i + 1 < count) cobj_drop(&items[i + 1]);
        }else if(cobj_hashmap_set(map, items[i], items[i + 1]) != COBJ_OK){
            cobj_hashmap_free(map);
            map = NULL;
        }

        items[i] = cobj_empty();
        if(i + 1 < count) items[i + 1] = cobj_empty();
    }
    return map;
}

void cobj_hashmap_free(cobj_hashmap* map){
    if(map == NULL) return;

    for(size_t i = 0; i < map->used; i++){
        cobj_drop(&map->entries[i].key);
        cobj_drop(&map->entries[i].value);
    }

    cobj_free(map->entries);
    cobj_free(map->index);
    cobj_free(map);
}

size_t cobj_hashmap_len(const cobj_hashmap* map){
    return map != NULL ? map->len : 0;
}

cobj_status cobj_hashmap_set(cobj_hashmap* map, cobj key, cobj value){
    cobj_status status = COBJ_OK;
    if(map == NULL || cobj_is_empty(&key) || cobj_is_empty(&value)) status = COBJ_EINVAL;
    else if(!cobj_hashable(&key)) status = COBJ_ETYPE;

    if(status != COBJ_OK){
        cobj_drop(&key);
        cobj_drop(&value);
        return status;
    }

    size_t hash = cobj_hash(&key);
    size_t free_slot = 0;
    size_t slot = find_slot(map, &key, hash, &free_slot);

    if(slot != NOT_FOUND){
        entry* existing = &map->entries[map->index[slot] - 1];
        cobj_drop(&existing->value);
        existing->value = value;
        cobj_drop(&key);
        return COBJ_OK;
    }

    if(map->used == map->entries_cap){
        if(rebuild(map, (map->len + 1) * 2) != COBJ_OK){
            cobj_drop(&key);
            cobj_drop(&value);
            return COBJ_ENOMEM;
        }
        find_slot(map, &key, hash, &free_slot);
    }

    entry* added = &map->entries[map->used];
    added->key = key;
    added->value = value;
    added->hash = hash;
    map->index[free_slot] = map->used + 1;
    map->used++;
    map->len++;
    return COBJ_OK;
}

//The entry holding key, or NULL
static entry* find_entry(const cobj_hashmap* map, const cobj* key){
    if(map == NULL || !cobj_hashable(key)) return NULL;

    size_t slot = find_slot(map, key, cobj_hash(key), NULL);
    return slot != NOT_FOUND ? &map->entries[map->index[slot] - 1] : NULL;
}

const cobj* cobj_hashmap_get(const cobj_hashmap* map, const cobj* key){
    entry* found = find_entry(map, key);
    return found != NULL ? &found->value : NULL;
}

const cobj* cobj_hashmap_get_str(const cobj_hashmap* map, const char* key){
    if(key == NULL) return NULL;

    //A borrowed COBJ_STR: only read, never dropped
    cobj borrowed = {&COBJ_STR, {.p = (void*)key}};
    return cobj_hashmap_get(map, &borrowed);
}

bool cobj_hashmap_contains(const cobj_hashmap* map, const cobj* key){
    return find_entry(map, key) != NULL;
}

cobj_status cobj_hashmap_remove(cobj_hashmap* map, const cobj* key, cobj* out){
    if(out != NULL) *out = cobj_empty();
    if(map == NULL || cobj_is_empty(key)) return COBJ_EINVAL;
    if(!cobj_hashable(key)) return COBJ_ENOTFOUND;

    size_t slot = find_slot(map, key, cobj_hash(key), NULL);
    if(slot == NOT_FOUND) return COBJ_ENOTFOUND;

    entry* removed = &map->entries[map->index[slot] - 1];
    map->index[slot] = DELETED_SLOT;
    cobj_drop(&removed->key);

    if(out != NULL) *out = removed->value;
    else cobj_drop(&removed->value);
    removed->value = cobj_empty();

    map->len--;
    return COBJ_OK;
}

bool cobj_hashmap_next(const cobj_hashmap* map, size_t* position, const cobj** key, const cobj** value){
    if(map == NULL || position == NULL) return false;

    while(*position < map->used){
        const entry* current = &map->entries[(*position)++];
        if(cobj_is_empty(&current->key)) continue;

        if(key != NULL) *key = &current->key;
        if(value != NULL) *value = &current->value;
        return true;
    }
    return false;
}

static cobj key_copy(const cobj* key, const cobj* value){
    (void)value;
    return cobj_clone(key);
}

static cobj value_copy(const cobj* key, const cobj* value){
    (void)key;
    return cobj_clone(value);
}

static cobj item_copy(const cobj* key, const cobj* value){
    cobj pair[2] = {cobj_clone(key), cobj_clone(value)};
    return cobj_tuple_value(cobj_tuple_new(2, pair));
}

//A new list with make(key, value) for every entry
static cobj_list* collect(const cobj_hashmap* map, cobj (*make)(const cobj*, const cobj*)){
    if(map == NULL) return NULL;

    cobj_list* list = cobj_list_new();
    if(list == NULL) return NULL;

    size_t position = 0;
    const cobj* key;
    const cobj* value;
    while(cobj_hashmap_next(map, &position, &key, &value)){
        if(cobj_list_push(list, make(key, value)) != COBJ_OK){
            cobj_list_free(list);
            return NULL;
        }
    }
    return list;
}

cobj_list* cobj_hashmap_keys(const cobj_hashmap* map){
    return collect(map, key_copy);
}

cobj_list* cobj_hashmap_values(const cobj_hashmap* map){
    return collect(map, value_copy);
}

cobj_list* cobj_hashmap_items(const cobj_hashmap* map){
    return collect(map, item_copy);
}

cobj cobj_hashmap_value(cobj_hashmap* map){
    return cobj_box(&COBJ_HASHMAP, map);
}

cobj_hashmap* cobj_hashmap_of(const cobj* value){
    return (cobj_hashmap*)cobj_unbox(value, &COBJ_HASHMAP);
}

//======================================= PROPERTIES ==========================================

static cobj_status hashmap_clone(cobj* dst, const cobj* src){
    const cobj_hashmap* source = (const cobj_hashmap*)src->as.p;
    cobj_hashmap* copy = cobj_hashmap_new(source->len);
    if(copy == NULL) return COBJ_ENOMEM;

    size_t position = 0;
    const cobj* key;
    const cobj* value;
    while(cobj_hashmap_next(source, &position, &key, &value)){
        if(cobj_hashmap_set(copy, cobj_clone(key), cobj_clone(value)) != COBJ_OK){
            cobj_hashmap_free(copy);
            return COBJ_ENOMEM;
        }
    }

    dst->as.p = copy;
    return COBJ_OK;
}

static void hashmap_drop(cobj* self){
    cobj_hashmap_free((cobj_hashmap*)self->as.p);
}

static int entry_cmp(const void* a, const void* b){
    const entry* entry_a = *(const entry* const*)a;
    const entry* entry_b = *(const entry* const*)b;
    return cobj_cmp(&entry_a->key, &entry_b->key);
}

//The live entries sorted by key, so the order doesn't depend on insertion order
static const entry** sorted_entries(const cobj_hashmap* map){
    const entry** sorted = (const entry**)cobj_malloc((map->len > 0 ? map->len : 1) * sizeof(entry*));
    if(sorted == NULL) return NULL;

    size_t count = 0;
    for(size_t i = 0; i < map->used; i++){
        if(!cobj_is_empty(&map->entries[i].key)) sorted[count++] = &map->entries[i];
    }

    qsort(sorted, count, sizeof(entry*), entry_cmp);
    return sorted;
}

//By length, then entry by entry in key order (key first, then value)
static int hashmap_cmp(const cobj* a, const cobj* b){
    const cobj_hashmap* map_a = (const cobj_hashmap*)a->as.p;
    const cobj_hashmap* map_b = (const cobj_hashmap*)b->as.p;

    if(map_a->len != map_b->len) return CMP3(map_a->len, map_b->len);

    const entry** entries_a = sorted_entries(map_a);
    const entry** entries_b = sorted_entries(map_b);
    int result = 0;

    //Out of memory: fall back to identity, which is still consistent for equal maps
    if(entries_a == NULL || entries_b == NULL) result = CMP3((uintptr_t)map_a, (uintptr_t)map_b);

    for(size_t i = 0; entries_a != NULL && entries_b != NULL && i < map_a->len && result == 0; i++){
        result = cobj_cmp(&entries_a[i]->key, &entries_b[i]->key);
        if(result == 0) result = cobj_cmp(&entries_a[i]->value, &entries_b[i]->value);
    }

    cobj_free(entries_a);
    cobj_free(entries_b);
    return result;
}

//A sum, so it doesn't depend on the order of the entries
static size_t hashmap_hash(const cobj* self){
    const cobj_hashmap* map = (const cobj_hashmap*)self->as.p;
    size_t hash = map->len;

    for(size_t i = 0; i < map->used; i++){
        const entry* current = &map->entries[i];
        if(!cobj_is_empty(&current->key)) hash += cobj_hash_combine(current->hash, cobj_hash(&current->value));
    }
    return hash;
}

static cobj_status hashmap_fmt(const cobj* self, cobj_buf* out){
    const cobj_hashmap* map = (const cobj_hashmap*)self->as.p;
    cobj_status status = cobj_buf_puts(out, "{");

    size_t position = 0;
    const cobj* key;
    const cobj* value;
    for(int first = 1; status == COBJ_OK && cobj_hashmap_next(map, &position, &key, &value); first = 0){
        if(!first) status = cobj_buf_puts(out, ", ");
        if(status == COBJ_OK) status = cobj_fmt(key, out);
        if(status == COBJ_OK) status = cobj_buf_puts(out, ": ");
        if(status == COBJ_OK) status = cobj_fmt(value, out);
    }

    return status == COBJ_OK ? cobj_buf_puts(out, "}") : status;
}

static size_t hashmap_len(const cobj* self){
    return ((const cobj_hashmap*)self->as.p)->len;
}

//Iterating a map yields its keys
static bool hashmap_next(const cobj* self, cobj_iter* it, cobj* out){
    const cobj* key;
    if(!cobj_hashmap_next((const cobj_hashmap*)self->as.p, &it->index, &key, NULL)) return false;

    *out = *key;
    return true;
}

static bool hashmap_contains(const cobj* self, const cobj* key){
    return cobj_hashmap_contains((const cobj_hashmap*)self->as.p, key);
}

const cobj_type COBJ_HASHMAP = {.name = "hashmap", .clone = hashmap_clone, .drop = hashmap_drop,
                                .cmp = hashmap_cmp, .hash = hashmap_hash, .fmt = hashmap_fmt,
                                .len = hashmap_len, .next = hashmap_next, .contains = hashmap_contains};
