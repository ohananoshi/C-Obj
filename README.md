## C-Obj

### Python-like data structures for C, built on one pluggable value type

C-Obj gives C a single value type, `cobj`, that can hold a number, a string, a list, a map, a set, an n-d array or any type you define. Every type is described by a `cobj_type`: a small table of properties (clone, drop, compare, hash, format, and for containers length and iteration). The built-in types and the bundled data structures use that same table, so a new data structure plugs in by providing one, and from then on it works everywhere the others do: inside lists, as map keys, in comparisons when printed, and in any loop.

### Quick start

```C
#include "c_hashmap/c_hashmap.h"

int main(void){
    cobj stock = cobj_hashmap_make("fruits", cobj_list_make("banana", 3, 2.5));

    cobj_list* fruits = cobj_list_of(cobj_hashmap_get_str(cobj_hashmap_of(&stock), "fruits"));
    cobj_list_push(fruits, cobj_of("kiwi"));

    cobj_print(&stock); //{fruits: [banana, 3, 2.5, kiwi]}
    printf("\n");

    cobj_drop(&stock); //frees the map, the list and the strings
    return 0;
}
```

After `make install` (see Installing below):

```bash
cc main.c $(pkg-config --cflags --libs c_obj) -o main
```

### Values

A `cobj` is 16 bytes: a pointer to its type and a union holding the value. Numbers live inside the value, so they need no allocation. Strings, containers and most custom types point to a heap payload that the value owns. A value whose type is `NULL` is *empty*.

| Constructor | Type | Read it with |
|---|---|---|
| `cobj_i8`, `cobj_i16`, `cobj_i32`, `cobj_i64` | `COBJ_I8` … `COBJ_I64` | `v.as.i` |
| `cobj_u8`, `cobj_u16`, `cobj_u32`, `cobj_u64` | `COBJ_U8` … `COBJ_U64` | `v.as.u` |
| `cobj_f32`, `cobj_f64` | `COBJ_F32`, `COBJ_F64` | `v.as.f`, `v.as.d` |
| `cobj_ldouble` | `COBJ_LDOUBLE` | `*(long double*)cobj_unbox(&v, &COBJ_LDOUBLE)` |
| `cobj_str("text")` (copies the string) | `COBJ_STR` | `cobj_as_cstr(&v)` |
| `cobj_bool(b)` | `COBJ_BOOL` | `v.as.u` (0 or 1) |
| `cobj_ptr(p)` (borrowed, never freed) | `COBJ_PTR` | `v.as.p` |
| `cobj_empty()` | none | `cobj_is_empty(&v)` |

`cobj_of(x)` picks the constructor from the C type of its argument: `cobj_of(1)` is an i32, `cobj_of(2.5)` an f64, `cobj_of("text")` a str, `cobj_of(flag)` a bool when `flag` is a `bool` variable, and a `cobj` passes through. In C, `true` and `false` are `int` constants, so `cobj_of(true)` is an i32.

`cobj_is(&v, &COBJ_STR)` checks a value's type, and `cobj_type_name(&v)` gives its name.

### Ownership rules

1. A value owns its payload. `cobj_drop(&v)` frees it and leaves `v` empty; dropping an empty value does nothing.
2. `cobj_clone(&v)` returns a deep copy.
3. Functions that take a `cobj` by value **consume** it, even when they fail. `cobj_list_push(list, cobj_str("x"))` needs no cleanup. To keep your own copy, pass `cobj_clone(&v)`.
4. Functions that return `const cobj*` lend the value: it stays owned by the container.
5. A constructor that fails to allocate returns an empty value, and inserting an empty value fails with `COBJ_EINVAL`.

Functions that can fail return a `cobj_status` (`COBJ_OK`, `COBJ_ENOMEM`, `COBJ_EINVAL`, `COBJ_ETYPE`, `COBJ_ERANGE`, `COBJ_ENOTFOUND`, `COBJ_EIO`); `cobj_strerror` turns it into text. The library never prints errors.

Every allocation goes through `cobj_malloc` / `cobj_free`. To use your own allocator (an arena, a tracking allocator, an embedded heap), call `cobj_set_allocator` once at startup, before creating any value. Payloads you give to `cobj_box`, and strings from `cobj_to_string`, then come from and go back to `cobj_malloc` / `cobj_free`.

### Operations on any value

| Function | What it does |
|---|---|
| `cobj_clone`, `cobj_drop` | Deep copy, free |
| `cobj_cmp`, `cobj_equals` | Total order returning -1, 0 or 1 |
| `cobj_hash`, `cobj_hashable` | Hash that agrees with `cobj_cmp`; only hashable values can be map keys |
| `cobj_print`, `cobj_fprint`, `cobj_to_string`, `cobj_fmt` | Text form: `[1, 2.5]`, `(a, b)`, `{k: v}`, `{a, b}` |
| `cobj_len`, `cobj_contains`, `cobj_iterable` | Size and membership of any container (0 and false for other values) |
| `COBJ_FOREACH(item, &v)`, `cobj_iter_begin` / `cobj_iter_next` | Loop over any container |

Comparison rules: values of the same type use that type's `cmp`. Values of different types are never equal and are ordered by type name, with empty values first. For floats, NaN sorts after every number and equals NaN, and `-0.0` equals `0.0`.

### Iteration

Every container implements the same iteration protocol, so one loop works for all of them: lists, tuples and sets yield their elements, maps yield their keys, and arrays yield their elements in row-major order.

```C
COBJ_FOREACH(fruit, &fruits){
    cobj_print(&fruit);
    printf("\n");
}
```

Iterated elements are borrowed: don't drop them, and clone one to keep it. Changing a container while looping over it is undefined. Functions built on the protocol accept any iterable: `cobj_list_from`, `cobj_set_from`, `cobj_array_from` and `cobj_string_join`.

### Modules

| Module | Provides |
|---|---|
| [c_list](c_list/c_list.h) | `cobj_list`: a growable list (`push`, `get`, `set`, `pop`, `from`, `sort`) |
| [c_tuple](c_tuple/c_tuple.h) | `cobj_tuple`: an immutable fixed-length sequence |
| [c_hashmap](c_hashmap/c_hashmap.h) | `cobj_hashmap`: any hashable key to any value, in insertion order like a Python dict (`set`, `get`, `get_str`, `remove`, `contains`, `keys`, `values`, `items`) |
| [c_set](c_set/c_set.h) | `cobj_set`: distinct hashable values in insertion order (`add`, `remove`, `contains`, `union`, `intersection`, `difference`, `is_subset`) |
| [c_array](c_array/c_array.h) | `cobj_array`: a typed n-d array of numbers or plain structs, stored unboxed in row-major order (`get`/`set` with `COBJ_IDX(...)`, `fill`, `reshape`, `data`) |
| [c_string](c_string/c_string.h) | Python-style string functions that return string values: `format`, `split`, `join`, `slice`, `strip`, `replace`, `find`, `upper`, `lower`, file I/O |

Each header documents its functions.

```C
cobj parts = cobj_list_value(cobj_string_split("a,b,c", ","));
cobj line = cobj_string_join(&parts, " | ");                  //a | b | c

cobj_array* m = cobj_array_new(&COBJ_F64, 2, COBJ_IDX(2, 3));  //2 x 3 zeros
cobj_array_set(m, COBJ_IDX(1, 2), cobj_f64(4.5));
cobj matrix = cobj_array_value(m);                            //[[0.0, 0.0, 0.0], [0.0, 0.0, 4.5]]
```

[examples/word_count.c](examples/word_count.c) puts strings, a map, iteration and sorting together.

### Literals

Each container has a macro that builds it from 1 to 32 plain C values (or `cobj` values), passing each one through `cobj_of`:

```C
cobj l = cobj_list_make(1, "two", 3.5);             //[1, two, 3.5]
cobj t = cobj_tuple_make("x", 2);                   //(x, 2)
cobj s = cobj_set_make(3, 1, 3);                    //{3, 1}
cobj m = cobj_hashmap_make("one", 1, "two", 2);     //{one: 1, two: 2}
cobj a = cobj_array_make(&COBJ_F64, 1.0, 2.5);      //[1.0, 2.5]
cobj nested = cobj_list_make(1, cobj_list_make(2, 3));
```

`cobj_hashmap_make` takes keys and values alternately; an odd number of arguments doesn't compile. Array values must have the array's C type (`1.0`, not `1`, for `COBJ_F64`). When something fails, the macro gives an empty value and every item is dropped.

### Adding a data structure

Every module follows the same contract:

1. **One folder.** The module `<m>` lives in `c_<m>/c_<m>.h` and `c_<m>/c_<m>.c`, includes `../c_obj.h`, and uses only the public core API, so it can be added or removed without touching anything else. It may include other modules it builds on (c_hashmap uses c_list and c_tuple).
2. **Properties.** It exports a descriptor, `extern const cobj_type COBJ_<M>;`. Define it with designated initializers so fields added later default to `NULL`:

   | Field | Meaning | Default when `NULL` |
   |---|---|---|
   | `name` | Type name, used in messages and to order different types | required |
   | `size` | `0`: the value lives in `as`. `> 0`: `as.p` points to a payload of this size | `0` |
   | `clone(dst, src)` | Fill `dst->as` with a deep copy of `src` | bitwise copy, or `cobj_malloc` + `memcpy` of `size` bytes |
   | `drop(self)` | Release what the value owns | nothing, or `cobj_free(as.p)` when `size > 0` |
   | `cmp(a, b)` | Order two values of this type; any sign works | compare the raw bytes |
   | `hash(self)` | Must agree with `cmp` | none: the type can't be a map key |
   | `fmt(self, out)` | Append the text form with `cobj_buf_puts` / `cobj_buf_printf` | `<name>` |
   | `len(self)` | Containers: number of elements | `0`: not a container |
   | `next(self, it, out)` | Containers: write the next element (borrowed) to `out`, using `it->index` / `it->cursor` as you like; return false at the end | not iterable |
   | `contains(self, value)` | Containers: membership | scan with `next` |

3. **Constructors.** `cobj_<m>* cobj_<m>_new(...)` and `void cobj_<m>_free(cobj_<m>*)`.
4. **Value bridges.** `cobj cobj_<m>_value(cobj_<m>*)` moves the handle into a value, and `cobj_<m>* cobj_<m>_of(const cobj*)` borrows it back (NULL for other types). Both are one line: `return cobj_box(&COBJ_<M>, handle);` and `return cobj_unbox(value, &COBJ_<M>);`.
5. **Operations** are named `cobj_<m>_*` and follow the ownership rules above.
6. **Tests.** Add `tests/test_<m>.c`, add a few sample values to `make_samples()` in [tests/test_contract.c](tests/test_contract.c), and add `<m>` to `MODULES` in the [Makefile](Makefile). The contract test checks that clones are equal and independent, that `cmp` is a consistent total order, that equal values hash the same, that formatting works, and, for containers, that iteration yields `len` elements that are all `contains`-ed.

7. **Literal (optional).** A `cobj_<m>_make(...)` macro over a function that moves an array of values into a new container, built with `COBJ_ARG_COUNT` and `COBJ_MAP(cobj_of, ...)` (see `cobj_list_make` in [c_list/c_list.h](c_list/c_list.h)).

Allocate with `cobj_malloc` / `cobj_free`, never `malloc` / `free`, so custom allocators see every block. [tests/test_alloc.c](tests/test_alloc.c) runs every module under a counting allocator and fails each allocation in turn; add your module to its workload.

The bundled modules are the reference implementations: [c_list/c_list.c](c_list/c_list.c) is the simplest container, and [c_set/c_set.c](c_set/c_set.c) shows a module built on another one.

#### A plain struct type

A type whose payload is a plain struct only needs a descriptor with `size` set: the defaults copy and free the struct. Create values with `cobj_new(&TYPE, &my_struct)` and read them with `cobj_unbox(&v, &TYPE)`. This is [examples/person.c](examples/person.c), which `make test` builds and runs:

```C
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
```

Output:

```
[Ivan (32), Ze (26)]
Ivan is admin
```



### Version

This is version 0.3.0. `COBJ_VERSION_STRING` (and `COBJ_VERSION_MAJOR` / `_MINOR` / `_PATCH`) give the version of the headers, and `cobj_version()` gives the version of the compiled library.

### License

C-Obj is released under the [MIT License](LICENSE).
