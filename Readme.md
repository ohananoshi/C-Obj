## C-Obj

### A Generic Object Handling Library in C

```C
A generic structure and functions to allow building of custom interoperable structures
```

### Constants

```C
//Ids for all primitive types and some more
UINT8_T
UINT16_T
UINT32_T
UINT64_T
INT8_T
INT16_T
INT32_T
INT64_T
FLOAT
DOUBLE
LONG_DOUBLE
CHAR
C_OBJ
SAME
VOIDP
END // Used in va_args to point the end
```

### Data Types and Structures

#### c_obj_t
```C

typedef struct c_obj_t{
    void* value;     //used to carry any data information
    int8_t obj_type; //receives the type ID for void handling
}c_obj_t;
```

#### obj base functions
Any custom structure implemented with these functions can be compatible themselves.

```C
typedef int8_t (*obj_set_fn)(c_obj_t* dst, ...);
//Standard function format to copy data from any source to a c_obj_t variable

typedef int8_t (*obj_get_fn)(c_obj_t src, ...);
//Standard function format to copy data from a c_obj_t to a variable

typedef int32_t (*obj_cmp_fn)(void* src, ...);
//Standard function format to compare a custom type to any other value

typedef int8_t (*obj_print_fn)(void* src, ...);
//Standard function format to print a custom type
```

#### custom type mapping
To work with custom structures we need to map these structures building functions in _common attributes_ that cab be acessed by standard obj functions. The attribute map have a linked list shape. 


```C
typedef struct common_attributes_t{
    obj_set_fn set;
    obj_get_fn get;
    obj_cmp_fn cmp;
    obj_print_fn print;
}common_attributes_t;

typedef struct att_map_t{
    uint8_t type_id;
    common_attributes_t* att;
    struct att_map_t* next;
}att_map_t;
```

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

#### obj std functions
Common functions to c_obj_t variables handling

```C
int8_t obj_std_set(c_obj_t* dst, ...);
/**
 *  A wrapper to obj_std_vset;
 * 
 * Args:
 *  va_args:
 *      att_map_t attribute_map (global variable);
 *      int8_t source_value_type_ID;
 *      source variabel/value;
 * 
 * Sample:
 *  obj_std_set(&src, global_attribute_map, UINT8_T, 123);
 * */
```

```C
int8_t obj_std_vset(c_obj_t* dst, att_map_t attribute_map,va_list args);
/**
 *  Standard function to handling copy from any mapped data to a c_obj_t variable;
 * 
 * Args:
 *  att_map_t attribute_map (global variable);
 *  va_list args:
 *      int8_t source_value_type_ID;
 *      source variabel/value;
 * */
```
```C
int8_t obj_std_get(c_obj_t src, ...);
/**
 *  A wrapper to obj_std_vget;
 * 
 * Args:
 *  va_args:
 *      att_map_t attribute_map (global variable);
 *      source variabel/value;
 * 
 * Sample:
 *  obj_std_set(src, global_attribute_map, &dst);
 * */
```
```C
int8_t obj_std_vget(c_obj_t src, att_map_t attribute_map, va_list args);
/**
 *  Standard function to handling copy from a c_obj_t to a variable;
 * 
 * Args:
 *  va_list args:
 *      void* dst_variable;
 * */
```
```C
int32_t obj_std_cmp(c_obj_t src, ...);
/**
 *  A wrapper to obj_std_vcmp;
 * 
 * Args:
 *  va_args:
 *      att_map_t attribute_map (global variable);
 *      int8_t source_value_type_ID;
 *      source variabel/value;
 * 
 * Sample:
 *  double b_value = 3.14;
 *  obj_std_cmp(src, global_attribute_map, DOUBLE, b_value);
 * */
```
```C
int32_t obj_std_vcmp(c_obj_t src, att_map_t attribute_map, va_list args);
/**
 *  Standard function to handling comparison between a c_obt_t with any other value;
 * 
 * Args:
 *  va_list args:
 *      int8_t source_value_type_ID;
 *      source variabel/value;
 * */
```
```C
int8_t obj_std_print(c_obj_t src, ...);
/**
 *  A wrapper to obj_std_vprint;
 * 
 * Args:
 *  va_args:
 *      att_map_t attribute_map (global variable);
 * 
 * Sample:
 *  obj_std_set(src, global_attribute_map);
 * */
```
```C
int8_t obj_std_vprint(c_obj_t src, att_map_t attribute_map, va_list args);
/**
 *  Standard function to handling print of a c_obj_t;
 * 
 * Args:
 *  va_list args: not used
 * */
```
```C
uint8_t add_maps(att_map_t* dst, ...);
/**
 *  Function to add new common_attributes to global attribute_map;
 * 
 * */
```

### Building your own custom type
C-Obj works like a design pattern where you decorate your base function with standard wrapplers.

```
header file: person.h
c file: person.h
sample structure: person_t
```

```C
//person.h

/*  YOUR LIBS */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>

/* Step 01: Include c_obj lib in your custom lib */
#include "c-obj/c_obj.h"

/* Step 02: Define your custom type ID (must be greater than 50 and different from other implemented type ID) */
#define PERSON_ID 57

/* Step 03: Declare your type */
typedef struct person_t{
    char name[100];
    uint8_t age;
    float weight;
}person_t;

/* Step 04: Declare and implement the base functions for your struct in the same format of standar base functions */

int8_t person_set_obj(c_obj_t* dst, ...);

int8_t person_get_obj(c_obj_t src, ...);

int32_t person_cmp(void* src, ...)

int8_t person_print(void* src, ...)

/* Step 05: Declare and initialize your common struct */
extern common_attributes_t person_att;
```

```C
//person.c
#include "person.h"

/* Step 04: ... */
int8_t person_set_obj(c_obj_t* dst, ...){
    va_list args;
    va_start(args, dst);

    person_t src = *(person_t*)va_arg(args, void*);

    dst->value = calloc(1, sizeof(person_t));
    memcpy(&src, dst->value, sizeof(person_t));

    dst->obj_type = PERSON_ID;

    va_end(args);
}

int8_t person_get_obj(c_obj_t src, ...){
    va_list args;
    va_start(args, src);

    void *dst = va_arg(args, void*);

    memcpy(src.value, dst, sizeof(person_t));

    va_end(args); 
}

int32_t person_cmp(void* src, ...){
    va_list args;
    va_start(args, src);

    person_t b_person = *(person_t*)va_arg(args, void*);
    person_t a_person = *(person_t*)src;

    int32_t cmp = strcmp(a.name, b.name);
    if(cmp != 0) return cmp;

    cmp = a.age - b.age;
    if(cmp != 0) return cmp;

    return a.weight - b.weight;
}

int8_t person_print(void* src, ...){
    person_t person = *(person_t*)src;

    printf("%s:\n\tAge: %d\n\tWeight: %.2f\n", person.name, person.age, person.weight);
}

/* Step 05: ... */
extern common_attributes_t list_att = {person_set_obj, person_get_obj, person_cmp, person_print};

```

```C
//main.c

#include "c_list/c_list.h"

/* Step 06: include your header */
#include "person/person.h"

/* Step 07: Initialize the attribute map */
att_map_t att_map = {0,NULL, NULL};

int main(){
    /* Step 08: Add your custom common into att_map */
    add_maps(&att_map, PERSON_ID, &person_att);

    /*For example: now you can have a list of persons*/
    person_t a = {"Ivan", 32, 1.72};
    person_t b = {"Ze", 26, 1.81}

    list_t my_list = list_init;
    list_append(&my_list, att_map, PERSON_ID, &a, PERSON_ID, &b, END);
}
```