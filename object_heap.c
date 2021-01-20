#include "object_heap.h"

void init_object_heap()
{
    /* max contain 5000 object */
    object_heap.objects = malloc(sizeof(object_t *) * 5000);
    object_heap.length = 0;
}

/* create java object */
object_t *create_object(class_file_t *clazz)
{
    size_t size = clazz->fields_count * sizeof(variable_t);
    object_t *new_obj = malloc(sizeof(object_t));
    new_obj->field_count = clazz->fields_count;
    /* prevent undefined behavior */
    if (size == 0) {
        new_obj->ptr = NULL;
    } else {
        new_obj->ptr = malloc(size);
        memset(&new_obj->ptr->value, 0, size);
        for (int i = 0; i < clazz->fields_count; ++i) {
            new_obj->ptr[i].type = VAR_NONE;
        }
    }
    new_obj->type = clazz;
    object_heap.objects[object_heap.length++] = new_obj;
    return new_obj;
}

/* create array object, place it address in object heap */
void *create_array(class_file_t *clazz, int count)
{
    void *arr = malloc(count * sizeof(int));
    object_t *arr_obj = malloc(sizeof(object_t));
    arr_obj->ptr = malloc(sizeof(variable_t));
    arr_obj->ptr->type = VAR_ARRAY_PTR;
    arr_obj->ptr->value.ptr_value = arr;
    /* this type attribute is useless */
    arr_obj->type = clazz;
    object_heap.objects[object_heap.length++] = arr_obj;
    arr_obj->field_count = 1;

    return arr;
}

/**
 * create two dimension array, place it address in object heap
 * note that:
 *      field 0 store array address,
 *      field 1 store first dimension size
 *      field 2 store second dimension size
 */
void **create_two_dimension_array(class_file_t *clazz, int count1, int count2)
{
    /* only support integer array */
    int **arr = malloc(count1 * sizeof(int *));
    for (int i = 0; i < count1; ++i) {
        arr[i] = malloc(count2 * sizeof(int));
    }
    object_t *arr_obj = malloc(sizeof(object_t));
    arr_obj->ptr = malloc(sizeof(variable_t) * 3);
    arr_obj->ptr[0].type = VAR_MULTARRAY_PTR;
    arr_obj->ptr[0].value.ptr_value = arr;
    arr_obj->ptr[1].type = VAR_INT;
    arr_obj->ptr[1].value.int_value = count1;
    arr_obj->ptr[2].type = VAR_INT;
    arr_obj->ptr[2].value.int_value = count2;
    /* this type attribute is useless */
    arr_obj->type = clazz;
    object_heap.objects[object_heap.length++] = arr_obj;
    arr_obj->field_count = 3;

    return (void **) arr;
}

/**
 *  create string object and put it in object heap
 *  clazz must be java/lang/String
 */
char *create_string(class_file_t *clazz, char *src)
{
    char *dest = malloc((strlen(src) + 1) * sizeof(char));
    strcpy(dest, src);
    object_t *str_obj = malloc(sizeof(object_t));
    str_obj->ptr = malloc(sizeof(variable_t));
    str_obj->ptr->type = VAR_STR_PTR;
    str_obj->ptr->value.ptr_value = dest;
    str_obj->type = clazz;
    object_heap.objects[object_heap.length++] = str_obj;
    str_obj->field_count = 1;

    return dest;
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
    return NULL;
}

void free_object_heap()
{
    for (int i = 0; i < object_heap.length; ++i) {
        if (object_heap.objects[i]->ptr) {
            if (object_heap.objects[i]->ptr->type == VAR_STR_PTR) {
                free(object_heap.objects[i]->ptr->value.ptr_value);
            } else if (object_heap.objects[i]->ptr->type == VAR_ARRAY_PTR) {
                free(object_heap.objects[i]->ptr->value.ptr_value);
            } else if (object_heap.objects[i]->ptr->type == VAR_MULTARRAY_PTR) {
                int count = object_heap.objects[i]->ptr[1].value.int_value;
                int **addr = object_heap.objects[i]->ptr[0].value.ptr_value;
                for (int j = 0; j < count; ++j) {
                    free(addr[j]);
                }
                free(addr);
            }
        }
        free(object_heap.objects[i]->ptr);
        free(object_heap.objects[i]);
    }
    free(object_heap.objects);
}