#include "vm.h"
#include "bytecode.h"
#include "function.h"
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

VM::VM ()
{
        for (int i = 0; i < MAX_THREADS; i++) {
                this->threads[i].state = UNUSED;
        }
        this->thread = this->allocate_thread ();
        this->thread->ip = NULL;
        this->thread->bp = NULL;
        this->thread->frame_no = 0;
        this->thread->op_count = 0;
        this->verbose = false;
        this->thread->next = this->thread;
        this->thread->previous = this->thread;
}

void VM::assert_valid_ip (int8_t *ip)
{
        if (ip < this->thread->instructions) {
                fprintf (stderr,
                         "error: underflow: invalid instruction pointer location: address: %ld",
                         this->thread->ip - this->thread->instructions);
                exit (EXIT_FAILURE);
        }

        if (ip >= this->thread->instructions + CODE_SIZE) {
                fprintf (stderr,
                         "error: overflow: invalid instruction pointer location: address: %ld",
                         this->thread->ip - this->thread->instructions);
                exit (EXIT_FAILURE);
        }
}

void VM::assert_valid_stack_location (const char *prefix, void *ptr)
{
        if (ptr >= this->thread->stack + STACK_SIZE) {
                fprintf (stderr, "error: stack overflow: %s\n", prefix);
                fprintf (stderr, "ip: %ld", this->thread->ip - this->thread->instructions);

                exit (EXIT_FAILURE);
        } else if (ptr < this->thread->stack) {
                fprintf (stderr, "error: stack underflow: %s\n", prefix);
                fprintf (stderr, "ip: %ld", this->thread->ip - this->thread->instructions);
                exit (EXIT_FAILURE);
        }
}

int32_t VM::read_int32 ()
{
        this->assert_valid_ip (this->thread->ip);

        int32_t value = *(int32_t *)this->thread->ip;
        this->thread->ip += sizeof (int32_t);
        return value;
}

enum OpCode VM::read_op ()
{
        this->assert_valid_ip (this->thread->ip);

        int8_t op = *this->thread->ip;
        this->thread->ip += sizeof (int8_t);

        return (enum OpCode)op;
}

int32_t VM::pop ()
{
        this->thread->sp -= 1;

        this->assert_valid_stack_location ("pop: attempted to pop stack with invalid VM configuration",
                                           this->thread->sp);

        return *this->thread->sp;
}

void VM::push (int32_t v)
{
        *this->thread->sp = v;
        this->thread->sp += 1;
        this->assert_valid_stack_location ("push: attempted to push stack with invalid VM configuration",
                                           this->thread->sp);
}

void VM::bin_op (enum OpCode op)
{
        int32_t b = this->pop ();
        int32_t a = this->pop ();
        int32_t c;
        switch (op) {
        case OPADD: c = a + b; break;
        case OPMULT: c = a * b; break;
        case OPDIV: c = a / b; break;
        case OPMOD: c = a % b; break;
        case OPEQ: c = (a == b); break;
        case OPGT: c = (a > b); break;
        case OPGTEQ: c = (a >= b); break;
        case OPLT: c = (a < b); break;
        case OPLTEQ: c = (a <= b); break;
        case OPAND: c = (a && b); break;
        case OPOR: c = (a || b); break;
        default: exit (EXIT_FAILURE); break;
        }
        this->push (c);
}

void VM::neg_op ()
{
        push (-pop ());
}

void VM::not_op ()
{
        push (1 - pop ());
}

void VM::jmp_op ()
{
        int32_t addr = read_int32 ();
        this->thread->ip = (int8_t *)(this->thread->instructions + addr);
}

void VM::jmpfalse_op ()
{
        int32_t addr = read_int32 ();

        if (!pop ())
                this->thread->ip = (int8_t *)(this->thread->instructions + addr);
}

void VM::store_op ()
{
        int32_t offset = read_int32 ();
        int32_t value = pop ();

        int32_t *store_location = this->thread->bp + offset;

        assert_valid_stack_location ("store: attempted to store with invalid VM configuration", store_location);

        *store_location = value;

        if (store_location >= this->thread->sp) {
                this->thread->sp = store_location + 1;
        }
}

void VM::load_op ()
{
        int32_t offset = read_int32 ();

        int32_t *load_location = this->thread->bp + offset;
        assert_valid_stack_location ("load: attempted to load with invalid VM configuration", load_location);
        push (*load_location);
}

void VM::call_op ()
{
        int32_t addr = read_int32 ();
        push (this->thread->ip - this->thread->instructions);

        if (this->thread->frame_no == FRAME_SIZE) {
                fprintf (stderr, "call: maximum recursion depth exceeded\n");
                this->thread->state = KILLED;
                return;
        }

        this->thread->stack_frames[this->thread->frame_no++] = this->thread->bp;
        this->thread->bp = this->thread->sp;
        this->thread->ip = this->thread->instructions + addr;
}

void VM::swap_op ()
{
        int32_t a = read_int32 ();
        int32_t b = read_int32 ();

        int32_t *location_a = this->thread->bp + a;
        int32_t *location_b = this->thread->bp + b;

        assert_valid_stack_location ("swap: invalid swap location pair", location_a);
        assert_valid_stack_location ("swap: invalid swap location pair", location_b);

        int32_t temp = *location_a;
        *location_a = *location_b;
        *location_b = temp;
}

void VM::ret_op ()
{
        int32_t ret_value = pop ();
        int32_t ret_addr = pop ();
        this->thread->bp = this->thread->stack_frames[--this->thread->frame_no];
        this->thread->ip = this->thread->instructions + ret_addr;

        this->push (ret_value);
}

void VM::halt_op ()
{
        this->thread->state = EXITED;
        this->display_thread_info (this->thread);
}

void VM::fork_op ()
{
        struct context *new_thread = this->allocate_thread ();

        if (!new_thread) {
                this->push (-1);
                return;
        }

        this->copy_thread_stats (this->thread, new_thread);

        this->push (new_thread - this->threads + 1);
        struct context *old_thread = this->thread;
        this->thread = new_thread;
        this->push (0);
        this->thread = old_thread;

        this->add_thread (new_thread);

        if (this->verbose)
                printf ("New thread was created...\n");

        this->display_thread_info (new_thread);
}

void VM::kill_op ()
{
        int32_t thread_id = this->pop () - 1;

        struct context *victim_thread = &this->threads[thread_id];

        if (victim_thread->state == RUNNING) {
                victim_thread->state = KILLED;
                this->remove_thread (victim_thread);
                this->push (1);
        } else {
                this->push (0);
        }

        this->display_thread_info (victim_thread);
}

void VM::print_op ()
{
        printf ("%d\n", this->pop ());
}

void VM::display_thread_info (struct context *thread)
{
        if (!this->verbose)
                return;

        int thread_id = thread - this->threads;

        switch (thread->state) {
        case EXITED: printf ("Thread #%d exited normally\n", thread_id); break;
        case KILLED: printf ("Thread #%d was killed\n", thread_id); break;
        case RUNNING: printf ("Thread #%d is running\n", thread_id); break;
        default: break;
        }

        printf ("Opcodes executed: %lu\n", thread->op_count);
}

void VM::copy_thread_stats (struct context *src, struct context *dest)
{
        dest->ip = dest->instructions + (src->ip - src->instructions);
        dest->sp = dest->stack + (src->sp - src->stack);
        dest->bp = dest->stack + (src->bp - src->stack);
        dest->frame_no = src->frame_no;
        dest->op_count = src->op_count;
        dest->state = src->state;

        size_t used_stack_size = (src->sp - src->stack) * sizeof (int32_t);
        memcpy (dest->stack, src->stack, used_stack_size);
        memcpy (dest->instructions, src->instructions, CODE_SIZE * sizeof (int8_t));

        for (int i = 0; i < src->frame_no; i++)
                dest->stack_frames[i] = dest->stack + (src->stack_frames[i] - src->stack);
}

struct context *VM::allocate_thread ()
{
        for (int curr = 0; curr < MAX_THREADS; curr++) {
                struct context *free_thread = &this->threads[curr];

                switch (free_thread->state) {
                case UNUSED:
                case KILLED:
                case EXITED: {
                        free_thread->op_count = 0;
                        return free_thread;
                }
                default: continue;
                }
        }

        return NULL;
}

void VM::add_thread (struct context *thread)
{
        struct context *curr_next = this->thread->next;

        this->thread->next = thread;
        thread->previous = this->thread;

        thread->next = curr_next;
        curr_next->previous = thread;
}

void VM::remove_thread (struct context *thread)
{
        if (thread->previous == thread || thread->next == thread)
                return;

        struct context *left = thread->previous;
        struct context *right = thread->next;

        left->next = right;
        right->previous = left;
}

void VM::schedule ()
{
        struct context *old_thread = this->thread;

        this->thread = this->thread->next;

        if (old_thread->state != RUNNING)
                this->remove_thread (old_thread);
}
void VM::execute_instruction ()
{
        enum OpCode op = read_op ();

        if (OPADD <= op && op <= OPOR) {
                bin_op (op);
        } else {
                switch (op) {
                case OPNOT: not_op (); break;
                case OPNEG: neg_op (); break;
                case OPJMP: jmp_op (); break;
                case OPJMPFALSE: jmpfalse_op (); break;
                case OPSTORE: store_op (); break;
                case OPLOAD: load_op (); break;
                case OPPUSH: push (read_int32 ()); break;
                case OPPOP: pop (); break;
                case OPHALT: halt_op (); break;
                case OPCALL: call_op (); break;
                case OPFORK: fork_op (); break;
                case OPPRINT: print_op (); break;
                case OPKILL: kill_op (); break;
                case OPRET: ret_op (); break;
                default:
                        fprintf (stderr, "illegal instruction: 0x%x\n", op);
                        this->thread->state = KILLED;
                        break;
                }
        }

        this->thread->op_count++;

        return;
}

void VM::run ()
{
        while (this->thread->state == RUNNING) {
                this->execute_instruction ();
                this->schedule ();
        }
}

int VM::initialize_and_run (int8_t *code, size_t code_size, int32_t entry_address)
{
        if (code_size > CODE_SIZE)
                return -1;

        for (size_t i = 0; i < code_size; i++)
                this->thread->instructions[i] = code[i];

        this->thread->sp = this->thread->stack;
        this->thread->bp = this->thread->stack;
        this->thread->ip = this->thread->instructions + entry_address;
        this->thread->frame_no = 0;
        this->thread->state = RUNNING;
        this->thread->op_count = 0;
        run ();

        return 1;
}

int VM::load_file_and_run (char *filename)
{
        FILE *fp = fopen (filename, "rb");

        if (!fp) {
                perror ("fopen");
                exit (EXIT_FAILURE);
        }

        fseek (fp, 0, SEEK_END);

        size_t fsize = ftell (fp);

        int8_t code[fsize];

        rewind (fp);

        fread (code, sizeof (int8_t), fsize, fp);

        fclose (fp);

        return initialize_and_run (code, fsize, 0);
}

int VM::load_function_and_run (Function *f)
{
        return this->initialize_and_run (f->bytecode->chunk, f->bytecode->count, f->entry_address);
}
