#pragma once

#include "jvm_info.h"



typedef struct
{
    variable_t* ptr;
    class_file_t *type;
    size_t field_count;
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
void *create_array(class_file_t *clazz, int count);
void **create_two_dimension_array(class_file_t *clazz, int count1, int count2);
char *create_string(class_file_t *clazz, char *src);
