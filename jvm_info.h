#pragma once

#include <stdint.h>
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
    char *name;
    char *descriptor;
    code_t code;
} method_t;

typedef enum {
    CONSTANT_Utf8 = 1,
    CONSTANT_Integer = 3,
    CONSTANT_Class = 7,
    CONSTANT_FieldRef = 9,
    CONSTANT_MethodRef = 10,
    CONSTANT_NameAndType = 12
} const_pool_tag_t;

typedef struct {
    u2 string_index;
} CONSTANT_Class_info;

typedef struct {
    u2 class_index;
    u2 name_and_type_index;
} CONSTANT_FieldOrMethodRef_info;

typedef struct {
    int32_t bytes;
} CONSTANT_Integer_info;

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
    method_t *methods;
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
    i_iload = 0x15,
    i_iload_0 = 0x1a,
    i_iload_1 = 0x1b,
    i_iload_2 = 0x1c,
    i_iload_3 = 0x1d,
    i_istore = 0x36,
    i_istore_0 = 0x3b,
    i_istore_1 = 0x3c,
    i_istore_2 = 0x3d,
    i_istore_3 = 0x3e,
    i_iadd = 0x60,
    i_isub = 0x64,
    i_imul = 0x68,
    i_idiv = 0x6c,
    i_irem = 0x70,
    i_ineg = 0x74,
    i_iinc = 0x84,
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
    i_ireturn = 0xac,
    i_return = 0xb1,
    i_getstatic = 0xb2,
    i_invokevirtual = 0xb6,
    i_invokestatic = 0xb8,
} jvm_opcode_t;

u1 read_u1(FILE *class_file);
u2 read_u2(FILE *class_file);
u4 read_u4(FILE *class_file);
const_pool_info *get_constant(constant_pool_t *constant_pool, u2 index);
CONSTANT_NameAndType_info *get_method_name_and_type(constant_pool_t *cp, u2 idx);
uint16_t get_number_of_parameters(method_t *method);
method_t *find_method(const char *name, const char *desc, class_file_t *clazz);
method_t *find_method_from_index(uint16_t idx, class_file_t *clazz);
class_header_t get_class_header(FILE *class_file);
class_info_t get_class_info(FILE *class_file);
void read_method_attributes(FILE *class_file,
                            method_info *info,
                            code_t *code,
                            constant_pool_t *cp);
method_t *get_methods(FILE *class_file, constant_pool_t *cp);
class_file_t get_class(FILE *class_file);
void free_class(class_file_t *clazz);