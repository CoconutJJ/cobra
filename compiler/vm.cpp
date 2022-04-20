#include "vm.h"
#include "bytecode.h"
#include "function.h"
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STACK_SIZE 1024
#define FRAME_SIZE 1024
#define CODE_SIZE  1024

VM::VM ()
{
        for (int i = 0; i < MAX_THREADS; i++) {
                this->threads[i].state = UNUSED;
        }
        this->thread = this->allocate_thread ();
        this->thread->ip = NULL;
        this->thread->bp = NULL;
        this->thread->frame_no = 0;
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
        if (ptr > this->thread->sp) {
                fprintf (stderr, "error: stack overflow: %s\n", prefix);
                exit (EXIT_FAILURE);
        } else if (ptr < this->thread->stack) {
                fprintf (stderr, "error: stack underflow: %s\n", prefix);
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

void VM::add_op ()
{
        push (pop () + pop ());
}

void VM::neg_op ()
{
        push (-pop ());
}

void VM::mult_op ()
{
        push (pop () * pop ());
}

void VM::div_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a / b);
}

void VM::mod_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a % b);
}

void VM::eq_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a == b);
}

void VM::gt_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a > b);
}

void VM::lt_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a < b);
}

void VM::gteq_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a >= b);
}

void VM::lteq_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a <= b);
}

void VM::and_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a && b);
}

void VM::or_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a || b);
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
}

void VM::fork_op ()
{
        struct context *new_thread = this->allocate_thread ();

        if (!new_thread) {
                this->push (-1);
                return;
        }

        size_t used_stack_size = (this->thread->sp - this->thread->stack) * sizeof (int32_t);

        memcpy (new_thread->instructions, this->thread->instructions, CODE_SIZE * sizeof (int8_t));

        memcpy (new_thread->stack, this->thread->stack, used_stack_size);

        for (int i = 0; i < this->thread->frame_no; i++) {
                new_thread->stack_frames[i] = new_thread->stack + (this->thread->stack_frames[i] - this->thread->stack);
        }

        new_thread->ip = new_thread->instructions + (this->thread->ip - this->thread->instructions);
        new_thread->bp = new_thread->stack + (this->thread->bp - this->thread->stack);
        new_thread->sp = new_thread->stack + (this->thread->sp - this->thread->stack);

        this->push (new_thread - this->threads);
        struct context *old_thread = this->thread;
        this->thread = new_thread;
        this->push (0);
        this->thread = old_thread;
}

void VM::kill_op ()
{
        int32_t thread_id = this->pop ();

        struct context *victim_thread = &this->threads[thread_id];

        victim_thread->state = KILLED;
}

struct context *VM::allocate_thread ()
{
        for (int curr = 0; curr < MAX_THREADS; curr++) {
                struct context *free_thread = &this->threads[curr];

                if (free_thread->state == UNUSED)
                        return free_thread;
        }

        return NULL;
}

void VM::schedule ()
{
        int index = this->thread - this->threads;

        int curr = (index + 1) % MAX_THREADS;

        while (curr != index) {
                if (this->threads[curr].state == RUNNING) {
                        this->thread = &this->threads[curr];
                        break;
                }

                curr = (curr + 1) % MAX_THREADS;
        }
}
void VM::execute_instruction ()
{
        enum OpCode op;

        switch ((op = read_op ())) {
        case OPADD: add_op (); break;
        case OPNEG: neg_op (); break;
        case OPMULT: mult_op (); break;
        case OPDIV: div_op (); break;
        case OPMOD: mod_op (); break;
        case OPEQ: eq_op (); break;
        case OPGT: gt_op (); break;
        case OPLT: lt_op (); break;
        case OPGTEQ: gteq_op (); break;
        case OPLTEQ: lteq_op (); break;
        case OPAND: and_op (); break;
        case OPOR: or_op (); break;
        case OPJMP: jmp_op (); break;
        case OPJMPFALSE: jmpfalse_op (); break;
        case OPSTORE: store_op (); break;
        case OPLOAD: load_op (); break;
        case OPPUSH: push (read_int32 ()); break;
        case OPPOP: pop (); break;
        case OPHALT: halt_op (); break;
        case OPCALL: call_op (); break;
        case OPFORK: fork_op (); break;
        case OPKILL: kill_op (); break;
        case OPRET: ret_op (); break;
        default:
                fprintf (stderr, "illegal instruction: 0x%x\n", op);
                this->thread->state = KILLED;
                break;
        }

_halt:
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
        run ();

        return 1;
}

int VM::load_file_and_run (char *filename)
{
        FILE *fp = fopen (filename, "rb");

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
