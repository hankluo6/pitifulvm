#pragma once

#include "jvm_info.h"



typedef struct
{
    variable_t* ptr;
    class_file_t *type;
} object_t;


typedef struct {
    u2 length;
    object_t** objects;
} object_heap_t;

// move to jvm.c
object_heap_t object_heap;

void init_object_heap();
void free_object_heap();
object_t* create_object(class_file_t *clazz);
variable_t *find_field_addr(object_t *obj, char *name);
