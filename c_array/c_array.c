#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "c_array.h"

#define CMP3(a, b) (((a) > (b)) - ((a) < (b)))

struct cobj_array{
    const cobj_type* dtype;
    size_t width;          //bytes per element
    size_t ndim;
    size_t len;            //total number of elements
    size_t* shape;
    unsigned char* data;
};

//Numeric and bool dtypes: descriptor, C type, field of cobj.as, constructor
#define NUMERIC_TYPES(X)                                                         \
    X(COBJ_I8, int8_t, i, cobj_i8)     X(COBJ_I16, int16_t, i, cobj_i16)        \
    X(COBJ_I32, int32_t, i, cobj_i32)  X(COBJ_I64, int64_t, i, cobj_i64)        \
    X(COBJ_U8, uint8_t, u, cobj_u8)    X(COBJ_U16, uint16_t, u, cobj_u16)       \
    X(COBJ_U32, uint32_t, u, cobj_u32) X(COBJ_U64, uint64_t, u, cobj_u64)       \
    X(COBJ_F32, float, f, cobj_f32)    X(COBJ_F64, double, d, cobj_f64)        \
    X(COBJ_BOOL, bool, u, cobj_bool)

//Bytes per element, 0 for types that can't be stored unboxed
static size_t element_width(const cobj_type* dtype){
#define X(type, ctype, field, make) if(dtype == &type) return sizeof(ctype);
    NUMERIC_TYPES(X)
#undef X

    //Plain data: copying the bytes is a complete copy
    if(dtype != NULL && dtype->size > 0 && dtype->clone == NULL && dtype->drop == NULL) return dtype->size;
    return 0;
}

//The element at slot as a value: numbers are copied into it, plain-data elements are borrowed from the storage
static cobj load(const cobj_array* array, const unsigned char* slot){
#define X(type, ctype, field, make)                                               \
    if(array->dtype == &type){                                                    \
        ctype x;                                                                  \
        memcpy(&x, slot, sizeof(x));                                              \
        return make(x);                                                           \
    }
    NUMERIC_TYPES(X)
#undef X

    cobj view = {array->dtype, {.p = (void*)slot}};
    return view;
}

//Writes a value of the array's dtype into slot
static void store(cobj_array* array, unsigned char* slot, const cobj* value){
#define X(type, ctype, field, make)                                               \
    if(array->dtype == &type){                                                    \
        ctype x = (ctype)value->as.field;                                         \
        memcpy(slot, &x, sizeof(x));                                              \
        return;                                                                   \
    }
    NUMERIC_TYPES(X)
#undef X

    memcpy(slot, value->as.p, array->width);
}

static unsigned char* slot_at(const cobj_array* array, size_t position){
    return array->data + position * array->width;
}

//Row-major position of an n-d index (the last index varies fastest). False when out of range
static bool position_of(const cobj_array* array, const size_t* index, size_t* position){
    size_t result = 0;

    for(size_t d = 0; d < array->ndim; d++){
        if(index[d] >= array->shape[d]) return false;
        result = result * array->shape[d] + index[d];
    }

    *position = result;
    return true;
}

//Total number of elements of a shape, false when it overflows with elements of this width
static bool total_of(size_t ndim, const size_t* shape, size_t width, size_t* total){
    size_t result = 1;

    for(size_t d = 0; d < ndim; d++){
        if(shape[d] != 0 && result > SIZE_MAX / width / shape[d]) return false;
        result *= shape[d];
    }

    *total = result;
    return true;
}

cobj_array* cobj_array_new(const cobj_type* dtype, size_t ndim, const size_t* shape){
    size_t width = element_width(dtype);
    size_t len = 0;
    if(width == 0 || ndim == 0 || shape == NULL || !total_of(ndim, shape, width, &len)) return NULL;

    cobj_array* array = (cobj_array*)cobj_calloc(1, sizeof(cobj_array));
    size_t* dims = (size_t*)cobj_malloc(ndim * sizeof(size_t));
    unsigned char* data = (unsigned char*)cobj_calloc(len > 0 ? len : 1, width);
    if(array == NULL || dims == NULL || data == NULL){
        cobj_free(array);
        cobj_free(dims);
        cobj_free(data);
        return NULL;
    }

    memcpy(dims, shape, ndim * sizeof(size_t));
    array->dtype = dtype;
    array->width = width;
    array->ndim = ndim;
    array->len = len;
    array->shape = dims;
    array->data = data;
    return array;
}

void cobj_array_free(cobj_array* array){
    if(array == NULL) return;

    cobj_free(array->data);
    cobj_free(array->shape);
    cobj_free(array);
}

const cobj_type* cobj_array_dtype(const cobj_array* array){
    return array != NULL ? array->dtype : NULL;
}

size_t cobj_array_ndim(const cobj_array* array){
    return array != NULL ? array->ndim : 0;
}

const size_t* cobj_array_shape(const cobj_array* array){
    return array != NULL ? array->shape : NULL;
}

size_t cobj_array_len(const cobj_array* array){
    return array != NULL ? array->len : 0;
}

cobj_status cobj_array_get_flat(const cobj_array* array, size_t position, cobj* out){
    if(out != NULL) *out = cobj_empty();
    if(array == NULL || out == NULL) return COBJ_EINVAL;
    if(position >= array->len) return COBJ_ERANGE;

    cobj element = load(array, slot_at(array, position));
    *out = cobj_clone(&element);
    return cobj_is_empty(out) ? COBJ_ENOMEM : COBJ_OK;
}

//Checks that value can be stored in the array; drops it otherwise
static cobj_status check_value(const cobj_array* array, cobj* value){
    cobj_status status = COBJ_OK;
    if(array == NULL || cobj_is_empty(value)) status = COBJ_EINVAL;
    else if(value->type != array->dtype) status = COBJ_ETYPE;

    if(status != COBJ_OK) cobj_drop(value);
    return status;
}

cobj_status cobj_array_set_flat(cobj_array* array, size_t position, cobj value){
    cobj_status status = check_value(array, &value);
    if(status != COBJ_OK) return status;

    if(position >= array->len){
        cobj_drop(&value);
        return COBJ_ERANGE;
    }

    store(array, slot_at(array, position), &value);
    cobj_drop(&value);
    return COBJ_OK;
}

cobj_status cobj_array_get(const cobj_array* array, const size_t* index, cobj* out){
    if(out != NULL) *out = cobj_empty();
    if(array == NULL || index == NULL || out == NULL) return COBJ_EINVAL;

    size_t position;
    if(!position_of(array, index, &position)) return COBJ_ERANGE;
    return cobj_array_get_flat(array, position, out);
}

cobj_status cobj_array_set(cobj_array* array, const size_t* index, cobj value){
    size_t position;

    if(array == NULL || index == NULL){
        cobj_drop(&value);
        return COBJ_EINVAL;
    }

    if(!position_of(array, index, &position)){
        cobj_drop(&value);
        return COBJ_ERANGE;
    }
    return cobj_array_set_flat(array, position, value);
}

cobj_status cobj_array_fill(cobj_array* array, cobj value){
    cobj_status status = check_value(array, &value);
    if(status != COBJ_OK) return status;

    for(size_t i = 0; i < array->len; i++) store(array, slot_at(array, i), &value);

    cobj_drop(&value);
    return COBJ_OK;
}

cobj_status cobj_array_reshape(cobj_array* array, size_t ndim, const size_t* shape){
    size_t len = 0;
    if(array == NULL || ndim == 0 || shape == NULL) return COBJ_EINVAL;
    if(!total_of(ndim, shape, array->width, &len) || len != array->len) return COBJ_EINVAL;

    size_t* dims = (size_t*)cobj_malloc(ndim * sizeof(size_t));
    if(dims == NULL) return COBJ_ENOMEM;

    memcpy(dims, shape, ndim * sizeof(size_t));
    cobj_free(array->shape);
    array->shape = dims;
    array->ndim = ndim;
    return COBJ_OK;
}

void* cobj_array_data(cobj_array* array){
    return array != NULL ? array->data : NULL;
}

cobj_array* cobj_array_from_values(const cobj_type* dtype, size_t count, cobj* items){
    if(count > 0 && items == NULL) return NULL;

    bool valid = true;
    for(size_t i = 0; i < count; i++){
        if(items[i].type != dtype) valid = false;
    }

    cobj_array* array = valid ? cobj_array_new(dtype, 1, &count) : NULL;
    for(size_t i = 0; i < count; i++){
        if(array != NULL) store(array, slot_at(array, i), &items[i]);
        cobj_drop(&items[i]);
    }
    return array;
}

cobj_array* cobj_array_from(const cobj_type* dtype, const cobj* iterable){
    if(!cobj_iterable(iterable)) return NULL;

    size_t count = 0;
    COBJ_FOREACH(item, iterable){
        if(item.type != dtype) return NULL;
        count++;
    }

    cobj_array* array = cobj_array_new(dtype, 1, &count);
    if(array == NULL) return NULL;

    size_t position = 0;
    COBJ_FOREACH(item, iterable) store(array, slot_at(array, position++), &item);
    return array;
}

cobj cobj_array_value(cobj_array* array){
    return cobj_box(&COBJ_ARRAY, array);
}

cobj_array* cobj_array_of(const cobj* value){
    return (cobj_array*)cobj_unbox(value, &COBJ_ARRAY);
}

//======================================= PROPERTIES ==========================================

static cobj_status array_clone(cobj* dst, const cobj* src){
    const cobj_array* source = (const cobj_array*)src->as.p;
    cobj_array* copy = cobj_array_new(source->dtype, source->ndim, source->shape);
    if(copy == NULL) return COBJ_ENOMEM;

    if(source->len > 0) memcpy(copy->data, source->data, source->len * source->width);
    dst->as.p = copy;
    return COBJ_OK;
}

static void array_drop(cobj* self){
    cobj_array_free((cobj_array*)self->as.p);
}

//By element type, then number of dimensions, then shape, then elements in row-major order
static int array_cmp(const cobj* a, const cobj* b){
    const cobj_array* array_a = (const cobj_array*)a->as.p;
    const cobj_array* array_b = (const cobj_array*)b->as.p;

    if(array_a->dtype != array_b->dtype){
        int by_name = strcmp(array_a->dtype->name, array_b->dtype->name);
        return by_name != 0 ? by_name : CMP3((uintptr_t)array_a->dtype, (uintptr_t)array_b->dtype);
    }

    if(array_a->ndim != array_b->ndim) return CMP3(array_a->ndim, array_b->ndim);
    for(size_t d = 0; d < array_a->ndim; d++){
        if(array_a->shape[d] != array_b->shape[d]) return CMP3(array_a->shape[d], array_b->shape[d]);
    }

    for(size_t i = 0; i < array_a->len; i++){
        cobj element_a = load(array_a, slot_at(array_a, i));
        cobj element_b = load(array_b, slot_at(array_b, i));
        int result = cobj_cmp(&element_a, &element_b);
        if(result != 0) return result;
    }
    return 0;
}

static size_t array_hash(const cobj* self){
    const cobj_array* array = (const cobj_array*)self->as.p;
    size_t hash = array->ndim;

    for(size_t d = 0; d < array->ndim; d++) hash = cobj_hash_combine(hash, array->shape[d]);
    for(size_t i = 0; i < array->len; i++){
        cobj element = load(array, slot_at(array, i));
        hash = cobj_hash_combine(hash, cobj_hash(&element));
    }
    return hash;
}

//One "[...]" per dimension; *position walks the elements in row-major order
static cobj_status fmt_dimension(const cobj_array* array, size_t d, size_t* position, cobj_buf* out){
    cobj_status status = cobj_buf_puts(out, "[");

    for(size_t i = 0; i < array->shape[d] && status == COBJ_OK; i++){
        if(i > 0) status = cobj_buf_puts(out, ", ");
        if(status != COBJ_OK) break;

        if(d + 1 < array->ndim){
            status = fmt_dimension(array, d + 1, position, out);
        }else{
            cobj element = load(array, slot_at(array, (*position)++));
            status = cobj_fmt(&element, out);
        }
    }

    return status == COBJ_OK ? cobj_buf_puts(out, "]") : status;
}

static cobj_status array_fmt(const cobj* self, cobj_buf* out){
    size_t position = 0;
    return fmt_dimension((const cobj_array*)self->as.p, 0, &position, out);
}

static size_t array_len(const cobj* self){
    return ((const cobj_array*)self->as.p)->len;
}

static bool array_next(const cobj* self, cobj_iter* it, cobj* out){
    const cobj_array* array = (const cobj_array*)self->as.p;
    if(it->index >= array->len) return false;

    *out = load(array, slot_at(array, it->index++));
    return true;
}

const cobj_type COBJ_ARRAY = {.name = "array", .clone = array_clone, .drop = array_drop,
                              .cmp = array_cmp, .hash = array_hash, .fmt = array_fmt,
                              .len = array_len, .next = array_next};
