/*
 * Counts the words of a text: string functions, a map, iteration and
 * sorting working together.
 */

#include "c_hashmap/c_hashmap.h"
#include "c_string/c_string.h"

static const char* TEXT =
    "The cat sat on the mat.\n"
    "The dog sat on the log, and the cat watched the dog.";

int main(void){
    cobj words = cobj_list_value(cobj_string_split(TEXT, NULL));
    cobj counts = cobj_hashmap_value(cobj_hashmap_new(0));
    cobj_hashmap* map = cobj_hashmap_of(&counts);

    COBJ_FOREACH(word, &words){
        cobj bare = cobj_string_strip(cobj_as_cstr(&word), ".,");
        cobj key = cobj_string_lower(cobj_as_cstr(&bare));
        cobj_drop(&bare);

        const cobj* seen = cobj_hashmap_get(map, &key);
        int64_t count = seen != NULL ? seen->as.i : 0;
        cobj_hashmap_set(map, key, cobj_i64(count + 1));
    }

    //(count, word) tuples: cobj_cmp sorts them by count, then alphabetically
    cobj ranking = cobj_list_value(cobj_list_new());
    size_t position = 0;
    const cobj* word;
    const cobj* count;
    while(cobj_hashmap_next(map, &position, &word, &count)){
        cobj pair[2] = {cobj_clone(count), cobj_clone(word)};
        cobj_list_push(cobj_list_of(&ranking), cobj_tuple_value(cobj_tuple_new(2, pair)));
    }
    cobj_list_sort(cobj_list_of(&ranking));

    cobj_print(&counts);
    printf("\n");
    cobj_print(&ranking);
    printf("\n%zu words, %zu different\n", cobj_len(&words), cobj_len(&counts));

    cobj_drop(&words);
    cobj_drop(&counts);
    cobj_drop(&ranking);
    return 0;
}
