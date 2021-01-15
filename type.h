#pragma once

#include <stdint.h>

typedef uint8_t u1;
typedef uint16_t u2;
typedef uint32_t u4;

typedef union
{
    u1 char_value;
    u2 short_value;
    u4 int_value;
    void *ptr_value;
} variable_t;

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
