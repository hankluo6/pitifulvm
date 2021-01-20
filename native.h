#pragma once

#include <sys/time.h>
#include "java_file.h"
#include "stack.h"

void void_native_method(method_t *method, local_variable_t *locals);
void *ptr_native_method(method_t *method, local_variable_t *locals);