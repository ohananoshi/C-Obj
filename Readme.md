## C-Obj
### A Generic Object Handling Library in C

```C
A generic structure to receives any data type and handling with custom data types and structures like simple variables
```

### Macros

```C
DEFAULT_MAX_POINTER_LEVEL 3
```

### Data Types and Structures

```C
typedef struct metadata{
    uint8_t shape[DEFAULT_MAX_POINTER_LEVEL + 1];
    uint8_t type_id;
}metadata_t;

typedef struct obj{
    void* data;
    metadata_t obj_metadata;
}obj_t;
```

### Format String Convention

```C
int8_t      ->  1  ->  "INT8"     -> [0] single variable -> [1,...] array
uint8_t     ->  2  ->  "UINT8"    -> [0] single variable -> [1,...] array 
int16_t     ->  3  ->  "INT16"    -> [0] single variable -> [1,...] array 
uint16_t    ->  4  ->  "UINT16"   -> [0] single variable -> [1,...] array  
int32_t     ->  5  ->  "INT32"    -> [0] single variable -> [1,...] array 
uint32_t    ->  6  ->  "UINT32"   -> [0] single variable -> [1,...] array  
int64_t     ->  7  ->  "INT64"    -> [0] single variable -> [1,...] array 
uint64_t    ->  8  ->  "UINT64"    -> [0] single variable -> [1,...] array 
char*       ->  9  ->  "STR"      -> [0] single variable -> [1,...] array
float       ->  10 ->  "FLOAT"    -> [0] single variable -> [1,...] array 
double      ->  11 ->  "DOUBLE"   -> [0] single variable -> [1,...] array 
long double ->  12 ->  "L_DOUBLE" -> [0] single variable -> [1,...] array   
```

```C
uint8_t ex1; -> "UINT8[0]"
float ex2[2,2]; -> "FLOAT[2,2]"
char ex3[20]; -> "STR[20]"
double *p = calloc(4, sizeof(double)); -> "DOUBLE[4]"
```

### obj_set_metadata

```C
//Set values to a empty metadata structure from fmt str

DATA_SIZE_TYPE obj_set_metadata(obj_t* dest_obj, const char* fmt)

dest_obj: variable to receive metadata
fmt     : format string

//Returns: Size in bytes of variable described by fmt

ex:

"UINT8[2,3,4]" -> sizeof(uint8_t)*2*3*4 -> 24
```

### void obj_set(obj_t* obj, const char* fmt, void* data)

```C
//Set values to a empty obj structure

void obj_set(obj_t* obj, const char* fmt, void* data)

obj : dest obj
fmt : format string
data: source data
```

### obj_free

```C
//Free a obj from memory

void obj_free(obj_t* obj)
```

### obj_get_data_size

```C
//Gets the size in bytes of the data allocated in a obj

DATA_SIZE_TYPE obj_get_data_size(obj_t obj)
```

### obj_print

```C
//Prints a obj

void obj_print(obj_t obj)
```

###

```C
//Compares two obj

DATA_CMP_TYPE obj_cmp(obj_t a, obj_t b)

//Return compare order:
    type_id:
        -1 -> a(type_id) < b(type_id)
         0 -> a(type_id) == b(type_id) continue comparing
         1 -> a(type_id) > b(type_id)

    shape:
        -1 -> a(shape) < b(shape)
         0 -> a(shape) == b(shape) continue comparing
         1 -> a(shape) > b(shape)

    elements:
        -1 -> a(first n-element) < b(first n-element)
         0 -> a(all elements) == b(all elements)
         1 -> a(first n-element) < b(first n-element)
```

### init_obj_map

```C
//Add obj_t into global type map

void init_obj_map()
```

### Future Improvements

```
Possibility to distinguish by fmt if a array is dynamic or static allocated

Better prints
```