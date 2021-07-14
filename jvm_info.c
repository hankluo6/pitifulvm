#include "jvm_info.h"

/* Read unsigned big-endian integers */
u1 read_u1(FILE *class_file)
{
    int result = fgetc(class_file);
    assert(result != EOF && "Reached end of file prematurely");
    return result;
}

u2 read_u2(FILE *class_file)
{
    return (u2) read_u1(class_file) << 8 | read_u1(class_file);
}

u4 read_u4(FILE *class_file)
{
    return (u4) read_u2(class_file) << 16 | read_u2(class_file);
}

/**
 * Get the constant at the given index in a constant pool.
 * Assert that the index is valid (i.e. between 1 and the pool size).
 *
 * @param constant_pool the class's constant pool
 * @param index the 1-indexed constant pool index
 * @return the constant at the given index
 */
const_pool_info *get_constant(constant_pool_t *constant_pool, u2 index)
{
    assert(0 < index && index <= constant_pool->constant_pool_count &&
           "Invalid constant pool index");
    /* Convert 1-indexed index to 0-indexed index */
    return &constant_pool->constant_pool[index - 1];
}

CONSTANT_NameAndType_info *get_method_name_and_type(constant_pool_t *cp, u2 idx)
{
    const_pool_info *method = get_constant(cp, idx);
    assert(method->tag == CONSTANT_MethodRef && "Expected a MethodRef");
    const_pool_info *name_and_type_constant = get_constant(
        cp,
        ((CONSTANT_FieldOrMethodRef_info *) method->info)->name_and_type_index);
    assert(name_and_type_constant->tag == CONSTANT_NameAndType &&
           "Expected a NameAndType");
    return (CONSTANT_NameAndType_info *) name_and_type_constant->info;
}

CONSTANT_FieldOrMethodRef_info *get_methodref(constant_pool_t *cp, u2 idx)
{
    const_pool_info *method = get_constant(cp, idx);
    assert(method->tag == CONSTANT_MethodRef && "Expected a MethodRef");
    return (CONSTANT_FieldOrMethodRef_info *) method->info;
}

CONSTANT_Class_info *get_class_name(constant_pool_t *cp, u2 idx)
{
    const_pool_info *class = get_constant(cp, idx);
    assert(class->tag == CONSTANT_Class && "Expected a Class");
    return (CONSTANT_Class_info *) class->info;
}

/**
 * Get the number of integer parameters that a method takes.
 * Use the descriptor string of the method to determine its signature.
 */
uint16_t get_number_of_parameters(method_t *method)
{
    /* Type descriptors have the length ( + #params + ) + return type */
    return strlen(method->descriptor) - 3;
}

/**
 * Find the method with the given name and signature.
 * The descriptor is necessary because Java allows method overloading.
 * This only needs to be called directly to invoke main();
 * for the invokestatic instruction, use find_method_from_index().
 *
 * @param name the method name, e.g. "factorial"
 * @param desc the method descriptor string, e.g. "(I)I"
 * @param clazz the parsed class file
 * @return the method if it was found, or NULL
 */
method_t *find_method(const char *name, const char *desc, class_file_t *clazz)
{
    for (method_t *method = clazz->methods; method->name; method++) {
        if (!(strcmp(name, method->name) || strcmp(desc, method->descriptor)))
            return method;
    }
    return NULL;
}

char *find_method_info_from_index(uint16_t idx, class_file_t *clazz, char **name_info, char **descriptor_info)
{
    CONSTANT_FieldOrMethodRef_info *method_ref =
        get_methodref(&clazz->constant_pool, idx);
    const_pool_info *name_and_type =
        get_constant(&clazz->constant_pool, method_ref->name_and_type_index);
    assert(name_and_type->tag == CONSTANT_NameAndType && "Expected a NameAndType");
    const_pool_info *name =
        get_constant(&clazz->constant_pool, ((CONSTANT_NameAndType_info *) name_and_type->info)->name_index);
    assert(name->tag == CONSTANT_Utf8 && "Expected a UTF8");
    *name_info = (char *)name->info;
    const_pool_info *descriptor =
        get_constant(&clazz->constant_pool, ((CONSTANT_NameAndType_info *) name_and_type->info)->descriptor_index);
    assert(descriptor->tag == CONSTANT_Utf8 && "Expected a UTF8");
    *descriptor_info = (char *)descriptor->info;
    CONSTANT_Class_info *class_info =
        get_class_name(&clazz->constant_pool, method_ref->class_index);
    const_pool_info *class_name =
        get_constant(&clazz->constant_pool, class_info->string_index);

    return (char *)class_name->info;
}

class_header_t get_class_header(FILE *class_file)
{
    return (class_header_t){
        .magic = read_u4(class_file),
        .major_version = read_u2(class_file),
        .minor_version = read_u2(class_file),
    };
}

constant_pool_t get_constant_pool(FILE *class_file)
{
    constant_pool_t cp = {
        /* Constant pool count includes unused constant at index 0 */
        .constant_pool_count = read_u2(class_file) - 1,
        .constant_pool =
            malloc(sizeof(const_pool_info) * cp.constant_pool_count),
    };
    assert(cp.constant_pool && "Failed to allocate constant pool");

    const_pool_info *constant = cp.constant_pool;
    for (u2 i = 0; i < cp.constant_pool_count; i++, constant++) {
        constant->tag = read_u1(class_file);
        switch (constant->tag) {
        case CONSTANT_Utf8: {
            u2 length = read_u2(class_file);
            char *value = malloc(length + 1);
            assert(value && "Failed to allocate UTF8 constant");
            size_t bytes_read = fread(value, 1, length, class_file);
            assert(bytes_read == length && "Failed to read UTF8 constant");
            value[length] = '\0';
            constant->info = (u1 *) value;
            break;
        }

        case CONSTANT_Integer: {
            CONSTANT_Integer_info *value = malloc(sizeof(*value));
            assert(value && "Failed to allocate integer constant");
            value->bytes = read_u4(class_file);
            constant->info = (u1 *) value;
            break;
        }

        case CONSTANT_Class: {
            CONSTANT_Class_info *value = malloc(sizeof(*value));
            assert(value && "Failed to allocate class constant");
            value->string_index = read_u2(class_file);
            constant->info = (u1 *) value;
            break;
        }

        case CONSTANT_MethodRef:
        case CONSTANT_FieldRef: {
            CONSTANT_FieldOrMethodRef_info *value = malloc(sizeof(*value));
            assert(value &&
                   "Failed to allocate FieldRef or MethodRef constant");
            value->class_index = read_u2(class_file);
            value->name_and_type_index = read_u2(class_file);
            constant->info = (u1 *) value;
            break;
        }

        case CONSTANT_NameAndType: {
            CONSTANT_NameAndType_info *value = malloc(sizeof(*value));
            assert(value && "Failed to allocate NameAndType constant");
            value->name_index = read_u2(class_file);
            value->descriptor_index = read_u2(class_file);
            constant->info = (u1 *) value;
            break;
        }

        default:
            fprintf(stderr, "Unknown constant type %d\n", constant->tag);
            exit(1);
        }
    }
    return cp;
}

class_info_t get_class_info(FILE *class_file)
{
    class_info_t info = {
        .access_flags = read_u2(class_file),
        .this_class = read_u2(class_file),
        .super_class = read_u2(class_file),
    };
    u2 interfaces_count = read_u2(class_file);
    assert(!interfaces_count && "This VM does not support interfaces.");
    u2 fields_count = read_u2(class_file);
    assert(!fields_count && "This VM does not support fields.");
    return info;
}

void read_method_attributes(FILE *class_file,
                            method_info *info,
                            code_t *code,
                            constant_pool_t *cp)
{
    bool found_code = false;
    for (u2 i = 0; i < info->attributes_count; i++) {
        attribute_info ainfo = {
            .attribute_name_index = read_u2(class_file),
            .attribute_length = read_u4(class_file),
        };
        long attribute_end = ftell(class_file) + ainfo.attribute_length;
        const_pool_info *type_constant =
            get_constant(cp, ainfo.attribute_name_index);
        assert(type_constant->tag == CONSTANT_Utf8 && "Expected a UTF8");
        if (!strcmp((char *) type_constant->info, "Code")) {
            assert(!found_code && "Duplicate method code");
            found_code = true;

            code->max_stack = read_u2(class_file);
            code->max_locals = read_u2(class_file);
            code->code_length = read_u4(class_file);
            code->code = malloc(code->code_length);
            assert(code->code && "Failed to allocate method code");
            size_t bytes_read =
                fread(code->code, 1, code->code_length, class_file);
            assert(bytes_read == code->code_length &&
                   "Failed to read method code");
        }
        /* Skip the rest of the attribute */
        fseek(class_file, attribute_end, SEEK_SET);
    }
    assert(found_code && "Missing method code");
}

#define IS_STATIC 0x0008

method_t *get_methods(FILE *class_file, constant_pool_t *cp)
{
    u2 method_count = read_u2(class_file);
    method_t *methods = malloc(sizeof(*methods) * (method_count + 1));
    assert(methods && "Failed to allocate methods");

    method_t *method = methods;
    for (u2 i = 0; i < method_count; i++, method++) {
        method_info info = {
            .access_flags = read_u2(class_file),
            .name_index = read_u2(class_file),
            .descriptor_index = read_u2(class_file),
            .attributes_count = read_u2(class_file),
        };

        const_pool_info *name = get_constant(cp, info.name_index);
        assert(name->tag == CONSTANT_Utf8 && "Expected a UTF8");
        method->name = (char *) name->info;
        const_pool_info *descriptor = get_constant(cp, info.descriptor_index);
        assert(descriptor->tag == CONSTANT_Utf8 && "Expected a UTF8");
        method->descriptor = (char *) descriptor->info;

        /* FIXME: this VM can only execute static methods, while every class
         * has a constructor method <init>
         */
        if (strcmp(method->name, "<init>"))
            assert((info.access_flags & IS_STATIC) &&
                   "Only static methods are supported by this VM.");

        read_method_attributes(class_file, &info, &method->code, cp);
    }

    /* Mark end of array with NULL name */
    method->name = NULL;
    return methods;
}

/**
 * Read an entire class file.
 * The end of the parsed methods array is marked by a method with a NULL name.
 *
 * @param class_file the open file to read
 * @return the parsed class file
 */
class_file_t get_class(FILE *class_file)
{
    /* Read the leading header of the class file */
    get_class_header(class_file);

    /* Read the constant pool */
    class_file_t clazz = {.constant_pool = get_constant_pool(class_file)};

    /* Read information about the class that was compiled. */
    get_class_info(class_file);

    /* Read the list of static methods */
    clazz.methods = get_methods(class_file, &clazz.constant_pool);
    return clazz;
}

/**
 * Frees the memory used by a parsed class file.
 *
 * @param class the parsed class file
 */
void free_class(class_file_t *clazz)
{
    constant_pool_t *cp = &clazz->constant_pool;
    for (u2 i = 0; i < cp->constant_pool_count; i++)
        free(cp->constant_pool[i].info);
    free(cp->constant_pool);

    for (method_t *method = clazz->methods; method->name; method++)
        free(method->code.code);
    free(clazz->methods);
}