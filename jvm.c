/* PitifulVM is a minimalist Java Virtual Machine implementation written in C.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "java_file.h"
#include "stack.h"
#include "class_heap.h"
#include "object_heap.h"

/* TODO: add -cp arg to achieve class path select */
char *prefix;


static inline void bipush(stack_frame_t *op_stack,
                          uint32_t pc,
                          uint8_t *code_buf)
{
    int8_t param = code_buf[pc + 1];

    push_byte(op_stack, param);
}

static inline void sipush(stack_frame_t *op_stack,
                          uint32_t pc,
                          uint8_t *code_buf)
{
    uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
    int16_t res = ((param1 << 8) | param2);

    push_short(op_stack, res);
}

static inline void iadd(stack_frame_t *op_stack)
{
    int32_t op1 = pop_int(op_stack);
    int32_t op2 = pop_int(op_stack);
    
    push_int(op_stack, op1 + op2);
}

static inline void isub(stack_frame_t *op_stack)
{
    int32_t op1 = pop_int(op_stack);
    int32_t op2 = pop_int(op_stack);
    
    push_int(op_stack, op2 - op1);
}

static inline void imul(stack_frame_t *op_stack)
{
    int32_t op1 = pop_int(op_stack);
    int32_t op2 = pop_int(op_stack);
    
    push_int(op_stack, op1 * op2);
}

static inline void idiv(stack_frame_t *op_stack)
{
    int32_t op1 = pop_int(op_stack);
    int32_t op2 = pop_int(op_stack);
    
    push_int(op_stack, op2 / op1);
}

static inline void irem(stack_frame_t *op_stack)
{
    int32_t op1 = pop_int(op_stack);
    int32_t op2 = pop_int(op_stack);
    
    push_int(op_stack, op2 % op1);
}

static inline void ineg(stack_frame_t *op_stack)
{
    int32_t op1 = pop_int(op_stack);
    
    push_int(op_stack, -op1);
}

static inline void invokevirtual(stack_frame_t *op_stack)
{
    int32_t op = pop_int(op_stack);

    /* FIXME: the implement is not correct. */
    printf("%d\n", op);
}

static inline void iconst(stack_frame_t *op_stack, uint8_t current)
{
    push_int(op_stack, current - i_iconst_0);
}

/**
 * Execute the opcode instructions of a method until it returns.
 *
 * @param method the method to run
 * @param locals the array of local variables, including the method parameters.
 *               Except for parameters, the locals are uninitialized.
 * @param clazz the class file the method belongs to
 * @return if the method returns an int, a heap-allocated pointer to it;
 *         NULL if the method returns void, NULL;
 */
int32_t *execute(method_t *method, local_variable_t *locals, class_file_t *clazz)
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
            int32_t *ret = malloc(sizeof(int32_t));
            *ret = pop_int(op_stack);
            free(op_stack->store);
            free(op_stack);
            return ret;
        } break;

        /* Return void from method */
        case i_return:
            free(op_stack->store);
            free(op_stack);
            return NULL;

        /* Invoke a class (static) method */
        case i_invokestatic: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);

            /* the method to be called */
            char *method_name, *method_descriptor, *class_name;
            class_name = find_method_info_from_index(index, clazz, &method_name, &method_descriptor);
            class_file_t *target_class = find_class_from_heap(class_name);

            if (target_class == NULL) {
                /* 7 = origin name length + ".class" + 1 */
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
            local_variable_t own_locals[own_method->code.max_locals];
            for (int i = num_params - 1; i >= 0; i--) {
                pop_to_local(op_stack, &own_locals[i]);
            }

            int32_t *exec_res = execute(own_method, own_locals, clazz);
            if (exec_res) {
                push_int(op_stack, *exec_res);
            }

            free(exec_res);
            pc += 3;
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

        /* Branch always */
        case i_goto: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            int16_t res = ((param1 << 8) | param2);
            pc += res;
        } break;

        /* Push item from run-time constant pool */
        case i_ldc: {
            constant_pool_t constant_pool = clazz->constant_pool;

            /* find the parameter which will be the index from which we retrieve
             * constant in the constant pool.
             */
            int16_t param = code_buf[pc + 1];

            /* get the constant */
            uint8_t *info = get_constant(&constant_pool, param)->info;
            
            /* need to check type */
            push_int(op_stack, ((CONSTANT_Integer_info *) info)->bytes);
            pc += 2;
        } break;

        /* Load int from local variable */
        case i_iload_0:
        case i_iload_1:
        case i_iload_2:
        case i_iload_3: {
            int32_t param = current - i_iload_0;
            int32_t loaded;

            memcpy(&loaded, locals[param].entry.val, sizeof(int32_t));
            push_int(op_stack, loaded);
            pc += 1;
        } break;

        /* Load int from local variable */
        case i_iload: {
            int32_t param = code_buf[pc + 1];
            int32_t loaded;
            
            memcpy(&loaded, locals[param].entry.val, sizeof(int32_t));
            push_int(op_stack, loaded);

            pc += 2;
        } break;

        /* Store int into local variable */
        case i_istore: {
            int32_t param = code_buf[pc + 1];
            int32_t stored = pop_int(op_stack);
            memcpy(locals[param].entry.val, &stored, sizeof(int32_t));
            pc += 2;
        } break;

        /* Store int into local variable */
        case i_istore_0:
        case i_istore_1:
        case i_istore_2:
        case i_istore_3: {
            int32_t param = current - i_istore_0;
            int32_t stored = pop_int(op_stack);
            memcpy(locals[param].entry.val, &stored, sizeof(int32_t));
            locals[param].type = STACK_ENTRY_INT;
            pc += 1;
        } break;

        /* Increment local variable by constant */
        case i_iinc: {
            uint8_t i = code_buf[pc + 1];
            int8_t b = code_buf[pc + 2]; /* signed value */
            int32_t value;
            memcpy(&value, locals[i].entry.val, sizeof(int32_t));
            value += b;
            memcpy(locals[i].entry.val, &value, sizeof(int32_t));
            pc += 3;
        } break;


        /* Push byte */
        case i_bipush: {
            bipush(op_stack, pc, code_buf);
            pc += 2;
        } break;

        /* Add int */
        case i_iadd: {
            iadd(op_stack);
            pc += 1;
        } break;

        /* Subtract int */
        case i_isub: {
            isub(op_stack);
            pc += 1;
        } break;

        /* Multiply int */
        case i_imul: {
            imul(op_stack);
            pc += 1;
        } break;

        /* Divide int */
        case i_idiv: {
            idiv(op_stack);
            pc += 1;
        } break;

        /* Remainder int */
        case i_irem: {
            irem(op_stack);
            pc += 1;
        } break;

        /* Negate int */
        case i_ineg: {
            ineg(op_stack);
            pc += 1;
        } break;

        /* Get static field from class */
        case i_getstatic: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);
            
            char *field_name, *field_descriptor, *class_name;
            class_name = find_field_info_from_index(index, clazz, &field_name, &field_descriptor);

            if (strcmp(class_name, "java/lang/System") == 0) {
                pc += 3;
                break;
            }

            assert(strcmp(field_descriptor, "I") == 0 && "Only support integer field");

            class_file_t *new_clazz = find_class_from_heap(class_name);

            /* if not found, add specify class path. this can be remove by record path infomation in class heap */
            if (!new_clazz) {
                char *tmp = malloc(sizeof(class_name) + strlen(prefix));
                strcpy(tmp, prefix);
                strcat(tmp, class_name);
                new_clazz = find_class_from_heap(tmp);

                free(tmp);
            }


            field_t *field = find_field(field_name, field_descriptor, new_clazz);
            
            push_int(op_stack, field->value->int_value);
            pc += 3;

        } break;

        /* Put static field to class */
        case i_putstatic: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);

            /* TODO: support other type (e.g reference) */
            int32_t value = pop_int(op_stack);
            
            char *field_name, *field_descriptor, *class_name;
            class_name = find_field_info_from_index(index, clazz, &field_name, &field_descriptor);
            assert(strcmp(field_descriptor, "I") == 0 && "Only support integer field");

            class_file_t *target_class = find_class_from_heap(class_name);

            if (target_class == NULL) {
                if (strcmp(class_name, "java/lang/System") == 0) {
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

            field_t *field = find_field(field_name, field_descriptor, target_class);
            
            field->value->int_value = value;
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
            iconst(op_stack, current);
            pc += 1;
        } break;

        case i_aload: {
            int32_t param = code_buf[pc + 1];
            object_t *obj = locals[param].entry.ptr;
            push_ref(op_stack, obj);
            pc += 2;
        } break;

        case i_aload_0:
        case i_aload_1:
        case i_aload_2:
        case i_aload_3: {
            int32_t param = current - i_aload_0;
            object_t *obj = locals[param].entry.ptr;
            push_ref(op_stack, obj);
            pc += 1;
        } break;
        
        case i_astore: {
            int32_t param = code_buf[pc + 1];
            locals[param].entry.ptr = pop_ref(op_stack);
            pc += 2;
        } break;

        case i_astore_0:
        case i_astore_1:
        case i_astore_2:
        case i_astore_3: {
            int32_t param = current - i_astore_0;
            locals[param].entry.ptr = pop_ref(op_stack);
            pc += 1;
        } break;

        /* Push short */
        case i_sipush: {
            sipush(op_stack, pc, code_buf);
            pc += 3;
        } break;

        case i_getfield: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);

            object_t *obj = pop_ref(op_stack);
            char *field_name, *field_descriptor, *class_name;
            class_name = find_field_info_from_index(index, obj->type, &field_name, &field_descriptor);
            variable_t *addr = find_field_addr(obj, field_name);
            push_int(op_stack, addr->int_value);
            pc += 3;
        } break;

        case i_putfield: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);

            /* TODO: support other type (e.g reference) */
            int32_t value = pop_int(op_stack);
            object_t *obj = pop_ref(op_stack);
            
            char *field_name, *field_descriptor, *class_name;
            class_name = find_field_info_from_index(index, obj->type, &field_name, &field_descriptor);
            assert(strcmp(field_descriptor, "I") == 0 && "Only support integer field");
            variable_t *addr = find_field_addr(obj, field_name);
            addr->int_value = value;
            pc += 3;
        } break;

        /* New object */
        case i_new: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);
            
            char *class_name = find_class_name_from_index(index, clazz);
            class_file_t *new_class = find_class_from_heap(class_name);

            if (new_class == NULL) {
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
                free(tmp);
            }
            
            object_t *object = create_object(new_class);
            push_ref(op_stack, object);

            pc += 3;
        } break;

        case i_dup: {
            op_stack->store[op_stack->size] = op_stack->store[op_stack->size - 1];
            op_stack->size++;
            pc += 1;
        } break;

        case i_invokespecial: {
            uint8_t param1 = code_buf[pc + 1], param2 = code_buf[pc + 2];
            uint16_t index = ((param1 << 8) | param2);

            /* the method to be called */
            char *method_name, *method_descriptor, *class_name;
            class_name = find_method_info_from_index(index, clazz, &method_name, &method_descriptor);
            class_file_t *target_class = find_class_from_heap(class_name);

            if (target_class == NULL) {
                /* this should be run when invokespecial with java/lang/Object."<init>" */
                if (strcmp(class_name, "java/lang/Object") == 0) {
                    pop_ref(op_stack);
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

            /* constructor */
            method_t *constructor = find_method(method_name, method_descriptor, target_class);
            uint16_t num_params = get_number_of_parameters(constructor);
            local_variable_t own_locals[constructor->code.max_locals];
            for (int i = num_params; i >= 1; i--) {
                pop_to_local(op_stack, &own_locals[i]);
            }
            object_t *obj = pop_ref(op_stack);

            /* first argument is this pointer */
            own_locals[0].entry.ptr = obj;
            own_locals[0].type = STACK_ENTRY_REF;

            int32_t *exec_res = execute(constructor, own_locals, obj->type);
            assert(exec_res == NULL && "constructor must be no return");
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
            own_locals[0].entry.ptr = obj;
            own_locals[0].type = STACK_ENTRY_REF;

            /* FIXME: virtual method is related to object and its field */
            int32_t *exec_res = execute(method, own_locals, obj->type);
            if (exec_res) {
                push_int(op_stack, *exec_res);
            }
            free(exec_res);
            pc += 3;
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
    /* change to heap */
    class_file_t *clazz = malloc(sizeof(class_file_t));
    *clazz = get_class(class_file);
    
    int error = fclose(class_file);
    assert(!error && "Failed to close file");

    init_class_heap();

    char *match = strrchr(argv[1], '/');
    if (match == NULL) {
        add_class(clazz, NULL, argv[1]);
        prefix = malloc(1 * sizeof(char));
        prefix[0] = '\0';
    } else {
        add_class(clazz, NULL, match + 1);
        prefix = malloc((match - argv[1]) * sizeof(char));
        strncpy(prefix, argv[1], match - argv[1] + 1);
        prefix[match - argv[1] + 1] = '\0';
    }
    

    init_object_heap();

    /* execute the main method if found */
    method_t *main_method =
        find_method("main", "([Ljava/lang/String;)V", clazz);
    assert(main_method && "Missing main() method");

    /* FIXME: locals[0] contains a reference to String[] args, but right now
     * we lack of the support for java.lang.Object. Leave it uninitialized.
     */
    local_variable_t locals[main_method->code.max_locals];
    int32_t *result = execute(main_method, locals, clazz);
    assert(!result && "main() should return void");

    //free_class(&clazz);
    free(prefix);
    free_class_heap();
    free_object_heap();

    return 0;
}
