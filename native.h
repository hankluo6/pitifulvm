#pragma once

#include "java_file.h"
#include "stack.h"


int32_t *native_method(method_t *method, local_variable_t *locals, class_file_t *clazz);