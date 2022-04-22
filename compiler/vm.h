#ifndef vm_h
#define vm_h
#include "bytecode.h"
#include "function.h"
#include <stdint.h>

#define STACK_SIZE  (1024 * 3)
#define FRAME_SIZE  (1024 * 3)
#define CODE_SIZE   (1024 * 3)
#define MAX_THREADS 10

enum thread_state { RUNNING, BLOCKED, KILLED, EXITED, UNUSED };

struct context {
        int8_t *ip;
        int8_t instructions[CODE_SIZE];
        int32_t *sp;
        int32_t *bp;
        int32_t stack[STACK_SIZE];
        int32_t frame_no;
        int32_t *stack_frames[FRAME_SIZE];
        uint64_t op_count;
        enum thread_state state;
        struct context *next;
        struct context *previous;
};

class VM {
    public:
        VM ();
        int load_file_and_run (char *filename);
        int load_function_and_run (Function *f);
        bool verbose;

    private:
        struct context *thread;

        struct context threads[MAX_THREADS];

        void assert_valid_stack_location (const char *prefix, void *ptr);
        void assert_valid_ip (int8_t *ip);
        int32_t read_int32 ();
        enum OpCode read_op ();

        int32_t pop ();
        void push (int32_t v);

        void bin_op (enum OpCode op);
        void neg_op ();
        void not_op ();
        void jmp_op ();
        void jmpfalse_op ();
        void store_op ();
        void load_op ();
        void call_op ();
        void ret_op ();
        void halt_op ();
        void fork_op ();
        void kill_op ();
        void print_op ();
        void swap_op ();

        struct context *allocate_thread ();
        void schedule ();
        void add_thread(struct context *thread);
        void remove_thread(struct context *thread);
        void execute_instruction ();
        void run ();
        int initialize_and_run (int8_t *code, size_t code_size, int32_t entry_address);
        void display_thread_info (struct context *thread);
        void copy_thread_stats (struct context *src, struct context *dest);
};

#endif