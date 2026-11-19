#ifndef C_OBJ_H
#define C_OBJ_H

/**
 * @file c_obj.h
 * @brief C-Obj core: the cobj value, type descriptors and the operations every value supports.
 *
 * A cobj is a 16-byte value whose behavior comes from a type descriptor
 * (cobj_type). Built-in types and module types use the same descriptor, so a
 * new data structure plugs in by exporting one.
 *
 * Ownership rules:
 *  - A cobj owns its payload. cobj_drop() frees it and leaves the value empty.
 *  - cobj_clone() returns a deep copy.
 *  - Functions that take a cobj by value consume it, even when they fail.
 *  - Functions that return `const cobj*` lend the value: it stays owned by the container.
 *  - Constructors that fail to allocate return an empty value; inserting an
 *    empty value fails with #COBJ_EINVAL.
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/** @cond INTERNAL */
#if defined(__GNUC__)
#define COBJ_PRINTF_FMT(fmt_index, args_index) __attribute__((format(printf, fmt_index, args_index)))
#else
#define COBJ_PRINTF_FMT(fmt_index, args_index)
#endif
/** @endcond */

//======================================= VERSION =============================================

#define COBJ_VERSION_MAJOR 0          ///< Major version: incompatible API changes
#define COBJ_VERSION_MINOR 3          ///< Minor version: compatible additions
#define COBJ_VERSION_PATCH 0          ///< Patch version: fixes
#define COBJ_VERSION_STRING "0.3.0"   

/// The version of the compiled library; compare it with #COBJ_VERSION_STRING to catch a header/library mismatch
const char* cobj_version(void);

//======================================= ALLOCATION ==========================================

/**
 * The functions every allocation made by C-Obj and its modules goes through.
 *
 * Set a custom allocator before creating any value: memory must be freed by
 * the allocator that allocated it. Payloads given to cobj_box(), and strings
 * returned by cobj_to_string(), use cobj_malloc() / cobj_free(). Not
 * thread-safe: set it once, at startup.
 */
typedef struct cobj_allocator{
    void* (*malloc_fn)(size_t size);                ///< Like malloc
    void* (*realloc_fn)(void* ptr, size_t size);    ///< Like realloc
    void (*free_fn)(void* ptr);                     ///< Like free; never called with NULL
}cobj_allocator;

/// Routes every allocation through allocator. NULL (or a NULL field) restores malloc, realloc and free
void cobj_set_allocator(const cobj_allocator* allocator);

/// Allocates size bytes with the current allocator
void* cobj_malloc(size_t size);

/// Zeroed memory for count elements of size bytes; NULL when the total overflows
void* cobj_calloc(size_t count, size_t size);

/// Resizes a block from cobj_malloc() with the current allocator
void* cobj_realloc(void* ptr, size_t size);

/// Frees a block from cobj_malloc(); NULL is ignored
void cobj_free(void* ptr);

//======================================= STATUS ==============================================

/// The result of every function that can fail
typedef enum cobj_status{
    COBJ_OK = 0,      ///< Success
    COBJ_ENOMEM,      ///< Allocation failed
    COBJ_EINVAL,      ///< Invalid argument: NULL pointer or empty value
    COBJ_ETYPE,       ///< Value of the wrong type for the operation
    COBJ_ERANGE,      ///< Index out of range
    COBJ_ENOTFOUND,   ///< Key or element not found
    COBJ_EIO          ///< Reading or writing a file failed
}cobj_status;

/// A short English description of a status
const char* cobj_strerror(cobj_status status);

//======================================= TEXT BUFFER =========================================

/// Growable, always NUL-terminated text buffer used by the fmt callbacks
typedef struct cobj_buf{
    char* data;   ///< The text, NULL until something is written
    size_t len;   ///< Length of the text, without the NUL terminator
    size_t cap;   ///< Bytes allocated
}cobj_buf;

/// Initializer of an empty buffer: `cobj_buf buf = COBJ_BUF_INIT;`
#define COBJ_BUF_INIT {NULL, 0, 0}

/// Appends a NUL-terminated string
cobj_status cobj_buf_puts(cobj_buf* buf, const char* str);

/// Appends len chars of str
cobj_status cobj_buf_putn(cobj_buf* buf, const char* str, size_t len);

/// Appends printf-style formatted text
cobj_status cobj_buf_printf(cobj_buf* buf, const char* fmt, ...) COBJ_PRINTF_FMT(2, 3);

/// cobj_buf_printf() with a va_list
cobj_status cobj_buf_vprintf(cobj_buf* buf, const char* fmt, va_list args) COBJ_PRINTF_FMT(2, 0);

/// Frees the text and leaves the buffer empty
void cobj_buf_free(cobj_buf* buf);

//======================================= VALUES AND TYPES ====================================

struct cobj_type;

/// A value: a pointer to its type descriptor and the value itself (16 bytes on 64-bit)
typedef struct cobj{
    const struct cobj_type* type;   ///< The type descriptor; NULL means an empty value
    union{
        int64_t i;                  ///< Signed integers
        uint64_t u;                 ///< Unsigned integers and bool
        double d;                   ///< f64
        float f;                    ///< f32
        void* p;                    ///< Heap payload (strings, containers, module types)
    }as;                            ///< The value, read through the member that matches the type
}cobj;

/// Iteration state. A type's next callback may use index and cursor as it likes; both start at zero
typedef struct cobj_iter{
    const cobj* source;   ///< The value being iterated
    size_t index;         ///< Free for the type's next callback; starts at 0
    void* cursor;         ///< Free for the type's next callback; starts at NULL
}cobj_iter;

/**
 * Type descriptor: the properties every type provides.
 *
 * All callbacks are optional. Define descriptors with designated
 * initializers, so fields added in later versions default to NULL.
 */
typedef struct cobj_type{
    /// Type name, used in messages and to order values of different types
    const char* name;

    /// 0: the value lives in `as`. Greater than 0: as.p points to a heap payload of this size
    size_t size;

    /// Fills `dst->as` with a deep copy of src (`dst->type` is already set). Default: bitwise copy, or cobj_malloc() + memcpy when `size > 0`
    cobj_status (*clone)(cobj* dst, const cobj* src);

    /// Releases what the value owns. Default: nothing, or `cobj_free(as.p)` when `size > 0`
    void (*drop)(cobj* self);

    /// Orders two values of this type; any sign is accepted, cobj_cmp() normalizes it. Default: compares the raw bytes
    int (*cmp)(const cobj* a, const cobj* b);

    /// Must agree with cmp: equal values hash the same. Default: none, so the type can't be a map key
    size_t (*hash)(const cobj* self);

    /// Appends a text form of the value. Default: `"<name>"`
    cobj_status (*fmt)(const cobj* self, cobj_buf* out);

    /// Containers: number of elements. Default: 0, the type is not a container
    size_t (*len)(const cobj* self);

    /// Containers: writes the next element to *out and returns true, or returns false at the end.
    /// The element is borrowed: it stays owned by the container. Default: not iterable
    bool (*next)(const cobj* self, cobj_iter* it, cobj* out);

    /// Containers: membership. Default: scans the elements with next and cobj_equals()
    bool (*contains)(const cobj* self, const cobj* value);
}cobj_type;

//======================================= BUILT-IN TYPES ======================================

extern const cobj_type COBJ_I8;       ///< int8_t, in as.i
extern const cobj_type COBJ_I16;      ///< int16_t, in as.i
extern const cobj_type COBJ_I32;      ///< int32_t, in as.i
extern const cobj_type COBJ_I64;      ///< int64_t, in as.i
extern const cobj_type COBJ_U8;       ///< uint8_t, in as.u
extern const cobj_type COBJ_U16;      ///< uint16_t, in as.u
extern const cobj_type COBJ_U32;      ///< uint32_t, in as.u
extern const cobj_type COBJ_U64;      ///< uint64_t, in as.u
extern const cobj_type COBJ_F32;      ///< float, in as.f
extern const cobj_type COBJ_F64;      ///< double, in as.d
extern const cobj_type COBJ_LDOUBLE;  ///< long double, in a heap payload (as.p)
extern const cobj_type COBJ_STR;      ///< Owned, NUL-terminated char*, in as.p
extern const cobj_type COBJ_PTR;      ///< Borrowed void*, in as.p, never freed
extern const cobj_type COBJ_BOOL;     ///< 0 or 1 in as.u, printed as true/false

/// The empty value (its type is NULL)
static inline cobj cobj_empty(void){ cobj v = {NULL, {.i = 0}}; return v; }

/// An i8 value
static inline cobj cobj_i8(int8_t x){ cobj v = {&COBJ_I8, {.i = x}}; return v; }
/// An i16 value
static inline cobj cobj_i16(int16_t x){ cobj v = {&COBJ_I16, {.i = x}}; return v; }
/// An i32 value
static inline cobj cobj_i32(int32_t x){ cobj v = {&COBJ_I32, {.i = x}}; return v; }
/// An i64 value
static inline cobj cobj_i64(int64_t x){ cobj v = {&COBJ_I64, {.i = x}}; return v; }

/// A u8 value
static inline cobj cobj_u8(uint8_t x){ cobj v = {&COBJ_U8, {.u = x}}; return v; }
/// A u16 value
static inline cobj cobj_u16(uint16_t x){ cobj v = {&COBJ_U16, {.u = x}}; return v; }
/// A u32 value
static inline cobj cobj_u32(uint32_t x){ cobj v = {&COBJ_U32, {.u = x}}; return v; }
/// A u64 value
static inline cobj cobj_u64(uint64_t x){ cobj v = {&COBJ_U64, {.u = x}}; return v; }

/// An f32 value
static inline cobj cobj_f32(float x){ cobj v = {&COBJ_F32, {.i = 0}}; v.as.f = x; return v; }
/// An f64 value
static inline cobj cobj_f64(double x){ cobj v = {&COBJ_F64, {.d = x}}; return v; }

/// A borrowed pointer: the value never frees it
static inline cobj cobj_ptr(void* x){ cobj v = {&COBJ_PTR, {.p = x}}; return v; }

/// A bool value
static inline cobj cobj_bool(bool x){ cobj v = {&COBJ_BOOL, {.u = x ? 1u : 0u}}; return v; }

/// A copy of the string; empty value when str is NULL or allocation fails
cobj cobj_str(const char* str);

/// A long double value; empty value when allocation fails
cobj cobj_ldouble(long double x);

/** @cond INTERNAL */
//Used by cobj_of for arguments that already are values (they are moved)
static inline cobj cobj_of_value_(cobj value){ return value; }
/** @endcond */

/**
 * A value from any C number, string, pointer or cobj, picked by the argument's type.
 *
 * cobj_of(1) is an i32, cobj_of(2.5) an f64, cobj_of("text") a str (copied),
 * and a cobj passes through (moved). Other types don't compile. true and
 * false are ints in C, so cobj_of(true) is an i32: use a bool variable or
 * cobj_bool().
 */
#define cobj_of(x) _Generic((x),                                                  \
    _Bool: cobj_bool,                                                             \
    char: cobj_i8, signed char: cobj_i8, unsigned char: cobj_u8,                  \
    short: cobj_i16, unsigned short: cobj_u16,                                    \
    int: cobj_i32, unsigned int: cobj_u32,                                        \
    long: cobj_i64, unsigned long: cobj_u64,                                      \
    long long: cobj_i64, unsigned long long: cobj_u64,                            \
    float: cobj_f32, double: cobj_f64, long double: cobj_ldouble,                 \
    char*: cobj_str, const char*: cobj_str,                                       \
    void*: cobj_ptr,                                                              \
    cobj: cobj_of_value_)(x)

/// The number of arguments, from 1 to 32: COBJ_ARG_COUNT(a, b, c) is 3. Used by the literal macros
#define COBJ_ARG_COUNT(...) COBJ_ARG_COUNT_(__VA_ARGS__,                         \
    32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17,              \
    16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)

/// Applies f to each of 1 to 32 arguments: COBJ_MAP(f, a, b) is f(a), f(b). Used by the literal macros
#define COBJ_MAP(f, ...) COBJ_CAT(COBJ_MAP_, COBJ_ARG_COUNT(__VA_ARGS__))(f, __VA_ARGS__)

/** @cond INTERNAL */
#define COBJ_ARG_COUNT_(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13,  \
    _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28,  \
    _29, _30, _31, _32, count, ...) count

#define COBJ_CAT_(a, b) a##b
#define COBJ_CAT(a, b) COBJ_CAT_(a, b)
#define COBJ_MAP_1(f, x) f(x)
#define COBJ_MAP_2(f, x, ...) f(x), COBJ_MAP_1(f, __VA_ARGS__)
#define COBJ_MAP_3(f, x, ...) f(x), COBJ_MAP_2(f, __VA_ARGS__)
#define COBJ_MAP_4(f, x, ...) f(x), COBJ_MAP_3(f, __VA_ARGS__)
#define COBJ_MAP_5(f, x, ...) f(x), COBJ_MAP_4(f, __VA_ARGS__)
#define COBJ_MAP_6(f, x, ...) f(x), COBJ_MAP_5(f, __VA_ARGS__)
#define COBJ_MAP_7(f, x, ...) f(x), COBJ_MAP_6(f, __VA_ARGS__)
#define COBJ_MAP_8(f, x, ...) f(x), COBJ_MAP_7(f, __VA_ARGS__)
#define COBJ_MAP_9(f, x, ...) f(x), COBJ_MAP_8(f, __VA_ARGS__)
#define COBJ_MAP_10(f, x, ...) f(x), COBJ_MAP_9(f, __VA_ARGS__)
#define COBJ_MAP_11(f, x, ...) f(x), COBJ_MAP_10(f, __VA_ARGS__)
#define COBJ_MAP_12(f, x, ...) f(x), COBJ_MAP_11(f, __VA_ARGS__)
#define COBJ_MAP_13(f, x, ...) f(x), COBJ_MAP_12(f, __VA_ARGS__)
#define COBJ_MAP_14(f, x, ...) f(x), COBJ_MAP_13(f, __VA_ARGS__)
#define COBJ_MAP_15(f, x, ...) f(x), COBJ_MAP_14(f, __VA_ARGS__)
#define COBJ_MAP_16(f, x, ...) f(x), COBJ_MAP_15(f, __VA_ARGS__)
#define COBJ_MAP_17(f, x, ...) f(x), COBJ_MAP_16(f, __VA_ARGS__)
#define COBJ_MAP_18(f, x, ...) f(x), COBJ_MAP_17(f, __VA_ARGS__)
#define COBJ_MAP_19(f, x, ...) f(x), COBJ_MAP_18(f, __VA_ARGS__)
#define COBJ_MAP_20(f, x, ...) f(x), COBJ_MAP_19(f, __VA_ARGS__)
#define COBJ_MAP_21(f, x, ...) f(x), COBJ_MAP_20(f, __VA_ARGS__)
#define COBJ_MAP_22(f, x, ...) f(x), COBJ_MAP_21(f, __VA_ARGS__)
#define COBJ_MAP_23(f, x, ...) f(x), COBJ_MAP_22(f, __VA_ARGS__)
#define COBJ_MAP_24(f, x, ...) f(x), COBJ_MAP_23(f, __VA_ARGS__)
#define COBJ_MAP_25(f, x, ...) f(x), COBJ_MAP_24(f, __VA_ARGS__)
#define COBJ_MAP_26(f, x, ...) f(x), COBJ_MAP_25(f, __VA_ARGS__)
#define COBJ_MAP_27(f, x, ...) f(x), COBJ_MAP_26(f, __VA_ARGS__)
#define COBJ_MAP_28(f, x, ...) f(x), COBJ_MAP_27(f, __VA_ARGS__)
#define COBJ_MAP_29(f, x, ...) f(x), COBJ_MAP_28(f, __VA_ARGS__)
#define COBJ_MAP_30(f, x, ...) f(x), COBJ_MAP_29(f, __VA_ARGS__)
#define COBJ_MAP_31(f, x, ...) f(x), COBJ_MAP_30(f, __VA_ARGS__)
#define COBJ_MAP_32(f, x, ...) f(x), COBJ_MAP_31(f, __VA_ARGS__)
/** @endcond */

//======================================= OPERATIONS ==========================================

/// True when v has this type
static inline bool cobj_is(const cobj* v, const cobj_type* type){ return v != NULL && v->type == type; }

/// True for NULL and for the empty value
static inline bool cobj_is_empty(const cobj* v){ return v == NULL || v->type == NULL; }

/// The name of v's type; "empty" for an empty value
const char* cobj_type_name(const cobj* v);

/// The string of a #COBJ_STR value, NULL for any other type
const char* cobj_as_cstr(const cobj* v);

/// Deep copy; empty value when src is empty or allocation fails
cobj cobj_clone(const cobj* src);

/// Frees what the value owns and leaves it empty. Safe on empty values
void cobj_drop(cobj* v);

/**
 * Total order, returns -1, 0 or 1.
 *
 * Values of different types are never equal: they are ordered by type name
 * (empty values first). Floats: NaN sorts after every number and equals NaN;
 * -0.0 equals 0.0.
 */
int cobj_cmp(const cobj* a, const cobj* b);

/// cobj_cmp() == 0
static inline bool cobj_equals(const cobj* a, const cobj* b){ return cobj_cmp(a, b) == 0; }

/// True when the type provides a hash, so the value can be a map key
bool cobj_hashable(const cobj* v);

/// A hash that agrees with cobj_cmp(); 0 for values that are not hashable
size_t cobj_hash(const cobj* v);

/// Appends the text form of v to out
cobj_status cobj_fmt(const cobj* v, cobj_buf* out);

/// Writes the text form of v to a stream
cobj_status cobj_fprint(const cobj* v, FILE* out);

/// Writes the text form of v to stdout
cobj_status cobj_print(const cobj* v);

/// The text form in a new string (free it with cobj_free()); NULL when allocation fails
char* cobj_to_string(const cobj* v);

//======================================= ITERATION ===========================================

/// Starts iterating over any value; iteration ends at once when it isn't iterable
cobj_iter cobj_iter_begin(const cobj* container);

/// Writes the next element (borrowed) to *out, or returns false at the end
bool cobj_iter_next(cobj_iter* it, cobj* out);

/// True when v's type implements next
bool cobj_iterable(const cobj* v);

/// Number of elements, 0 for values that are not containers
size_t cobj_len(const cobj* v);

/// True when value is an element (or, for maps, a key) of container
bool cobj_contains(const cobj* container, const cobj* value);

/**
 * Loops over the elements of any container.
 *
 * Elements are borrowed: don't drop them, clone one to keep it. Changing a
 * container while iterating over it is undefined. break works as usual.
 * @code
 * COBJ_FOREACH(item, &fruits){
 *     cobj_print(&item);
 * }
 * @endcode
 */
#define COBJ_FOREACH(item, container)                                                        \
    for(cobj_iter item##_iter_ = cobj_iter_begin(container); item##_iter_.source != NULL;   \
        item##_iter_.source = NULL)                                                          \
        for(cobj item; cobj_iter_next(&item##_iter_, &item);)

//======================================= MODULE HELPERS ======================================

/// Wraps a heap payload from cobj_malloc(); the value takes ownership. Empty value when payload is NULL
cobj cobj_box(const cobj_type* type, void* payload);

/// The payload of a value of this type (borrowed), NULL for any other type
void* cobj_unbox(const cobj* v, const cobj_type* type);

/// Copies `type->size` bytes from payload into a new value. Only for types with `size > 0`
cobj cobj_new(const cobj_type* type, const void* payload);

/// A well-mixed hash of len bytes, for hash callbacks
size_t cobj_hash_bytes(const void* data, size_t len);

/// Combines a hash into seed, for hash callbacks of containers
size_t cobj_hash_combine(size_t seed, size_t value);

#endif //C_OBJ_H
