#pragma once

#include <stdint.h>

typedef uint8_t u1;
typedef uint16_t u2;
typedef uint32_t u4;
typedef uint64_t u8;

typedef union
{
    u1 char_value;
    u2 short_value;
    u4 int_value;
    u8 long_value;
    void *ptr_value;
} value_t;

typedef struct
{
    value_t value;
    int type;    
} variable_t;

typedef enum
{
    NONE = 0,
    BYTE = 1,
    SHORT = 2,
    INT = 3,
    LONG = 4,
    PTR = 5,
    ARRAY_PTR = 6,
    MULTARRAY_PTR = 7,
    STR_PTR = 8
} variable_type_t;

typedef enum
{
    T_BOOLEN = 4,
    T_CHAR = 5,
    T_FLOAT = 6,
    T_DOUBLE = 7,
    T_BYTE = 8,
    T_SHORT = 9, 
    T_INT = 10,
    T_LONG = 11
} array_type_t;
