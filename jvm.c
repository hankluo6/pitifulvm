/* PitifulVM is a minimalist Java Virtual Machine implementation written in C.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "jvm_info.h"
#include "stack.h"
#include "class_heap.h"
#include "object_heap.h"
#include "native.h"

/* TODO: add -cp arg to achieve class path select */
char *prefix;

/**
 * Execute the opcode instructions of a method until it returns.
 *
 * @param method the method to run
 * @param locals the array of local variables, including the method parameters.
 *               Except for parameters, the locals are uninitialized.
 * @param clazz the class file the method belongs to
 * @return the method return variable, a heap-allocated pointer, should be free from caller;
 * 
 */
stack_entry_t *execute(method_t *method, local_variable_t *locals, class_file_t *clazz)
{
    code_t code = method->code;
    stack_frame_t *op_stack = malloc(sizeof(stack_frame_t));
    init_stack(op_stack, code.max_stack);

    /* position at the program to be run */
    uint32_t pc = 0;
    uint8_t *code_buf = code.code;

    int loop_count = 0;
    while (pc < code.code_length) {
        loop_count += 1;
        uint8_t current = code_buf[pc];

        /* Reference:
         * https://en.wikipedia.org/wiki/Java_bytecode_instruction_listings
         */
        switch (current) {
        /* Return int from method */
        case i_ireturn: {
            stack_entry_t *ret = malloc(sizeof(variable_t));
            ret->entry.int_value = (int32_t)pop_int(op_stack);
            ret->type = STACK_ENTRY_INT;

            free(op_stack->store);
            free(op_stack);

            return ret;
        } break;

        /* Return void from method */
        case i_return: {
            stack_entry_t *ret = malloc(sizeof(variable_t));
            ret->type = STACK_ENTRY_NONE;
            
            free(op_stack->store);
            free(op_stack);
            
            return ret;
        } break;

        /* Return long from method */
        case i_lreturn: {
            stack_entry_t *ret = malloc(sizeof(variable_t));
            ret->entry.long_value = pop_int(op_stack);
            ret->type = STACK_ENTRY_LONG;

            free(op_stack->store);
            free(op_stack);
            
            return ret;
        } break;

        /* Return reference from method */
        case i_areturn: {
            stack_entry_t *ret = malloc(sizeof(variable_t));
            ret->entry.ptr_value = pop_ref(op_stack);
            ret->type = STACK_ENTRY_REF;

            free(op_stack->store);
            free(op_stack);
            
            return ret;
        } break;

        /* Invoke a class (static) method */
        case i_invokestatic: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);

            /* the method to be called */
            char *method_name, *method_descriptor, *class_name;
            class_name = find_method_info_from_index(index, clazz, &method_name, &method_descriptor);
            class_file_t *target_class = find_class_from_heap(class_name);

            if (!target_class) {
                /* 7 = ".class" + 1 */
                char *tmp = malloc((strlen(class_name) + 7 + strlen(prefix)) * sizeof(char));
                strcpy(tmp, prefix);
                strcat(tmp, class_name);
                /* attempt to read given class file */
                FILE *class_file = fopen(strcat(tmp, ".class"), "r");
                assert(class_file && "Failed to open file");

                /* parse the class file */
                target_class = malloc(sizeof(class_file_t));
                *target_class = get_class(class_file);
                int error = fclose(class_file);
                assert(!error && "Failed to close file");
                add_class(target_class, NULL, tmp);
                free(tmp);
            }

            method_t *own_method = find_method(method_name, method_descriptor, target_class);
            uint16_t num_params = get_number_of_parameters(own_method);
            if (own_method->access_flag & ACC_NATIVE) {
                /* FIXME: locals size must be determined */
                local_variable_t own_locals[20];
                memset(own_locals, 0, sizeof(own_locals));

                for (int i = num_params; i >= 1; i--) {
                    pop_to_local(op_stack, &own_locals[i]);
                }

                /* method return void */
                if (method_descriptor[strlen(method_descriptor) - 1] == 'V') {
                    void_native_method(method, own_locals, target_class);
                /* method return long */
                } else if (method_descriptor[strlen(method_descriptor) - 1] == 'J') {
                    void *exec_res = ptr_native_method(own_method, own_locals, target_class);
                    if (exec_res) {
                        push_long(op_stack, *(int64_t *)exec_res);
                    }
                    free(exec_res);
                /* method return int */
                } else if (method_descriptor[strlen(method_descriptor) - 1] == 'I') {
                    void *exec_res = ptr_native_method(own_method, own_locals, target_class);
                    if (exec_res) {
                        push_int(op_stack, *(int32_t *)exec_res);
                    }
                    free(exec_res);
                /* method return char */
                } else if (method_descriptor[strlen(method_descriptor) - 1] == 'C') {
                    void *exec_res = ptr_native_method(own_method, own_locals, target_class);
                    if (exec_res) {
                        push_byte(op_stack, *(int8_t *)exec_res);
                    }
                    free(exec_res);
                /* method return string */
                } else {
                    void *exec_res = ptr_native_method(own_method, own_locals, target_class);
                    create_string(clazz, (char *)exec_res);
                    free(exec_res);
                }
            }
            else {
                local_variable_t own_locals[own_method->code.max_locals];
                for (int i = num_params - 1; i >= 0; i--) {
                    pop_to_local(op_stack, &own_locals[i]);
                }
                stack_entry_t *exec_res = execute(own_method, own_locals, target_class);
                switch (exec_res->type)
                {
                case STACK_ENTRY_INT: {
                    push_int(op_stack, exec_res->entry.int_value);
                } break;
                case STACK_ENTRY_LONG: {
                    push_long(op_stack, exec_res->entry.long_value);
                } break;
                case STACK_ENTRY_REF: {
                    push_ref(op_stack, exec_res->entry.ptr_value);
                } break;
                case STACK_ENTRY_NONE: {
                    /* nothing */
                } break;
                default: {
                    assert(0 && "unknown return type");
                }
                }
                free(exec_res);
            }
            
            pc += 3;
        } break;

        /* Compare long */
        case i_lcmp: {
            int64_t op1 = pop_int(op_stack), op2 = pop_int(op_stack);
            if (op1 < op2) {
                push_int(op_stack, 1);
            }
            else if (op1 == op2) {
                push_int(op_stack, 0);
            }
            else {
                push_int(op_stack, -1);
            }
            pc += 1;
        } break;

        /* Branch if int comparison with zero succeeds: if equals */
        case i_ifeq: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            int32_t conditional = pop_int(op_stack);
            pc += 3;
            if (conditional == 0) {
                int16_t res = ((param1 << 8) | param2);
                pc += res - 3;
            }
        } break;

        /* Branch if int comparison with zero succeeds: if not equals */
        case i_ifne: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            int32_t conditional = pop_int(op_stack);
            pc += 3;
            if (conditional != 0) {
                int16_t res = ((param1 << 8) | param2);
                pc += res - 3;
            }
        } break;

        /* Branch if int comparison with zero succeeds: if less than 0 */
        case i_iflt: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            int32_t conditional = pop_int(op_stack);
            pc += 3;
            if (conditional < 0) {
                int16_t res = ((param1 << 8) | param2);
                pc += res - 3;
            }
        } break;

        /* Branch if int comparison with zero succeeds: if >= 0 */
        case i_ifge: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            int32_t conditional = pop_int(op_stack);
            pc += 3;
            if (conditional >= 0) {
                int16_t res = ((param1 << 8) | param2);
                pc += res - 3;
            }
        } break;

        /* Branch if int comparison with zero succeeds: if greater than 0 */
        case i_ifgt: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            int32_t conditional = pop_int(op_stack);
            pc += 3;
            if (conditional > 0) {
                int16_t res = ((param1 << 8) | param2);
                pc += res - 3;
            }
        } break;

        /* Branch if int comparison with zero succeeds: if <= 0 */
        case i_ifle: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            int32_t conditional = pop_int(op_stack);
            pc += 3;
            if (conditional <= 0) {
                int16_t res = ((param1 << 8) | param2);
                pc += res - 3;
            }
        } break;

        /* Branch if int comparison succeeds: if equals */
        case i_if_icmpeq: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            int32_t op1 = pop_int(op_stack), op2 = pop_int(op_stack);
            pc += 3;
            if (op2 == op1) {
                int16_t res = ((param1 << 8) | param2);
                pc += res - 3;
            }
        } break;

        /* Branch if int comparison succeeds: if not equals */
        case i_if_icmpne: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            int32_t op1 = pop_int(op_stack), op2 = pop_int(op_stack);
            pc += 3;
            if (op2 != op1) {
                int16_t res = ((param1 << 8) | param2);
                pc += res - 3;
            }
        } break;

        /* Branch if int comparison succeeds: if less than */
        case i_if_icmplt: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            int32_t op1 = pop_int(op_stack), op2 = pop_int(op_stack);
            pc += 3;
            if (op2 < op1) {
                int16_t res = ((param1 << 8) | param2);
                pc += res - 3;
            }
        } break;

        /* Branch if int comparison succeeds: if greater than or equal to */
        case i_if_icmpge: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            int32_t op1 = pop_int(op_stack), op2 = pop_int(op_stack);
            pc += 3;
            if (op2 >= op1) {
                int16_t res = ((param1 << 8) | param2);
                pc += res - 3;
            }
        } break;

        /* Branch if int comparison succeeds: if greater than */
        case i_if_icmpgt: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            int32_t op1 = pop_int(op_stack), op2 = pop_int(op_stack);
            pc += 3;
            if (op2 > op1) {
                int16_t res = ((param1 << 8) | param2);
                pc += res - 3;
            }
        } break;

        /* Branch if int comparison succeeds: if less than or equal to */
        case i_if_icmple: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            int32_t op1 = pop_int(op_stack), op2 = pop_int(op_stack);
            pc += 3;
            if (op2 <= op1) {
                int16_t res = ((param1 << 8) | param2);
                pc += res - 3;
            }
        } break;

        /* Branch if reference is null */
        case i_ifnull: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            void *addr = pop_ref(op_stack);
            pc += 3;
            if (addr == NULL) {
                int16_t res = ((param1 << 8) | param2);
                pc += res - 3;
            }
        } break;

        /* Branch always */
        case i_goto: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            int16_t res = ((param1 << 8) | param2);
            pc += res;
        } break;

        /* Push item from run-time constant pool */
        case i_ldc: {
            constant_pool_t constant_pool = clazz->constant_pool;
            int16_t param = code_buf[pc + 1];

            /* get the constant */
            const_pool_info *info = get_constant(&constant_pool, param);
            switch (info->tag)
            {
            case CONSTANT_Integer: {
                push_int(op_stack, ((CONSTANT_Integer_info *) info->info)->bytes);
                break;
            }
            case CONSTANT_String: {
                char *src = (char *)get_constant(&constant_pool, ((CONSTANT_String_info *) info->info)->string_index)->info;
                char *dest = create_string(clazz, src);
                push_ref(op_stack, dest);
                break;
            }
            default:
                fprintf(stderr, "ldc only support int and string, but get tag %d\n", info->tag);
                exit(1);
            }
            pc += 2;
        } break;

        /* Push long or double from run-time constant pool (wide index) */
        case i_ldc2_w: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);

            uint64_t high = ((CONSTANT_LongOrDouble_info *)get_constant(&clazz->constant_pool, index)->info)->high_bytes;
            uint64_t low = ((CONSTANT_LongOrDouble_info *)get_constant(&clazz->constant_pool, index)->info)->low_bytes;
            int64_t value = high << 32 | low;
            push_long(op_stack, value);
            pc += 3;
        } break;

        /* Load int from local variable */
        case i_iload_0:
        case i_iload_1:
        case i_iload_2:
        case i_iload_3: {
            int32_t param = current - i_iload_0;
            int32_t loaded;

            //memcpy(&loaded, locals[param].entry.val, sizeof(int32_t));
            loaded = locals[param].entry.int_value;
            push_int(op_stack, loaded);
            pc += 1;
        } break;

        /* Load long from local variable */
        case i_lload: {
            int32_t param = code_buf[pc + 1];
            int64_t loaded;
            
            //memcpy(&loaded, locals[param].entry.val, sizeof(int64_t));
            loaded = locals[param].entry.long_value;
            push_long(op_stack, loaded);

            pc += 2;
        } break;

        /* Load long from local variable */
        case i_lload_0:
        case i_lload_1:
        case i_lload_2:
        case i_lload_3: {
            int64_t param = current - i_lload_0;
            int64_t loaded;

            //memcpy(&loaded, locals[param].entry.val, sizeof(uint64_t));
            loaded = locals[param].entry.long_value;
            push_long(op_stack, loaded);

            pc += 1;
        } break;

        /* Load int from local variable */
        case i_iload: {
            int32_t param = code_buf[pc + 1];
            int32_t loaded;
            
            //memcpy(&loaded, locals[param].entry.val, sizeof(int32_t));
            loaded = locals[param].entry.int_value;
            push_int(op_stack, loaded);

            pc += 2;
        } break;

        /* Store int into local variable */
        case i_istore: {
            int32_t param = code_buf[pc + 1];
            int32_t stored = pop_int(op_stack);
            //memcpy(locals[param].entry.val, &stored, sizeof(int32_t));
            locals[param].entry.int_value = stored;
            locals[param].type = STACK_ENTRY_INT;

            pc += 2;
        } break;

        /* Store int into local variable */
        case i_istore_0:
        case i_istore_1:
        case i_istore_2:
        case i_istore_3: {
            int32_t param = current - i_istore_0;
            int32_t stored = pop_int(op_stack);
            //memcpy(locals[param].entry.val, &stored, sizeof(int32_t));
            locals[param].entry.int_value = stored;
            locals[param].type = STACK_ENTRY_INT;

            pc += 1;
        } break;

        /* Store long into local variable */
        case i_lstore: {
            int32_t param = code_buf[pc + 1];
            int64_t stored = pop_int(op_stack);
            //memcpy(locals[param].entry.val, &stored, sizeof(int64_t));
            locals[param].entry.long_value = stored;
            locals[param].type = STACK_ENTRY_LONG;

            pc += 2;
        } break;

        /* Store long into local variable */
        case i_lstore_0:
        case i_lstore_1:
        case i_lstore_2:
        case i_lstore_3: {
            int32_t param = current - i_lstore_0;
            int64_t stored = pop_int(op_stack);
            //memcpy(locals[param].entry.val, &stored, sizeof(int64_t));
            locals[param].entry.long_value = stored;
            locals[param].type = STACK_ENTRY_LONG;

            pc += 1;
        } break;

        /* Increment local variable by constant */
        case i_iinc: {
            uint8_t i = code_buf[pc + 1];
            int8_t b = code_buf[pc + 2]; /* signed value */
            int32_t value;
            //memcpy(&value, locals[i].entry.val, sizeof(int32_t));
            locals[i].entry.int_value += b;
            //value += b;
            //memcpy(locals[i].entry.val, &value, sizeof(int32_t));
            //locals[i].entry.int_value = value;
            pc += 3;
        } break;

        /* Convert int to long */
        case i_i2l: {
            int32_t stored = pop_int(op_stack);
            push_long(op_stack, (int64_t) stored);

            pc += 1;
        } break;

        /* Convert int to char */
        case i_i2c: {
            int32_t stored = pop_int(op_stack);
            push_byte(op_stack, (int8_t) stored);

            pc += 1;
        } break;

        /* Push byte */
        case i_bipush: {
            int8_t param = code_buf[pc + 1];
            push_byte(op_stack, param);

            pc += 2;
        } break;

        /* Add int */
        case i_iadd: {
            int32_t op1 = pop_int(op_stack);
            int32_t op2 = pop_int(op_stack);
            push_int(op_stack, op1 + op2);

            pc += 1;
        } break;

        /* Subtract int */
        case i_isub: {
            int32_t op1 = pop_int(op_stack);
            int32_t op2 = pop_int(op_stack);
            push_int(op_stack, op2 - op1);

            pc += 1;
        } break;

        /* Multiply int */
        case i_imul: {
            int32_t op1 = pop_int(op_stack);
            int32_t op2 = pop_int(op_stack);
            push_int(op_stack, op1 * op2);

            pc += 1;
        } break;

        /* Divide int */
        case i_idiv: {
            int32_t op1 = pop_int(op_stack);
            int32_t op2 = pop_int(op_stack);
            push_int(op_stack, op2 / op1);

            pc += 1;
        } break;

        /* Remainder int */
        case i_irem: {
            int32_t op1 = pop_int(op_stack);
            int32_t op2 = pop_int(op_stack);
            push_int(op_stack, op2 % op1);

            pc += 1;
        } break;

        /* Negate int */
        case i_ineg: {
            int32_t op1 = pop_int(op_stack);
            push_int(op_stack, -op1);

            pc += 1;
        } break;

        /* Add long */
        case i_ladd: {
            int64_t op1 = pop_int(op_stack);
            int64_t op2 = pop_int(op_stack);
            
            push_long(op_stack, op1 + op2);
            pc += 1;
        } break;

        /* Subtract long */
        case i_lsub: {
            int64_t op1 = pop_int(op_stack);
            int64_t op2 = pop_int(op_stack);
            
            push_long(op_stack, op2 - op1);
            pc += 1;
        } break;

        /* Multiply long */
        case i_lmul: {
            int64_t op1 = pop_int(op_stack);
            int64_t op2 = pop_int(op_stack);
            
            push_long(op_stack, op1 * op2);
            pc += 1;
        } break;

        /* Divide long */
        case i_ldiv: {
            int64_t op1 = pop_int(op_stack);
            int64_t op2 = pop_int(op_stack);
            
            push_long(op_stack, op2 / op1);
            pc += 1;
        } break;

        /* Load reference from array */
        case i_aaload: {
            int32_t index = pop_int(op_stack);
            /* currently only support two dimension integer array */
            int32_t **addr = pop_ref(op_stack);

            push_ref(op_stack, addr[index]);
            pc += 1;
        } break;

        /* Access jump table by index and jump */
        case i_tableswitch: {
            int32_t base = pc;

            pc += 3;
            pc -= (pc % 4);

            uint8_t defaultbyte1 = code_buf[pc], defaultbyte2 = code_buf[pc + 1], defaultbyte3 = code_buf[pc + 2], defaultbyte4 = code_buf[pc + 3];
            int32_t _default = ((defaultbyte1 << 24) | (defaultbyte2 << 16) | (defaultbyte3 << 8) | defaultbyte4);
            pc += 4;
            uint8_t lowbyte1 = code_buf[pc], lowbyte2 = code_buf[pc + 1], lowbyte3 = code_buf[pc + 2], lowbyte4 = code_buf[pc + 3];
            int32_t low = ((lowbyte1 << 24) | (lowbyte2 << 16) | (lowbyte3 << 8) | lowbyte4);
            pc += 4;
            uint8_t highbyte1 = code_buf[pc + 1], highbyte2 = code_buf[pc + 1], highbyte3 = code_buf[pc + 2], highbyte4 = code_buf[pc + 3];
            int32_t high = ((highbyte1 << 24) | (highbyte2 << 16) | (highbyte3 << 8) | highbyte4);
            pc += 4;

            int32_t key = pop_int(op_stack);
            if (key >= low && key <= high) {
                unsigned index = pc + ((key - low) * 4);
                uint8_t byte1 = code_buf[index], byte2 = code_buf[index + 1], byte3 = code_buf[index + 2], byte4 = code_buf[index + 3];
                uint32_t addr = ((byte1 << 24) | (byte2 << 16) | (byte3 << 8) | byte4);
                pc = base + addr;
            } 
            else {
                pc = base + _default;
            }
        } break;

        /* Get static field from class */
        case i_getstatic: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);
            
            char *field_name, *field_descriptor, *class_name;
            class_name = find_field_info_from_index(index, clazz, &field_name, &field_descriptor);

            class_file_t *new_clazz = find_class_from_heap(class_name);

            /* if not found, add specify class path. this can be remove by record path infomation in class heap */
            if (!new_clazz) {
                char *tmp = malloc(strlen(class_name) + strlen(prefix) + 1);
                strcpy(tmp, prefix);
                strcat(tmp, class_name);
                new_clazz = find_class_from_heap(tmp);

                free(tmp);
            }

            if (!new_clazz) {
                char *tmp = malloc((strlen(class_name) + 7 + strlen(prefix)) * sizeof(char));
                strcpy(tmp, prefix);
                strcat(tmp, class_name);
                /* attempt to read given class file */
                FILE *class_file = fopen(strcat(tmp, ".class"), "r");
                assert(class_file && "Failed to open file");

                /* parse the class file */
                new_clazz = malloc(sizeof(class_file_t));
                *new_clazz = get_class(class_file);
                
                int error = fclose(class_file);
                assert(!error && "Failed to close file");
                add_class(new_clazz, NULL, tmp);

                method_t *method = find_method("<clinit>", "()V", new_clazz);
                if (method) {
                    local_variable_t own_locals[method->code.max_locals];
                    stack_entry_t *exec_res = execute(method, own_locals, new_clazz);
                    assert(exec_res->type == STACK_ENTRY_NONE && "<clinit> must be no return");
                    free(exec_res);
                }
                free(tmp);
            }

            field_t *field = find_field(field_name, field_descriptor, new_clazz);
            
            /* find super class */
            while (!field) {
                char *super_name = find_class_name_from_index(clazz->super_class, clazz);
                assert(super_name && "cannot find field");
                new_clazz = find_class_from_heap(super_name);
                if (!new_clazz) {
                    char *tmp = malloc(strlen(super_name) + strlen(prefix) + 1);
                    strcpy(tmp, prefix);
                    strcat(tmp, super_name);
                    new_clazz = find_class_from_heap(tmp);
                    free(tmp);
                }
                field = find_field(field_name, field_descriptor, new_clazz);
            }

            switch (field_descriptor[0])
            {
            case 'B':
                /* signed byte */
                push_byte(op_stack, field->value->value.char_value);
                break;
            case 'C':
                /* Unicode character code point in the Basic Multilingual Plane, encoded with UTF-16 */
                push_byte(op_stack, field->value->value.char_value);
                break;
            case 'D':
                /* double-precision floating-point value */
                break;
            case 'F':
                /* single-precision floating-point value */
                break;
            case 'I':
                /* integer */
                push_int(op_stack, field->value->value.int_value);
                break;
            case 'J':
                /* long integer */
                break;
            case 'S':
                /* signed short */
                push_short(op_stack, field->value->value.short_value);
                break;
            case 'Z':
                /* true or false */
                break;
            case 'L':
                /* an instance of class ClassName */
                push_ref(op_stack, field->value->value.ptr_value);
                break;
            case '[': {
                if (field_descriptor[1] != '[') {
                    /* one array dimension */
                    push_ref(op_stack, field->value->value.ptr_value);
                }
                else if (field_descriptor[1] == '[') {
                    /* two array dimension */
                    push_ref(op_stack, field->value->value.ptr_value);
                }
                else {
                    assert(0 && "only support two dimension array");
                }
                break;
            }
            default:
                fprintf(stderr, "Unknown field descriptor %c\n", field_descriptor[0]);
                exit(1);
            }

            pc += 3;

        } break;

        /* Put static field to class */
        case i_putstatic: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);
            
            char *field_name, *field_descriptor, *class_name;
            class_name = find_field_info_from_index(index, clazz, &field_name, &field_descriptor);

            class_file_t *target_class = find_class_from_heap(class_name);

            /* if not found, add specify class path. this can be remove by record path infomation in class heap */
            if (!target_class) {
                char *tmp = malloc(strlen(class_name) + strlen(prefix) + 1);
                strcpy(tmp, prefix);
                strcat(tmp, class_name);
                target_class = find_class_from_heap(tmp);

                free(tmp);
            }

            if (!target_class) {
                char *tmp = malloc((strlen(class_name) + 7 + strlen(prefix)) * sizeof(char));
                strcpy(tmp, prefix);
                strcat(tmp, class_name);
                /* attempt to read given class file */
                FILE *class_file = fopen(strcat(tmp, ".class"), "r");
                assert(class_file && "Failed to open file");

                /* parse the class file */
                target_class = malloc(sizeof(class_file_t));
                *target_class = get_class(class_file);
                
                int error = fclose(class_file);
                assert(!error && "Failed to close file");
                add_class(target_class, NULL, tmp);

                method_t *method = find_method("<clinit>", "()V", target_class);
                if (method) {
                    local_variable_t own_locals[method->code.max_locals];
                    stack_entry_t *exec_res = execute(method, own_locals, target_class);
                    assert(exec_res->type == STACK_ENTRY_NONE && "<clinit> must be no return");
                    free(exec_res);
                }
                free(tmp);
            }

            field_t *field = find_field(field_name, field_descriptor, target_class);
            
            /* find super class */
            while (!field) {
                char *super_name = find_class_name_from_index(clazz->super_class, clazz);
                assert(super_name && "cannot find field");
                target_class = find_class_from_heap(super_name);
                if (!target_class) {
                    char *tmp = malloc(strlen(super_name) + strlen(prefix) + 1);
                    strcpy(tmp, prefix);
                    strcat(tmp, super_name);
                    target_class = find_class_from_heap(tmp);
                    free(tmp);
                }
                field = find_field(field_name, field_descriptor, target_class);
            }

            switch (field_descriptor[0])
            {
            case 'B': 
                /* signed byte */
                field->value->value.char_value = pop_int(op_stack);
                field->value->type = VAR_BYTE;
                break;
            case 'C': 
                /* Unicode character code point in the Basic Multilingual Plane, encoded with UTF-16 */
                field->value->value.char_value = pop_int(op_stack);
                field->value->type = VAR_BYTE;
                break;
            case 'D':
                /* double-precision floating-point value */
                break;
            case 'F':
                /* single-precision floating-point value */
                break;
            case 'I': 
                /* integer */
                field->value->value.int_value = pop_int(op_stack);
                field->value->type = VAR_INT;
                break;
            case 'J':
                /* long integer */
                break;
            case 'S':
                /* signed short */
                field->value->value.short_value = pop_int(op_stack);
                field->value->type = VAR_SHORT;
                break;
            case 'Z':
                /* true or false */
                field->value->value.char_value = pop_int(op_stack);
                field->value->type = VAR_BYTE;
                break;
            case 'L':
                /* an instance of class ClassName */
                if (strcmp(field_descriptor, "Ljava/lang/String;") == 0) {
                    field->value->value.ptr_value = pop_ref(op_stack);
                    field->value->type = VAR_STR_PTR;
                }
                else {
                    field->value->value.ptr_value = pop_ref(op_stack);
                    field->value->type = VAR_PTR;
                }
                break;
            case '[': {
                if (field_descriptor[1] != '[') {
                    /* one array dimension */
                    field->value->value.ptr_value = pop_ref(op_stack);
                    field->value->type = VAR_ARRAY_PTR;
                }
                else if (field_descriptor[1] == '[') {
                    /* two array dimension */
                    field->value->value.ptr_value = pop_ref(op_stack);
                    field->value->type = VAR_MULTARRAY_PTR;
                }
                else {
                    assert(0 && "only support tow dimension array");
                }
                break;
            }
            default:
                fprintf(stderr, "Unknown field descriptor %c\n", field_descriptor[0]);
                exit(1);
            }
            
            pc += 3;
        } break;

        /* Invoke instance method; dispatch based on class */
        case i_invokevirtual: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);

            /* the method to be called */
            char *method_name, *method_descriptor, *class_name;
            class_name = find_method_info_from_index(index, clazz, &method_name, &method_descriptor);
            class_file_t *target_class = find_class_from_heap(class_name);

            if (target_class == NULL) {
                /* not support super class method */
                if (strcmp(class_name, "java/lang/Object") == 0) {
                    /* pop_ref(op_stack); */
                    pc += 3;
                    break;
                }
                /* to handle print method */
                if (strcmp(class_name, "java/io/PrintStream") == 0) {
                    /* don't pop object reference because we don't implement get_static yet */
                    /* pop_ref(op_stack); */
                    /* FIXME: the implement is not correct. */
                    int32_t op = pop_int(op_stack);

                    printf("%d\n", op);
                    pc += 3;
                    break;
                }

                char *tmp = malloc((strlen(class_name) + 7 + strlen(prefix)) * sizeof(char));
                strcpy(tmp, prefix);
                strcat(tmp, class_name);
                /* attempt to read given class file */
                FILE *class_file = fopen(strcat(tmp, ".class"), "r");
                assert(class_file && "Failed to open file");

                /* parse the class file */
                target_class = malloc(sizeof(class_file_t));
                *target_class = get_class(class_file);

                int error = fclose(class_file);
                assert(!error && "Failed to close file");
                add_class(target_class, NULL, tmp);
                free(tmp);
            }

            method_t *method = find_method(method_name, method_descriptor, target_class);
            uint16_t num_params = get_number_of_parameters(method);
            local_variable_t own_locals[method->code.max_locals];
            for (int i = num_params; i >= 1; i--) {
                pop_to_local(op_stack, &own_locals[i]);
            }
            object_t *obj = pop_ref(op_stack);
            /* first argument is this pointer */
            own_locals[0].entry.ptr_value = obj;
            own_locals[0].type = STACK_ENTRY_REF;

            /* FIXME: virtual method is related to object and its field */
            int32_t *exec_res = execute(method, own_locals, obj->type);
            if (exec_res) {
                push_int(op_stack, *exec_res);
            }
            free(exec_res);
            pc += 3;
        } break;

        /* Push int constant */
        case i_iconst_m1:
        case i_iconst_0:
        case i_iconst_1:
        case i_iconst_2:
        case i_iconst_3:
        case i_iconst_4:
        case i_iconst_5: {
            push_int(op_stack, current - i_iconst_0);
            pc += 1;
        } break;

        /* Load reference from local variable */
        case i_aload: {
            int32_t param = code_buf[pc + 1];
            object_t *obj = locals[param].entry.ptr_value;
            push_ref(op_stack, obj);

            pc += 2;
        } break;

        /* Load reference from local variable */
        case i_aload_0:
        case i_aload_1:
        case i_aload_2:
        case i_aload_3: {
            int32_t param = current - i_aload_0;
            object_t *obj = locals[param].entry.ptr_value;
            push_ref(op_stack, obj);

            pc += 1;
        } break;
        
        /* Store reference into local variable */
        case i_astore: {
            int32_t param = code_buf[pc + 1];
            locals[param].entry.ptr_value = pop_ref(op_stack);
            locals[param].type = STACK_ENTRY_REF;

            pc += 2;
        } break;

        /* Store reference into local variable */
        case i_astore_0:
        case i_astore_1:
        case i_astore_2:
        case i_astore_3: {
            int32_t param = current - i_astore_0;
            locals[param].entry.ptr_value = pop_ref(op_stack);
            locals[param].type = STACK_ENTRY_REF;

            pc += 1;
        } break;

        /* Push short */
        case i_sipush: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            int16_t res = ((param1 << 8) | param2);
            push_short(op_stack, res);

            pc += 3;
        } break;

        /* Fetch field from object */
        case i_getfield: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);

            object_t *obj = pop_ref(op_stack);
            char *field_name, *field_descriptor, *class_name;
            class_name = find_field_info_from_index(index, clazz, &field_name, &field_descriptor);
            variable_t *addr = find_field_addr(obj, field_name);
            
            /* FIXME: if not found, should find field from parent */
            switch (field_descriptor[0])
            {
            case 'I': {
                push_int(op_stack, addr->value.int_value);
            } break;
            case 'J': {
                push_long(op_stack, addr->value.long_value);
            } break;
            case 'L': {
                push_ref(op_stack, addr->value.ptr_value);
            } break;
            default:
                assert(0 && "Only support integer, long and reference field");
                break;
            }
            pc += 3;
        } break;

        /* Set field in object */
        case i_putfield: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);
            
            stack_entry_t element = top(op_stack);

            int64_t value = 0;
            void *addr = NULL;
            switch (element.type)
            {
            /* integer */
            case STACK_ENTRY_INT: 
            case STACK_ENTRY_SHORT: 
            case STACK_ENTRY_BYTE: 
            case STACK_ENTRY_LONG: {
                value = pop_int(op_stack);
                break;
            }
            case STACK_ENTRY_REF: {
                addr = pop_ref(op_stack);
                break;
            }
            default: {
                assert(0 && "only support integer and reference field");
                break;
            }
            }
            object_t *obj = pop_ref(op_stack);

            char *field_name, *field_descriptor, *class_name;
            class_name = find_field_info_from_index(index, clazz, &field_name, &field_descriptor);

            /* FIXME: if not found, should find field from parent */
            switch (field_descriptor[0])
            {
            case 'I': {
                variable_t *var = find_field_addr(obj, field_name);
                var->value.int_value = (int32_t)value;
                var->type = VAR_INT;
            } break;
            case 'J': {
                variable_t *var = find_field_addr(obj, field_name);
                var->value.long_value = value;
                var->type = VAR_LONG;
            } break;
            case 'L': {
                if (strcmp(field_descriptor, "Ljava/lang/String;") == 0) {
                    variable_t *var = find_field_addr(obj, field_name);
                    var->value.ptr_value = addr;
                    var->type = VAR_STR_PTR;
                }
                else {
                    variable_t *var = find_field_addr(obj, field_name);
                    var->value.ptr_value = addr;
                    var->type = VAR_PTR;
                }
            } break;
            default:
                assert(0 && "Only support integer, long and reference field");
                break;
            }
            pc += 3;
        } break;

        /* Create new object */
        case i_new: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);

            char *class_name = find_class_name_from_index(index, clazz);
            class_file_t *new_class = find_class_from_heap(class_name);

            /* if not found, add specify class path. this can be remove by record path infomation in class heap */
            if (!new_class) {
                char *tmp = malloc(strlen(class_name) + strlen(prefix) + 1);
                strcpy(tmp, prefix);
                strcat(tmp, class_name);
                new_class = find_class_from_heap(tmp);

                free(tmp);
            }

            if (!new_class) {
                char *tmp = malloc((strlen(class_name) + 7 + strlen(prefix)) * sizeof(char));
                strcpy(tmp, prefix);
                strcat(tmp, class_name);
                /* attempt to read given class file */
                FILE *class_file = fopen(strcat(tmp, ".class"), "r");
                assert(class_file && "Failed to open file");

                /* parse the class file */
                new_class = malloc(sizeof(class_file_t));
                *new_class = get_class(class_file);

                int error = fclose(class_file);
                assert(!error && "Failed to close file");
                add_class(new_class, NULL, tmp);

                method_t *method = find_method("<clinit>", "()V", new_class);
                if (method) {
                    local_variable_t own_locals[method->code.max_locals];
                    stack_entry_t *exec_res = execute(method, own_locals, new_class);
                    assert(exec_res->type == STACK_ENTRY_NONE && "<clinit> must be no return");
                    free(exec_res);
                }

                free(tmp);
            }

            object_t *object = create_object(new_class);
            push_ref(op_stack, object);

            pc += 3;
        } break;

        /* Duplicate the top operand stack value */
        case i_dup: {
            op_stack->store[op_stack->size] = op_stack->store[op_stack->size - 1];
            op_stack->size++;
            pc += 1;
        } break;

        /* Duplicate the top one or two operand stack values */
        case i_dup2: {
            stack_entry_t element = top(op_stack);
            /* category 2 */
            if (element.type == STACK_ENTRY_LONG || element.type == STACK_ENTRY_DOUBLE) {
                op_stack->store[op_stack->size] = op_stack->store[op_stack->size - 1];
                op_stack->size++;
                pc += 1;
            }
            /* category 1 */
            else {
                op_stack->store[op_stack->size] = op_stack->store[op_stack->size - 2];
                op_stack->store[op_stack->size + 1] = op_stack->store[op_stack->size - 1];
                op_stack->size += 2;
                pc += 1;
            }
        } break;

        /* Invoke dynamic method */
        case i_invokedynamic: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);
            
            bootstrap_methods_t *bootstrap_method = find_bootstrap_method(index, clazz);
        
            char *arg = NULL;
            /* we only support makeConcatWithConstants (string concatenation), this mean argument must be only one string */
            /* FIXME: if string contain "\0", it will cause error */
            assert(bootstrap_method->num_bootstrap_arguments == 1 && "only support makeConcatWithConstants");
            for (int i = 0; i < bootstrap_method->num_bootstrap_arguments; ++i) {
                arg = get_string_utf(&clazz->constant_pool, bootstrap_method->bootstrap_arguments[i]);
            }

            /* each \u0001 mean "1" in unicode, this should be replace by actual argument from stack, other are constant characters */
            uint16_t num_params = 0;
            uint16_t num_constant = 0;
            char *tmp = arg;
            while (*tmp != '\0') {
                if (*(tmp++) == 1) {
                    num_params++;
                }
            }
            num_constant = strlen(arg) - num_params;
            char **all_string = malloc(sizeof(char *) * num_params);
            size_t max_len = 0;
            for (int i = 0; i < num_params; i++) {
                stack_entry_t element = top(op_stack);
                int64_t value = 0;
                void *addr = NULL;
                switch (element.type)
                {
                /* integer */
                case STACK_ENTRY_INT: 
                case STACK_ENTRY_SHORT: 
                case STACK_ENTRY_BYTE: 
                case STACK_ENTRY_LONG: {
                    int64_t value = pop_int(op_stack);
                    char str[50];
                    //lld?
                    sprintf(str, "%ld", value);
                    char *dest = create_string(clazz, str);
                    all_string[i] = dest;
                    break;
                }
                case STACK_ENTRY_REF: {
                    all_string[i] = (char *)pop_ref(op_stack);
                    break;
                }
                default: {
                    printf("unknown stack top type (%d)\n", element.type);
                    break;
                }
                }
                max_len += strlen(all_string[i]);
            }
            max_len += num_constant;
            char *new_str = calloc((max_len + 1), sizeof(char));

            tmp = arg;
            while (*tmp != '\0') {
                if (*tmp == 1) {
                    strcat(new_str, all_string[--num_params]);
                }
                else {
                    strncat(new_str, tmp, 1);
                }
                tmp++;
            }
            new_str[max_len] = '\0';
            char *dest = create_string(clazz, new_str);
            push_ref(op_stack, dest);
            free(all_string);
            free(new_str);
            
            pc += 5;

        } break;

        /* Invoke instance method; special handling for superclass, private, and instance initialization method invocations */
        case i_invokespecial: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);

            /* the method to be called */
            char *method_name, *method_descriptor, *class_name;
            class_name = find_method_info_from_index(index, clazz, &method_name, &method_descriptor);
            class_file_t *target_class = find_class_from_heap(class_name);

            /* if not found, add specify class path. this can be remove by record path infomation in class heap */
            if (!target_class) {
                char *tmp = malloc(strlen(class_name) + strlen(prefix) + 1);
                strcpy(tmp, prefix);
                strcat(tmp, class_name);
                target_class = find_class_from_heap(tmp);

                free(tmp);
            }

            if (!target_class) {
                char *tmp = malloc((strlen(class_name) + 7 + strlen(prefix)) * sizeof(char));
                strcpy(tmp, prefix);
                strcat(tmp, class_name);
                /* attempt to read given class file */
                FILE *class_file = fopen(strcat(tmp, ".class"), "r");
                assert(class_file && "Failed to open file");

                /* parse the class file */
                target_class = malloc(sizeof(class_file_t));
                *target_class = get_class(class_file);

                int error = fclose(class_file);
                assert(!error && "Failed to close file");
                add_class(target_class, NULL, tmp);

                method_t *method = find_method("<clinit>", "()V", target_class);
                if (method) {
                    local_variable_t own_locals[method->code.max_locals];
                    stack_entry_t *exec_res = execute(method, own_locals, target_class);
                    assert(exec_res->type == STACK_ENTRY_NONE && "<clinit> must be no return");
                    free(exec_res);
                }

                free(tmp);
            }

            /* constructor */
            method_t *constructor = find_method(method_name, method_descriptor, target_class);
            uint16_t num_params = get_number_of_parameters(constructor);
            local_variable_t own_locals[constructor->code.max_locals];
            for (int i = num_params; i >= 1; i--) {
                pop_to_local(op_stack, &own_locals[i]);
            }
            object_t *obj = pop_ref(op_stack);

            /* first argument is this pointer */
            own_locals[0].entry.ptr_value = obj;
            own_locals[0].type = STACK_ENTRY_REF;

            stack_entry_t *exec_res = execute(constructor, own_locals, target_class);
            assert(exec_res->type == STACK_ENTRY_NONE && "constructor must be no return");
            free(exec_res);
            
            pc += 3;
        } break;

        /* Invoke instance method; dispatch based on class */
        case i_invokevirtual: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);

            /* the method to be called */
            char *method_name, *method_descriptor, *class_name;
            class_name = find_method_info_from_index(index, clazz, &method_name, &method_descriptor);
            class_file_t *target_class = find_class_from_heap(class_name);

            /* if not found, add specify class path. this can be remove by record path infomation in class heap */
            if (!target_class) {
                char *tmp = malloc(strlen(class_name) + strlen(prefix) + 1);
                strcpy(tmp, prefix);
                strcat(tmp, class_name);
                target_class = find_class_from_heap(tmp);

                free(tmp);
            }

            if (!target_class) {

                char *tmp = malloc((strlen(class_name) + 7 + strlen(prefix)) * sizeof(char));
                strcpy(tmp, prefix);
                strcat(tmp, class_name);
                /* attempt to read given class file */
                FILE *class_file = fopen(strcat(tmp, ".class"), "r");
                assert(class_file && "Failed to open file");

                /* parse the class file */
                target_class = malloc(sizeof(class_file_t));
                *target_class = get_class(class_file);
                
                int error = fclose(class_file);
                assert(!error && "Failed to close file");
                add_class(target_class, NULL, tmp);

                method_t *method = find_method("<clinit>", "()V", target_class);
                if (method) {
                    local_variable_t own_locals[method->code.max_locals];
                    stack_entry_t *exec_res = execute(method, own_locals, target_class);
                    assert(exec_res->type == STACK_ENTRY_NONE && "<clinit> must be no return");
                    free(exec_res);
                }

                free(tmp);
            }

            method_t *method = find_method(method_name, method_descriptor, target_class);
            uint16_t num_params;
            if (method->access_flag & ACC_NATIVE) {
                num_params = get_number_of_parameters(method);
                /* FIXME: only support max 20 stack */
                local_variable_t own_locals[20];
                memset(own_locals, 0, sizeof(own_locals));
                for (int i = num_params; i >= 1; i--) {
                    pop_to_local(op_stack, &own_locals[i]);
                }
                object_t *obj = pop_ref(op_stack);

                /* first argument is this pointer */
                own_locals[0].entry.ptr_value = obj;
                own_locals[0].type = STACK_ENTRY_REF;

                /* method return void */
                if (method_descriptor[strlen(method_descriptor) - 1] == 'V') {
                    void_native_method(method, own_locals, target_class);
                } else if (method_descriptor[strlen(method_descriptor) - 1] == 'J' ) {
                    void *exec_res = ptr_native_method(method, own_locals, target_class);
                    if (exec_res) {
                        push_long(op_stack, *(int64_t *)exec_res);
                    }
                    free(exec_res);
                } else if (method_descriptor[strlen(method_descriptor) - 1] == 'C' || method_descriptor[strlen(method_descriptor) - 1] == 'I') {
                    void *exec_res = ptr_native_method(method, own_locals, target_class);
                    if (exec_res) {
                        push_int(op_stack, *(int32_t *)exec_res);
                    }
                    free(exec_res);
                } else {
                    void *exec_res = ptr_native_method(method, own_locals, target_class);
                    char *new_str = create_string(clazz, (char *)exec_res);
                    push_ref(op_stack, new_str);
                    free(exec_res);
                }
            }
            else {
                num_params = get_number_of_parameters(method);
                local_variable_t own_locals[method->code.max_locals];
                memset(own_locals, 0, sizeof(own_locals));
                for (int i = num_params; i >= 1; i--) {
                    pop_to_local(op_stack, &own_locals[i]);
                }
                object_t *obj = pop_ref(op_stack);

                /* first argument is this pointer */
                own_locals[0].entry.ptr_value = obj;
                own_locals[0].type = STACK_ENTRY_REF;

                stack_entry_t *exec_res = execute(method, own_locals, target_class);
                switch (exec_res->type)
                {
                case STACK_ENTRY_INT: {
                    push_int(op_stack, exec_res->entry.int_value);
                } break;
                case STACK_ENTRY_LONG: {
                    push_long(op_stack, exec_res->entry.long_value);
                } break;
                case STACK_ENTRY_REF: {
                    push_ref(op_stack, exec_res->entry.ptr_value);
                } break;
                case STACK_ENTRY_NONE: {

                } break;
                default: {
                    assert(0 && "unknown return type");
                }
                }
                free(exec_res);
            }
            pc += 3;
        } break;

        /* Load int from array */
        case i_iaload: {
            int idx = pop_int(op_stack);
            int *arr = pop_ref(op_stack);

            push_int(op_stack, arr[idx]);
            pc += 1;
        } break;

        /* Store into int array */
        case i_iastore: {
            int value = pop_int(op_stack);
            int idx = pop_int(op_stack);
            int *arr = pop_ref(op_stack);

            arr[idx] = value;
            pc += 1;
        } break;

        /* Create new array */
        case i_newarray: {
            uint8_t index = code_buf[pc + 1];

            int count = pop_int(op_stack);
            void *arr = NULL;
            switch (index)
            {
            case T_INT: {
                arr = create_array(clazz, count);
            } break;
            case T_BOOLEN:
            case T_CHAR:
            case T_FLOAT:
            case T_DOUBLE:
            case T_BYTE:
            case T_SHORT:
            case T_LONG:
            default:
                assert(0 && "only support int array");
                break;
            }
            push_ref(op_stack, arr);
            pc += 2;
        } break;

        /* Create new multidimensional array */
        case i_multianewarray: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);
            uint8_t dimensions = code_buf[pc + 3];
            char *type = (char *)(get_constant(&clazz->constant_pool, get_class_name(&clazz->constant_pool, index)->string_index))->info;
            assert (dimensions == 2 && strcmp(type, "[[I") == 0 && "this VM only support two dimension integer array");
            
            int32_t x = pop_int(op_stack), y = pop_int(op_stack);
            void **arr = create_two_dimension_array(clazz, x, y);
            push_ref(op_stack, arr);
            pc += 4;
        } break;

        }
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        return -1;

    /* attempt to read given class file */
    FILE *class_file = fopen(argv[1], "r");
    assert(class_file && "Failed to open file");

    /* parse the class file */
    class_file_t *clazz = malloc(sizeof(class_file_t));
    *clazz = get_class(class_file);

    int error = fclose(class_file);
    assert(!error && "Failed to close file");

    init_class_heap();
    init_object_heap();
    load_native_class("java");

    /* native class clinit */
    for (int i = 0; i < class_heap.length; ++i) {
        method_t *method = find_method("<clinit>", "()V", class_heap.class_info[i]->clazz);
        if (method) {
            local_variable_t own_locals[method->code.max_locals];
            stack_entry_t *exec_res = execute(method, own_locals, class_heap.class_info[i]->clazz);
            assert(exec_res->type == STACK_ENTRY_NONE && "<clinit> must be no return");
            free(exec_res);
        }
    }

    char *match = strrchr(argv[1], '/');
    if (match == NULL) {
        add_class(clazz, NULL, argv[1]);
        prefix = malloc(1 * sizeof(char));
        prefix[0] = '\0';
    } else {
        add_class(clazz, NULL, match + 1);
<<<<<<< HEAD
<<<<<<< HEAD
        prefix = malloc((match - argv[1] + 2) * sizeof(char));
=======
        prefix = malloc((match - argv[1] + 1) * sizeof(char));
>>>>>>> 0c9995f... Implement native java class to let invokevirtual more resonable
=======
        prefix = malloc((match - argv[1] + 2) * sizeof(char));
>>>>>>> 8d80d49... Remove memory leak and use heap to record array and temporary string
        strncpy(prefix, argv[1], match - argv[1] + 1);
        prefix[match - argv[1] + 1] = '\0';
    }
    
    method_t *method = find_method("<clinit>", "()V", clazz);
    if (method) {
        local_variable_t own_locals[method->code.max_locals];
        stack_entry_t *exec_res = execute(method, own_locals, clazz);
        assert(exec_res->type == STACK_ENTRY_NONE && "<clinit> must be no return");
        free(exec_res);
    }


    /* execute the main method if found */
    method_t *main_method =
        find_method("main", "([Ljava/lang/String;)V", clazz);
    assert(main_method && "Missing main() method");

    /* FIXME: locals[0] contains a reference to String[] args, but right now
     * we lack of the support for java.lang.Object. Leave it unin]itialized.
     */
    local_variable_t locals[main_method->code.max_locals];
    memset(locals, 0, sizeof(locals));
<<<<<<< HEAD
<<<<<<< HEAD
    int32_t *result = execute(main_method, locals, clazz);
    assert(!result && "main() should return void");

<<<<<<< HEAD
=======
=======
    variable_t *result = execute(main_method, locals, clazz);
=======
    stack_entry_t *result = execute(main_method, locals, clazz);
>>>>>>> 24ccd06... Clean some useless code
    assert(result->type == STACK_ENTRY_NONE && "main() should return void");
    free(result);
<<<<<<< HEAD
>>>>>>> 12dc8d2... Add some instruction and fix field free error
    for (int i = 1; i < main_method->code.max_locals; ++i) {
        if (locals[i].type == STACK_ENTRY_REF) {
            //free(locals[i].entry.ptr);
        }
    }

    //free_class(&clazz);
>>>>>>> 8d80d49... Remove memory leak and use heap to record array and temporary string
=======

>>>>>>> d57961e... Remove some dummy comment
    free(prefix);
    free_object_heap();
    free_class_heap();

    return 0;
}
