#ifndef C_ARRAY_H
#define C_ARRAY_H

/**
 * @file c_array.h
 * @brief C-Array: a typed n-dimensional array, like a NumPy ndarray. Every element
 * has the same type and is stored unboxed and contiguously, in row-major
 * order (the last index varies fastest).
 *
 * @code
 * cobj_array* m = cobj_array_new(&COBJ_F64, 2, COBJ_IDX(2, 3));   // 2 x 3 zeros
 * cobj_array_set(m, COBJ_IDX(1, 2), cobj_f64(4.5));
 * double* raw = cobj_array_data(m);                               // raw[5] == 4.5
 * cobj matrix = cobj_array_value(m);
 * cobj_print(&matrix);                         // [[0.0, 0.0, 0.0], [0.0, 0.0, 4.5]]
 * cobj_drop(&matrix);
 * @endcode
 *
 * Element types: the numeric built-ins (COBJ_I8 ... COBJ_U64, COBJ_F32,
 * COBJ_F64), COBJ_BOOL, and plain-data types, which have `size > 0` and no clone or drop
 * (COBJ_LDOUBLE, plain structs).
 *
 * Set functions consume the value they receive, even when they fail.
 * Iterating an array value yields its elements in row-major order; plain-data
 * elements are borrowed views into the array's storage.
 */

#include "../c_obj.h"

/// An index as a temporary array: `cobj_array_get(a, COBJ_IDX(1, 2), &out)`
#define COBJ_IDX(...) ((const size_t[]){__VA_ARGS__})

/// An n-dimensional array (opaque); create it with cobj_array_new() or cobj_array_make()
typedef struct cobj_array cobj_array;

/// The array type descriptor
extern const cobj_type COBJ_ARRAY;

/// Elements start zeroed. NULL when dtype isn't supported, ndim is 0, shape is NULL or allocation fails
cobj_array* cobj_array_new(const cobj_type* dtype, size_t ndim, const size_t* shape);

/// NULL is ignored
void cobj_array_free(cobj_array* array);

/// The element type; NULL for NULL
const cobj_type* cobj_array_dtype(const cobj_array* array);

/// Number of dimensions; 0 for NULL
size_t cobj_array_ndim(const cobj_array* array);

/// ndim sizes, owned by the array
const size_t* cobj_array_shape(const cobj_array* array);

/// Total number of elements
size_t cobj_array_len(const cobj_array* array);

/// An owned copy of the element at an n-d index (numbers need no allocation). COBJ_ERANGE when out of range
cobj_status cobj_array_get(const cobj_array* array, const size_t* index, cobj* out);

/// COBJ_ETYPE when value's type isn't the array's dtype, COBJ_ERANGE when the index is out of range
cobj_status cobj_array_set(cobj_array* array, const size_t* index, cobj value);

/// The same with a row-major position, from 0 to len - 1
cobj_status cobj_array_get_flat(const cobj_array* array, size_t position, cobj* out);

/// cobj_array_set() with a row-major position
cobj_status cobj_array_set_flat(cobj_array* array, size_t position, cobj value);

/// Writes value to every element
cobj_status cobj_array_fill(cobj_array* array, cobj value);

/// Changes the shape, keeping the elements in row-major order. COBJ_EINVAL when the total size differs
cobj_status cobj_array_reshape(cobj_array* array, size_t ndim, const size_t* shape);

/// The elements in row-major order, as the dtype's C type (double* for COBJ_F64...)
void* cobj_array_data(cobj_array* array);

/// Moves items[0..count) into a new 1-d array and leaves them empty. Every item must have type dtype;
/// otherwise, or when allocation fails, every item is dropped and NULL is returned
cobj_array* cobj_array_from_values(const cobj_type* dtype, size_t count, cobj* items);

/// A 1-d array value of dtype from 1 to 32 values, each passed through cobj_of:
/// `cobj_array_make(&COBJ_F64, 1.0, 2.5)`. The C types must match dtype (1.0, not 1, for #COBJ_F64)
#define cobj_array_make(dtype, ...)                                                         \
    cobj_array_value(cobj_array_from_values(dtype, COBJ_ARG_COUNT(__VA_ARGS__),             \
                                            (cobj[]){COBJ_MAP(cobj_of, __VA_ARGS__)}))

/// A new 1-d array with the elements of any iterable, which must all have type dtype. NULL otherwise
cobj_array* cobj_array_from(const cobj_type* dtype, const cobj* iterable);

/// Moves the array into a value, which now owns it. Empty value when array is NULL
cobj cobj_array_value(cobj_array* array);

/// The array held by a value (borrowed), NULL for any other type
cobj_array* cobj_array_of(const cobj* value);

#endif //C_ARRAY_H
