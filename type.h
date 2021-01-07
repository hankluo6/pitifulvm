
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