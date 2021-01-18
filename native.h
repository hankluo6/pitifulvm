#pragma once

#include "java_file.h"
#include "stack.h"
#include <sys/time.h>

void void_native_method(method_t *method, local_variable_t *locals, class_file_t *clazz);
void *ptr_native_method(method_t *method, local_variable_t *locals, class_file_t *clazz);