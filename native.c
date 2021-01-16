#include "native.h"

/* not check class type */
int32_t *native_method(method_t *method, local_variable_t *locals, class_file_t *clazz)
{
    if (strcmp(method->name, "println") == 0) {
        if (strcmp(method->descriptor, "()V") == 0) {
            printf("\n");
            return NULL;
        }
        else if (strcmp(method->descriptor, "(I)V") == 0) {
            int32_t value = stack_to_int(locals[1].entry.val, 4);
            printf("%d\n", value);
            return NULL;
        }
        else if (strcmp(method->descriptor, "(Ljava/lang/String;)V") == 0) {
            void *addr = locals[1].entry.ptr;
            printf("%s\n", (char *)addr);
            return NULL;
        }
    }
    else if (strcmp(method->name, "print") == 0) {
        if (strcmp(method->descriptor, "(Ljava/lang/String;)V") == 0) {
            void *addr = locals[1].entry.ptr;
            printf("%s", (char *)addr);
            return NULL;
        }
    }
    else if (strcmp(method->name, "flush") == 0) {
        if (strcmp(method->descriptor, "()V") == 0) {
            fflush(stdout);
        }
    }
    return NULL;
}