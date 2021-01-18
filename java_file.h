#pragma once

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "type.h"

typedef struct {
    u4 magic;
    u2 minor_version;
    u2 major_version;
} class_header_t;

typedef struct {
    u2 access_flags;
    u2 this_class;
    u2 super_class;
} class_info_t;

typedef struct {
    u2 access_flags;
    u2 name_index;
    u2 descriptor_index;
    u2 attributes_count;
} method_info;

typedef method_info field_info;

typedef struct {
    u2 attribute_name_index;
    u4 attribute_length;
} attribute_info;

typedef struct {
    u2 max_stack;
    u2 max_locals;
    u4 code_length;
    u1 *code;
} code_t;

typedef struct {
    u2 bootstrap_method_ref;
    u2 num_bootstrap_arguments;
    u2 *bootstrap_arguments;
} bootstrap_methods_t;

typedef struct {
    u2 attribute_name_index;
    u4 attribute_length;
    u2 num_bootstrap_methods;
    bootstrap_methods_t *bootstrap_methods;
} bootstrapMethods_attribute_t;

typedef struct {
    char *class_name;
    char *name;
    char *descriptor;
    code_t code;
    u2 access_flag;
} method_t;

typedef struct
{
    char *class_name;
    char *name;
    char *descriptor;
    variable_t *value;
} field_t;

typedef enum {
    ACC_PUBLIC = 1,
    ACC_PRIVATE = 2,
    ACC_PROTECTED = 4,
    ACC_STATIC = 8,
    ACC_FINAL = 16,
    ACC_SYNCHRONIZED = 32,
    ACC_BRIDGE = 64,
    ACC_VARARGS = 128,
    ACC_NATIVE = 256,
    ACC_ABSTRACT = 1024,
    ACC_STRICT = 2048,
    ACC_SYNTHETIC = 4096
} method_access_t;

typedef enum {
    CONSTANT_Utf8 = 1,
    CONSTANT_Integer = 3,
    CONSTANT_Long = 5,
    CONSTANT_Class = 7,
    CONSTANT_String = 8,
    CONSTANT_FieldRef = 9,
    CONSTANT_MethodRef = 10,
    CONSTANT_InterfaceMethodref = 11,
    CONSTANT_NameAndType = 12,
    CONSTANT_MethodHandle = 15,
    CONSTANT_InvokeDynamic = 18
} const_pool_tag_t;

typedef struct {
    u2 string_index;
} CONSTANT_Class_info;

typedef struct {
    u2 string_index;
} CONSTANT_String_info;

typedef struct {
    u2 bootstrap_method_attr_index;
    u2 name_and_type_index;
} CONSTANT_InvokeDynamic_info;

typedef struct {
    u1 reference_kind;
    u2 reference_index;
} CONSTANT_MethodHandle_info;


typedef struct {
    u2 class_index;
    u2 name_and_type_index;
} CONSTANT_FieldOrMethodRef_info;

typedef struct {
    u2 class_index;
    u2 name_and_type_index;
} CONSTANT_InterfaceMethodref_info;

typedef struct {
    int32_t bytes;
} CONSTANT_Integer_info;

typedef struct {
    u4 high_bytes;
    u4 low_bytes;
} CONSTANT_LongOrDouble_info;

typedef struct {
    u2 name_index;
    u2 descriptor_index;
} CONSTANT_NameAndType_info;

typedef struct {
    const_pool_tag_t tag;
    u1 *info;
} const_pool_info;

typedef struct {
    u2 constant_pool_count;
    const_pool_info *constant_pool;
} constant_pool_t;

typedef struct {
    constant_pool_t constant_pool;
    // u2 methods_count;
    method_t *methods;
    u2 access_flags;
    u2 this_class;
    u2 super_class;
    u2 interfaces_count;
    u2* interfaces;
    u2 fields_count;
    field_t *fields;
    u2 attributes_count;
    attribute_info* attributes;
    bootstrapMethods_attribute_t *bootstrap;
} class_file_t;

typedef struct
{
    class_file_t *clazz;
    char *path;
    char *name;
} meta_class_t;


typedef enum {
    i_iconst_m1 = 0x2,
    i_iconst_0 = 0x3,
    i_iconst_1 = 0x4,
    i_iconst_2 = 0x5,
    i_iconst_3 = 0x6,
    i_iconst_4 = 0x7,
    i_iconst_5 = 0x8,
    i_bipush = 0x10,
    i_sipush = 0x11,
    i_ldc = 0x12,
    i_ldc2_w = 0x14,
    i_iload = 0x15,
    i_lload = 0x16,
    i_aload = 0x19,
    i_iload_0 = 0x1a,
    i_iload_1 = 0x1b,
    i_iload_2 = 0x1c,
    i_iload_3 = 0x1d,
    i_lload_0 = 0x1e,
    i_lload_1 = 0x1f,
    i_lload_2 = 0x20,
    i_lload_3 = 0x21,
    i_aload_0 = 0x2a,
    i_aload_1 = 0x2b,
    i_aload_2 = 0x2c,
    i_aload_3 = 0x2d,
    i_iaload = 0x2e,
    i_aaload = 0x32,
    i_istore = 0x36,
    i_lstore = 0x37,
    i_astore = 0x3a,
    i_istore_0 = 0x3b,
    i_istore_1 = 0x3c,
    i_istore_2 = 0x3d,
    i_istore_3 = 0x3e,
    i_lstore_0 = 0x3f,
    i_lstore_1 = 0x40,
    i_lstore_2 = 0x41,
    i_lstore_3 = 0x42,
    i_astore_0 = 0x4b,
    i_astore_1 = 0x4c,
    i_astore_2 = 0x4d,
    i_astore_3 = 0x4e,
    i_iastore = 0x4f,
    i_dup = 0x59,
    i_dup2 = 0x5c,
    i_iadd = 0x60,
    i_ladd = 0x61,
    i_isub = 0x64,
    i_lsub = 0x65,
    i_imul = 0x68,
    i_idiv = 0x6c,
    i_lmul = 0x69,
    i_ldiv = 0x6d,
    i_irem = 0x70,
    i_ineg = 0x74,
    i_iinc = 0x84,
    i_i2l = 0x85,
    i_i2c = 0x92,
    i_lcmp = 0x94,
    i_ifeq = 0x99,
    i_ifne = 0x9a,
    i_iflt = 0x9b,
    i_ifge = 0x9c,
    i_ifgt = 0x9d,
    i_ifle = 0x9e,
    i_if_icmpeq = 0x9f,
    i_if_icmpne = 0xa0,
    i_if_icmplt = 0xa1,
    i_if_icmpge = 0xa2,
    i_if_icmpgt = 0xa3,
    i_if_icmple = 0xa4,
    i_goto = 0xa7,
    i_tableswitch = 0xaa,
    i_ireturn = 0xac,
    i_lreturn = 0xad,
    i_areturn = 0xb0,
    i_return = 0xb1,
    i_getstatic = 0xb2,
    i_putstatic = 0xb3,
    i_getfield = 0xb4,
    i_putfield = 0xb5,
    i_invokevirtual = 0xb6,
    i_invokespecial = 0xb7,
    i_invokestatic = 0xb8,
    i_invokedynamic = 0xba,
    i_new = 0xbb,
    i_newarray = 0xbc,
    i_multianewarray = 0xc5,
    i_ifnull = 0xc6
} jvm_opcode_t;


u1 read_u1(FILE *class_file);
u2 read_u2(FILE *class_file);
u4 read_u4(FILE *class_file);
const_pool_info *get_constant(constant_pool_t *constant_pool, u2 index);
CONSTANT_NameAndType_info *get_method_name_and_type(constant_pool_t *cp, u2 idx);
CONSTANT_FieldOrMethodRef_info *get_methodref(constant_pool_t *cp, u2 idx);
CONSTANT_FieldOrMethodRef_info *get_fieldref(constant_pool_t *cp, u2 idx);
uint16_t get_number_of_parameters(method_t *method);
field_t *find_field(const char *name, const char *desc, class_file_t *clazz);
method_t *find_method(const char *name, const char *desc, class_file_t *clazz);
method_t *find_method_from_index(uint16_t idx, class_file_t *clazz, class_file_t *target_clazz);
CONSTANT_Class_info *get_class_name(constant_pool_t *cp, u2 idx);
char *get_string_utf(constant_pool_t *cp, u2 idx);
char *find_field_info_from_index(uint16_t idx, class_file_t *clazz, char **name_info, char **descriptor_info);
char *find_method_info_from_index(uint16_t idx, class_file_t *clazz, char **name_info, char **descriptor_info);
char *find_class_name_from_index(uint16_t idx, class_file_t *clazz);
class_header_t get_class_header(FILE *class_file);
void get_class_info(FILE *class_file, class_file_t *clazz);
void read_field_attributes(FILE *class_file,
                            field_info *info,
                            constant_pool_t *cp);
void read_method_attributes(FILE *class_file,
                            method_info *info,
                            code_t *code,
                            constant_pool_t *cp);
u2 *get_interface(FILE *class_file, constant_pool_t *cp, class_file_t *clazz);
method_t *get_methods(FILE *class_file, constant_pool_t *cp);
field_t *get_fields(FILE *class_file, constant_pool_t *cp, class_file_t *clazz);
bootstrapMethods_attribute_t *read_bootstrap_attribute(FILE *class_file, constant_pool_t *cp);
bootstrap_methods_t *find_bootstrap_method(uint16_t idx, class_file_t *clazz);
class_file_t get_class(FILE *class_file);
void free_class(class_file_t *clazz);