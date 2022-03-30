#ifndef vm_h
#define vm_h
#include "function.h"
#include "bytecode.h"
#include <stdint.h>

#define STACK_SIZE 1024
#define FRAME_SIZE 1024
#define CODE_SIZE  1024

class VM {
    public:
        VM ();
        int load_file_and_run (char *filename);
        int load_function_and_run (Function *f);

    private:
        int8_t *ip;
        int8_t instructions[CODE_SIZE];
        int32_t *sp;
        int32_t *bp;
        int32_t stack[STACK_SIZE];
        int32_t frame_no;
        int32_t *stack_frames[FRAME_SIZE];

        void assert_valid_stack_location (char *prefix, void *ptr);

        int32_t read_int32 ();
        enum OpCode read_op ();

        int32_t pop ();
        void push (int32_t v);
        void add_op ();
        void neg_op ();
        void mult_op ();
        void div_op ();
        void mod_op ();
        void eq_op ();
        void gt_op ();
        void lt_op ();
        void gteq_op ();
        void lteq_op ();
        void and_op ();
        void or_op ();
        void jmp_op ();
        void jmpfalse_op ();
        void store_op ();
        void load_op ();
        void call_op ();
        void ret_op ();

        void run ();
        int initialize_and_run (int8_t *code, size_t code_size, int32_t entry_address);
};

#endif