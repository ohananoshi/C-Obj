#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "c_obj.h"

#define CMP3(a, b) (((a) > (b)) - ((a) < (b)))

const char* cobj_version(void){
    return COBJ_VERSION_STRING;
}

//======================================= ALLOCATION ==========================================

static cobj_allocator allocator = {malloc, realloc, free};

void cobj_set_allocator(const cobj_allocator* custom){
    allocator.malloc_fn = custom != NULL && custom->malloc_fn != NULL ? custom->malloc_fn : malloc;
    allocator.realloc_fn = custom != NULL && custom->realloc_fn != NULL ? custom->realloc_fn : realloc;
    allocator.free_fn = custom != NULL && custom->free_fn != NULL ? custom->free_fn : free;
}

void* cobj_malloc(size_t size){
    return allocator.malloc_fn(size);
}

void* cobj_calloc(size_t count, size_t size){
    if(size != 0 && count > SIZE_MAX / size) return NULL;

    void* memory = allocator.malloc_fn(count * size);
    if(memory != NULL) memset(memory, 0, count * size);
    return memory;
}

void* cobj_realloc(void* ptr, size_t size){
    return allocator.realloc_fn(ptr, size);
}

void cobj_free(void* ptr){
    if(ptr != NULL) allocator.free_fn(ptr);
}

//======================================= STATUS ==============================================

const char* cobj_strerror(cobj_status status){
    switch(status){
        case COBJ_OK:        return "ok";
        case COBJ_ENOMEM:    return "out of memory";
        case COBJ_EINVAL:    return "invalid argument";
        case COBJ_ETYPE:     return "wrong type";
        case COBJ_ERANGE:    return "index out of range";
        case COBJ_ENOTFOUND: return "not found";
        case COBJ_EIO:       return "input/output error";
    }
    return "unknown status";
}

//======================================= TEXT BUFFER =========================================

//Makes room for extra chars plus the NUL terminator
static cobj_status buf_reserve(cobj_buf* buf, size_t extra){
    size_t need = buf->len + extra + 1;
    if(need <= buf->cap) return COBJ_OK;

    size_t cap = buf->cap > 0 ? buf->cap * 2 : 32;
    while(cap < need) cap *= 2;

    char* grown = (char*)cobj_realloc(buf->data, cap);
    if(grown == NULL) return COBJ_ENOMEM;

    buf->data = grown;
    buf->cap = cap;
    return COBJ_OK;
}

cobj_status cobj_buf_putn(cobj_buf* buf, const char* str, size_t len){
    if(buf == NULL || (str == NULL && len > 0)) return COBJ_EINVAL;
    if(buf_reserve(buf, len) != COBJ_OK) return COBJ_ENOMEM;

    if(len > 0) memcpy(buf->data + buf->len, str, len);
    buf->len += len;
    buf->data[buf->len] = '\0';
    return COBJ_OK;
}

cobj_status cobj_buf_puts(cobj_buf* buf, const char* str){
    if(str == NULL) return COBJ_EINVAL;
    return cobj_buf_putn(buf, str, strlen(str));
}

cobj_status cobj_buf_vprintf(cobj_buf* buf, const char* fmt, va_list args){
    if(buf == NULL || fmt == NULL) return COBJ_EINVAL;

    va_list measure;
    va_copy(measure, args);
    int len = vsnprintf(NULL, 0, fmt, measure);
    va_end(measure);
    if(len < 0) return COBJ_EINVAL;

    cobj_status status = buf_reserve(buf, (size_t)len);
    if(status != COBJ_OK) return status;

    vsnprintf(buf->data + buf->len, buf->cap - buf->len, fmt, args);
    buf->len += (size_t)len;
    return COBJ_OK;
}

cobj_status cobj_buf_printf(cobj_buf* buf, const char* fmt, ...){
    va_list args;
    va_start(args, fmt);
    cobj_status status = cobj_buf_vprintf(buf, fmt, args);
    va_end(args);
    return status;
}

void cobj_buf_free(cobj_buf* buf){
    if(buf == NULL) return;

    cobj_free(buf->data);
    buf->data = NULL;
    buf->len = 0;
    buf->cap = 0;
}

//======================================= HASHING =============================================

//splitmix64 finalizer: spreads every input bit over the whole result
static uint64_t mix64(uint64_t x){
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

//djb2 over the bytes, then mixed
size_t cobj_hash_bytes(const void* data, size_t len){
    const unsigned char* bytes = (const unsigned char*)data;
    uint64_t hash = 5381;

    for(size_t i = 0; i < len; i++) hash = ((hash << 5) + hash) + bytes[i];

    return (size_t)mix64(hash);
}

size_t cobj_hash_combine(size_t seed, size_t value){
    return seed ^ (value + (size_t)0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2));
}

//======================================= BUILT-IN TYPES ======================================

/* -------- INTEGERS: stored in as.i (signed) or as.u (unsigned) -------- */

static int int_cmp(const cobj* a, const cobj* b){ return CMP3(a->as.i, b->as.i); }

static size_t int_hash(const cobj* self){ return (size_t)mix64((uint64_t)self->as.i); }

static cobj_status int_fmt(const cobj* self, cobj_buf* out){ return cobj_buf_printf(out, "%" PRId64, self->as.i); }

static int uint_cmp(const cobj* a, const cobj* b){ return CMP3(a->as.u, b->as.u); }

static size_t uint_hash(const cobj* self){ return (size_t)mix64(self->as.u); }

static cobj_status uint_fmt(const cobj* self, cobj_buf* out){ return cobj_buf_printf(out, "%" PRIu64, self->as.u); }

/* -------- FLOATING: NaN sorts last and equals NaN, -0.0 equals 0.0 -------- */

static int double_order(double a, double b){
    int nan_a = isnan(a) != 0, nan_b = isnan(b) != 0;
    if(nan_a || nan_b) return CMP3(nan_a, nan_b);
    return CMP3(a, b);
}

static int ldouble_order(long double a, long double b){
    int nan_a = isnan(a) != 0, nan_b = isnan(b) != 0;
    if(nan_a || nan_b) return CMP3(nan_a, nan_b);
    return CMP3(a, b);
}

static size_t double_hash(double d){
    if(isnan(d)) return (size_t)mix64(0x7ff8000000000000ULL);
    if(d == 0.0) d = 0.0;   //folds -0.0 into 0.0

    uint64_t bits;
    memcpy(&bits, &d, sizeof(bits));
    return (size_t)mix64(bits);
}

//Appends ".0" to integral-looking text so floats read as floats (2.0, not 2)
static cobj_status put_float_text(cobj_buf* out, char* text, size_t size){
    if(strpbrk(text, ".eEnN") == NULL && strlen(text) + 2 < size) strcat(text, ".0");
    return cobj_buf_puts(out, text);
}

//Each format loop uses the fewest digits that read back as the same value

static cobj_status f32_fmt(const cobj* self, cobj_buf* out){
    char text[64];
    for(int precision = 1; precision <= FLT_DECIMAL_DIG; precision++){
        snprintf(text, sizeof(text), "%.*g", precision, (double)self->as.f);
        if(strtof(text, NULL) == self->as.f) break;
    }
    return put_float_text(out, text, sizeof(text));
}

static cobj_status f64_fmt(const cobj* self, cobj_buf* out){
    char text[64];
    for(int precision = 1; precision <= DBL_DECIMAL_DIG; precision++){
        snprintf(text, sizeof(text), "%.*g", precision, self->as.d);
        if(strtod(text, NULL) == self->as.d) break;
    }
    return put_float_text(out, text, sizeof(text));
}

static cobj_status ldouble_fmt(const cobj* self, cobj_buf* out){
    long double x = *(const long double*)self->as.p;
    char text[64];
    for(int precision = 1; precision <= LDBL_DECIMAL_DIG; precision++){
        snprintf(text, sizeof(text), "%.*Lg", precision, x);
        if(strtold(text, NULL) == x) break;
    }
    return put_float_text(out, text, sizeof(text));
}

static int f32_cmp(const cobj* a, const cobj* b){ return double_order(a->as.f, b->as.f); }

static size_t f32_hash(const cobj* self){ return double_hash(self->as.f); }

static int f64_cmp(const cobj* a, const cobj* b){ return double_order(a->as.d, b->as.d); }

static size_t f64_hash(const cobj* self){ return double_hash(self->as.d); }

static int ldouble_cmp(const cobj* a, const cobj* b){
    return ldouble_order(*(const long double*)a->as.p, *(const long double*)b->as.p);
}

static size_t ldouble_hash(const cobj* self){ return double_hash((double)*(const long double*)self->as.p); }

/* -------- STRINGS: as.p owns a NUL-terminated copy -------- */

static cobj_status str_clone(cobj* dst, const cobj* src){
    size_t len = strlen((const char*)src->as.p) + 1;
    char* copy = (char*)cobj_malloc(len);
    if(copy == NULL) return COBJ_ENOMEM;

    memcpy(copy, src->as.p, len);
    dst->as.p = copy;
    return COBJ_OK;
}

static void str_drop(cobj* self){ cobj_free(self->as.p); }

static int str_cmp(const cobj* a, const cobj* b){ return strcmp((const char*)a->as.p, (const char*)b->as.p); }

static size_t str_hash(const cobj* self){ return cobj_hash_bytes(self->as.p, strlen((const char*)self->as.p)); }

static cobj_status str_fmt(const cobj* self, cobj_buf* out){ return cobj_buf_puts(out, (const char*)self->as.p); }

/* -------- BOOL: 0 or 1 in as.u, false sorts first -------- */

static cobj_status bool_fmt(const cobj* self, cobj_buf* out){ return cobj_buf_puts(out, self->as.u ? "true" : "false"); }

/* -------- POINTERS: borrowed, compared by address -------- */

static int ptr_cmp(const cobj* a, const cobj* b){ return CMP3((uintptr_t)a->as.p, (uintptr_t)b->as.p); }

static size_t ptr_hash(const cobj* self){ return (size_t)mix64((uint64_t)(uintptr_t)self->as.p); }

static cobj_status ptr_fmt(const cobj* self, cobj_buf* out){ return cobj_buf_printf(out, "%p", self->as.p); }

const cobj_type COBJ_I8  = {.name = "i8",  .cmp = int_cmp, .hash = int_hash, .fmt = int_fmt};
const cobj_type COBJ_I16 = {.name = "i16", .cmp = int_cmp, .hash = int_hash, .fmt = int_fmt};
const cobj_type COBJ_I32 = {.name = "i32", .cmp = int_cmp, .hash = int_hash, .fmt = int_fmt};
const cobj_type COBJ_I64 = {.name = "i64", .cmp = int_cmp, .hash = int_hash, .fmt = int_fmt};

const cobj_type COBJ_U8  = {.name = "u8",  .cmp = uint_cmp, .hash = uint_hash, .fmt = uint_fmt};
const cobj_type COBJ_U16 = {.name = "u16", .cmp = uint_cmp, .hash = uint_hash, .fmt = uint_fmt};
const cobj_type COBJ_U32 = {.name = "u32", .cmp = uint_cmp, .hash = uint_hash, .fmt = uint_fmt};
const cobj_type COBJ_U64 = {.name = "u64", .cmp = uint_cmp, .hash = uint_hash, .fmt = uint_fmt};

const cobj_type COBJ_F32 = {.name = "f32", .cmp = f32_cmp, .hash = f32_hash, .fmt = f32_fmt};
const cobj_type COBJ_F64 = {.name = "f64", .cmp = f64_cmp, .hash = f64_hash, .fmt = f64_fmt};

//long double doesn't fit in the 8-byte union, so it uses a heap payload with the default clone/drop
const cobj_type COBJ_LDOUBLE = {.name = "ldouble", .size = sizeof(long double),
                                .cmp = ldouble_cmp, .hash = ldouble_hash, .fmt = ldouble_fmt};

const cobj_type COBJ_STR = {.name = "str", .clone = str_clone, .drop = str_drop,
                            .cmp = str_cmp, .hash = str_hash, .fmt = str_fmt};

const cobj_type COBJ_PTR = {.name = "ptr", .cmp = ptr_cmp, .hash = ptr_hash, .fmt = ptr_fmt};

const cobj_type COBJ_BOOL = {.name = "bool", .cmp = uint_cmp, .hash = uint_hash, .fmt = bool_fmt};

cobj cobj_str(const char* str){
    if(str == NULL) return cobj_empty();

    cobj borrowed = {&COBJ_STR, {.p = (void*)str}};
    return cobj_clone(&borrowed);
}

cobj cobj_ldouble(long double x){
    return cobj_new(&COBJ_LDOUBLE, &x);
}

//======================================= OPERATIONS ==========================================

const char* cobj_type_name(const cobj* v){
    return cobj_is_empty(v) ? "empty" : v->type->name;
}

const char* cobj_as_cstr(const cobj* v){
    return cobj_is(v, &COBJ_STR) ? (const char*)v->as.p : NULL;
}

cobj cobj_clone(const cobj* src){
    if(cobj_is_empty(src)) return cobj_empty();

    const cobj_type* type = src->type;
    cobj dst = *src;

    if(type->clone != NULL){
        if(type->clone(&dst, src) != COBJ_OK) return cobj_empty();
    }else if(type->size > 0){
        dst.as.p = cobj_malloc(type->size);
        if(dst.as.p == NULL) return cobj_empty();
        memcpy(dst.as.p, src->as.p, type->size);
    }

    return dst;
}

void cobj_drop(cobj* v){
    if(cobj_is_empty(v)) return;

    if(v->type->drop != NULL) v->type->drop(v);
    else if(v->type->size > 0) cobj_free(v->as.p);

    *v = cobj_empty();
}

int cobj_cmp(const cobj* a, const cobj* b){
    const cobj_type* type_a = cobj_is_empty(a) ? NULL : a->type;
    const cobj_type* type_b = cobj_is_empty(b) ? NULL : b->type;

    if(type_a != type_b){
        if(type_a == NULL) return -1;
        if(type_b == NULL) return 1;

        int by_name = strcmp(type_a->name, type_b->name);
        if(by_name != 0) return CMP3(by_name, 0);
        return CMP3((uintptr_t)type_a, (uintptr_t)type_b);
    }

    if(type_a == NULL) return 0;
    if(type_a->cmp != NULL) return CMP3(type_a->cmp(a, b), 0);

    //Default order: the raw payload bytes, or the raw 64-bit value for inline types
    if(type_a->size > 0) return CMP3(memcmp(a->as.p, b->as.p, type_a->size), 0);
    return CMP3(a->as.u, b->as.u);
}

bool cobj_hashable(const cobj* v){
    return !cobj_is_empty(v) && v->type->hash != NULL;
}

size_t cobj_hash(const cobj* v){
    return cobj_hashable(v) ? v->type->hash(v) : 0;
}

cobj_status cobj_fmt(const cobj* v, cobj_buf* out){
    if(out == NULL) return COBJ_EINVAL;
    if(cobj_is_empty(v)) return cobj_buf_puts(out, "<empty>");
    if(v->type->fmt != NULL) return v->type->fmt(v, out);

    return cobj_buf_printf(out, "<%s>", v->type->name);
}

cobj_status cobj_fprint(const cobj* v, FILE* out){
    if(out == NULL) return COBJ_EINVAL;

    cobj_buf buf = COBJ_BUF_INIT;
    cobj_status status = cobj_fmt(v, &buf);
    if(status == COBJ_OK && buf.len > 0) fwrite(buf.data, 1, buf.len, out);

    cobj_buf_free(&buf);
    return status;
}

cobj_status cobj_print(const cobj* v){
    return cobj_fprint(v, stdout);
}

char* cobj_to_string(const cobj* v){
    cobj_buf buf = COBJ_BUF_INIT;

    if(cobj_fmt(v, &buf) != COBJ_OK){
        cobj_buf_free(&buf);
        return NULL;
    }

    //A fmt callback may append nothing at all
    if(buf.data == NULL) return (char*)cobj_calloc(1, 1);
    return buf.data;
}

//======================================= ITERATION ===========================================

cobj_iter cobj_iter_begin(const cobj* container){
    cobj_iter it = {container, 0, NULL};
    return it;
}

bool cobj_iter_next(cobj_iter* it, cobj* out){
    if(out != NULL) *out = cobj_empty();
    if(it == NULL || out == NULL || !cobj_iterable(it->source)) return false;

    return it->source->type->next(it->source, it, out);
}

bool cobj_iterable(const cobj* v){
    return !cobj_is_empty(v) && v->type->next != NULL;
}

size_t cobj_len(const cobj* v){
    return !cobj_is_empty(v) && v->type->len != NULL ? v->type->len(v) : 0;
}

bool cobj_contains(const cobj* container, const cobj* value){
    if(cobj_is_empty(container)) return false;
    if(container->type->contains != NULL) return container->type->contains(container, value);

    COBJ_FOREACH(item, container){
        if(cobj_equals(&item, value)) return true;
    }
    return false;
}

//======================================= MODULE HELPERS ======================================

cobj cobj_box(const cobj_type* type, void* payload){
    if(type == NULL || payload == NULL) return cobj_empty();

    cobj v = {type, {.p = payload}};
    return v;
}

void* cobj_unbox(const cobj* v, const cobj_type* type){
    return cobj_is(v, type) ? v->as.p : NULL;
}

cobj cobj_new(const cobj_type* type, const void* payload){
    if(type == NULL || type->size == 0 || payload == NULL) return cobj_empty();

    void* copy = cobj_malloc(type->size);
    if(copy == NULL) return cobj_empty();

    memcpy(copy, payload, type->size);
    return cobj_box(type, copy);
}
