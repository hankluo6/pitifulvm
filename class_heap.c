#include "class_heap.h"

void init_class_heap()
{
    /* max contain 100 class file */
    class_heap.class_info = malloc(sizeof(meta_class_t*) * 100);
    class_heap.length = 0;
}

/* the name parameter should contain ".class" suffix and be cut in this function */
void add_class(class_file_t *clazz, char *path, char *name)
{
    meta_class_t *meta_class = malloc(sizeof(meta_class_t));
    meta_class->clazz = clazz;
    /* assume class file is in the same directory */
    meta_class->path = NULL; 
    meta_class->name = malloc(sizeof(char) * (strlen(name) + 1 - 6));
    strncpy(meta_class->name, name, strlen(name) - 6);
    meta_class->name[strlen(name) - 6] = '\0';

    class_heap.class_info[class_heap.length++] = meta_class;
}

class_file_t *find_class_from_heap(char *value)
{
    for (int i = 0; i < class_heap.length; ++i) {
        if (strcmp(class_heap.class_info[i]->name, value) == 0) {
            return class_heap.class_info[i]->clazz;
        }
    }
    return NULL;
}

/* recursively list all file in directory and add these class in heap */
void load_native_class(char *name)
{
    DIR *dir;
    struct dirent *entry;

    if (!(dir = opendir(name)))
        return;

    while ((entry = readdir(dir)) != NULL) {
        char path[1024];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
        char *pch;
        if ((pch = strstr(path, ".class")) != NULL) {

            FILE *class_file = fopen(path, "r");
            assert(class_file && "Failed to open file");

            /* parse the class file */
            /* change to heap */
            class_file_t *clazz = malloc(sizeof(class_file_t));
            *clazz = get_class(class_file);
            
            int error = fclose(class_file);
            assert(!error && "Failed to close file");

            add_class(clazz, NULL, path);
        }
        else {
            load_native_class(path);
        }
    }
    closedir(dir);
}

void free_class_heap()
{
    for (int i = 0; i < class_heap.length; ++i) {
        const_pool_info *constant = class_heap.class_info[i]->clazz->constant_pool.constant_pool;
        for (u2 j = 0; j < class_heap.class_info[i]->clazz->constant_pool.constant_pool_count; j++, constant++) {
            free(constant->info);
        }
        free(class_heap.class_info[i]->clazz->constant_pool.constant_pool);

        field_t *field = class_heap.class_info[i]->clazz->fields;
        for (u2 j = 0; j < class_heap.class_info[i]->clazz->fields_count; j++, field++)
            free(field->value);
        free(class_heap.class_info[i]->clazz->fields);

        for (method_t *method = class_heap.class_info[i]->clazz->methods; method->name; method++)
            free(method->code.code);
        free(class_heap.class_info[i]->clazz->methods);

        free(class_heap.class_info[i]->clazz->interfaces);

        bootstrapMethods_attribute_t *bootstrap = class_heap.class_info[i]->clazz->bootstrap;
        if (bootstrap) {
            for (u2 j = 0; j < bootstrap->num_bootstrap_methods; j++) {
                bootstrap_methods_t bootstrap_method = bootstrap->bootstrap_methods[j];
                for (u2 k = 0; k < bootstrap_method.num_bootstrap_arguments; ++k) {
                    free(bootstrap_method.bootstrap_arguments);
                }
            }
            free(bootstrap->bootstrap_methods);
            free(bootstrap);
        }

        free(class_heap.class_info[i]->clazz);
        free(class_heap.class_info[i]->name);
        free(class_heap.class_info[i]);
    }
    free(class_heap.class_info);
}