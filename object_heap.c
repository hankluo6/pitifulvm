#include "object_heap.h"

void init_object_heap()
{
    /* max contain 100 object */
    object_heap.objects = malloc(sizeof(object_t*) * 100);
    object_heap.length = 0;
}

object_t* create_object(class_file_t *clazz)
{
    size_t size = clazz->fields_count * sizeof(variable_t);
    object_t *new_obj = malloc(sizeof(object_t));
    new_obj->ptr = malloc(size);
    new_obj->type = clazz;
    object_heap.objects[object_heap.length++] = new_obj;

    return new_obj;
}

size_t get_field_size(class_file_t *clazz)
{
    size_t size = 0;
    field_t *field = clazz->fields;
    for (u2 i = 0; i < clazz->fields_count; i++, field++) {
        if (strcmp(field->descriptor, "B") == 0) {
            size += sizeof(u1);
        } else if (strcmp(field->descriptor, "C") == 0) {
            size += sizeof(u1);
        } else if (strcmp(field->descriptor, "D") == 0) {
            size += sizeof(double);
        } else if (strcmp(field->descriptor, "F") == 0) {
            size += sizeof(float);
        } else if (strcmp(field->descriptor, "I") == 0) {
            size += sizeof(int);
        } else if (strcmp(field->descriptor, "J") == 0) {
            size += sizeof(long);
        } else if (strcmp(field->descriptor, "S") == 0) {
            size += sizeof(u2);
        } else if (strcmp(field->descriptor, "Z") == 0) {
            size += sizeof(bool);
        } else if (strcmp(field->descriptor, "V") == 0) {
            /* TODO */
            size += 0; 
        } else if (strcmp(field->descriptor, "L") == 0) {
            /* pointer type */
            size += sizeof(size_t);
        } else {
            assert(0 && "cannot find field type");
        }
    }
    return size;
}

variable_t *find_field_addr(object_t *obj, char *name)
{
    field_t *field = obj->type->fields;
    u2 i;
    for (i = 0; i < obj->type->fields_count; i++, field++) {
        if (strcmp(field->name, name) == 0) {
            return &obj->ptr[i];
        }
    }
    assert(0 && "cannot find field name");
}

void free_object_heap()
{
    for (int i = 0; i < object_heap.length; ++i) {
        free(object_heap.objects[i]->ptr);
        free(object_heap.objects[i]);
    }
    free(object_heap.objects);
}