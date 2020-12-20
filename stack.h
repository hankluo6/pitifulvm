#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

/* stack frame type*/
typedef enum {
    STACK_ENTRY_NONE,
    STACK_ENTRY_BYTE,
    STACK_ENTRY_SHORT,
    STACK_ENTRY_INT,
    STACK_ENTRY_REF,
    STACK_ENTRY_LONG, 
    STACK_ENTRY_DOUBLE,
    STACK_ENTRY_FLOAT
} stack_entry_type_t;

/* max 8 bytes entry, very waste memory */
typedef struct {
    unsigned char entry[8];
    stack_entry_type_t type;
} stack_entry_t;

typedef struct {
    int max_size;
    int size;
    stack_entry_t *store;
} stack_frame_t;



void init_stack(stack_frame_t *stack, size_t entry_size);
void push_byte(stack_frame_t *stack, int8_t value);
void push_short(stack_frame_t *stack, int16_t value);
void push_int(stack_frame_t *stack, int32_t value);
int pop_int(stack_frame_t *stack);
void pop_to_local(stack_frame_t *stack, local_variable_t *locals);
size_t get_type_size(stack_entry_type_t type);