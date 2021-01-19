#include "native.h"

/* not check class type yet */
void void_native_method(method_t *method, local_variable_t *locals, class_file_t *clazz)
{
    if (strcmp(method->name, "println") == 0) {
        if (strcmp(method->descriptor, "()V") == 0) {
            printf("\n");
        }
        else if (strcmp(method->descriptor, "(I)V") == 0) {
            int32_t value = stack_to_int(&locals[1].entry, sizeof(int32_t));
            printf("%d\n", value);
        }
        else if (strcmp(method->descriptor, "(Ljava/lang/String;)V") == 0) {
            void *addr = locals[1].entry.ptr_value;
            printf("%s\n", (char *)addr);
        }
    }
    else if (strcmp(method->name, "print") == 0) {
        if (strcmp(method->descriptor, "(Ljava/lang/String;)V") == 0) {
            void *addr = locals[1].entry.ptr_value;
            printf("%s", (char *)addr);
        }
    }
    else if (strcmp(method->name, "flush") == 0) {
        if (strcmp(method->descriptor, "()V") == 0) {
            fflush(stdout);
        }
    }
}

void *ptr_native_method(method_t *method, local_variable_t *locals, class_file_t *clazz) {
    if (strcmp(method->name, "readLine") == 0) {
        if (strcmp(method->descriptor, "()Ljava/lang/String;") == 0) {
            char *str = malloc(sizeof(char) * 50);
            int ret = scanf("%50s", str);
            assert(ret > 0 && "scanf error");
            return str;
        }
    }
    else if (strcmp(method->name, "parseLong") == 0) {
        if (strcmp(method->descriptor, "(Ljava/lang/String;)J") == 0) {
            void *addr = locals[1].entry.ptr_value;
            long *value = malloc(sizeof(long));
            *value = atoll((char *)addr);
            return value;
        }
    }
    else if (strcmp(method->name, "currentTimeMillis") == 0) {
        if (strcmp(method->descriptor, "()J") == 0) {
            struct timeval time;
            gettimeofday(&time, NULL);
            int64_t s1 = (int64_t)(time.tv_sec) * 1000;
            int64_t s2 = (time.tv_usec / 1000);
            long *value = malloc(sizeof(long));
            *value = s1 + s2;
            return value;
        }
    }
    else if (strcmp(method->name, "charAt") == 0) {
        if (strcmp(method->descriptor, "(I)C") == 0) {
            char *str = locals[0].entry.ptr_value;
            char *c = malloc(sizeof(char));
            int32_t index = stack_to_int(&locals[1].entry, sizeof(int32_t));
            *c = str[index];
            return c;
        }
    }
    else if (strcmp(method->name, "compareTo") == 0) {
        if (strcmp(method->descriptor, "(Ljava/lang/String;)I") == 0) {
            char *str = locals[0].entry.ptr_value;
            char *str2 = locals[1].entry.ptr_value;

            size_t l1 = strlen(str), l2 = strlen(str2);
            int idx = 0;
            int result;

            int end = (l1 < l2 ? l1 : l2);
            int32_t *ret = malloc(sizeof(int32_t));

            while (idx < end) {
                if ((result = str[idx] - str2[idx]) != 0) {
                    *ret = result;
                    return ret;
                }
                idx++;
            }
            *ret = result;
            return ret;
        }
    }
    return NULL;
}