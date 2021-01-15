#pragma once

#include "java_file.h"
#include <dirent.h> 
#include <unistd.h>


typedef struct {
    u2 length;
    meta_class_t **class_info;
} class_heap_t;

class_heap_t class_heap;

void init_class_heap();
void free_class_heap();
void add_class(class_file_t *clazz, char *path, char *name);
class_file_t *find_class_from_heap(char *value);
void load_native_class(char *name);