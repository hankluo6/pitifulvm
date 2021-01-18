#include "stack.h"

void init_stack(stack_frame_t *stack, size_t entry_size)
{
    memset(stack, 0, sizeof(stack_frame_t));
    stack->max_size = entry_size;
    stack->store = (stack_entry_t *) malloc(sizeof(stack_entry_t) * entry_size);
    for (size_t i = 0; i < entry_size; i++)
        memset(&stack->store[i], 0, sizeof(stack_entry_t));
    stack->size = 0;
}

void push_byte(stack_frame_t *stack, int8_t value)
{
    stack->store[stack->size].entry.char_value = value;
    stack->store[stack->size].type = STACK_ENTRY_BYTE;
    stack->size++;
}

void push_short(stack_frame_t *stack, int16_t value)
{
    stack->store[stack->size].entry.short_value = value;
    stack->store[stack->size].type = STACK_ENTRY_SHORT;
    stack->size++;
}

void push_int(stack_frame_t *stack, int32_t value)
{
    stack->store[stack->size].entry.int_value = value;
    stack->store[stack->size].type = STACK_ENTRY_INT;
    stack->size++;
}

void push_long(stack_frame_t *stack, int64_t value)
{
    unsigned char *tmp = stack->store[stack->size].entry.val;
    memcpy(tmp, &value, sizeof(int64_t));
    stack->store[stack->size].type = STACK_ENTRY_LONG;
    stack->size++;
}

void push_ref(stack_frame_t *stack, void *addr)
{
    stack->store[stack->size].entry.ptr_value = addr;
    stack->store[stack->size].type = STACK_ENTRY_REF;
    stack->size++;
}

stack_entry_t top(stack_frame_t *stack)
{
    return stack->store[stack->size - 1];
}

int64_t stack_to_int(unsigned char *entry, size_t size)
{
    switch (size) {
    /* int8_t */
    case 1: {
        int8_t value;
        value = entry->char_value;
        return value;
    }
    /* int16_t */
    case 2: {
        int16_t value;
        value = entry->short_value;
        return value;
    }
    /* int32_t */
    case 4: {
        int32_t value;
        value = entry->int_value;
        return value;
    }
    /* int64_t */
    case 8: {
        int64_t value;
        memcpy(&value, entry, size);
        return value;
    }
    default:
        assert(0 && "stack entry not an interger");
        return -1;
    }
}

int64_t pop_int(stack_frame_t *stack)
{
    size_t size = get_type_size(stack->store[stack->size - 1].type);
    int64_t value = stack_to_int(stack->store[stack->size - 1].entry.val, size);
    stack->size--;
    return value;
}

void *pop_ref(stack_frame_t *stack)
{
    return stack->store[--stack->size].entry.ptr_value;
}

void pop_to_local(stack_frame_t *stack, local_variable_t *locals)
{
    stack_entry_type_t type = stack->store[stack->size - 1].type;
    /* convert to integer */
    if (type >= STACK_ENTRY_BYTE && type <= STACK_ENTRY_LONG) {
        int64_t value = pop_int(stack);
        memcpy(&locals->entry, &value, sizeof(int64_t));
        locals->type = type;
    }
    else if (type == STACK_ENTRY_REF) {
        void *addr = pop_ref(stack);
        locals->entry.ptr = addr;
        locals->type = type;
    }
    else {
        assert(0 && "not support stack type");
    }
}

size_t get_type_size(stack_entry_type_t type)
{
    size_t size = 0;
    switch (type) {
    case STACK_ENTRY_NONE:
        /* pass */
        break;
    case STACK_ENTRY_BYTE:
        size = sizeof(int8_t);
        break;
    case STACK_ENTRY_SHORT:
        size = sizeof(int16_t);
        break;
    case STACK_ENTRY_INT:
        size = sizeof(int32_t);
        break;
    case STACK_ENTRY_REF:
        size = sizeof(size_t);
        break;
    case STACK_ENTRY_LONG:
        size = sizeof(int64_t);
        break;
    case STACK_ENTRY_DOUBLE:
        size = sizeof(double);
        break;
    case STACK_ENTRY_FLOAT:
        size = sizeof(float);
        break;
    default:
        assert("unable to recognize stack entry's tag");
    }
    return size;
}