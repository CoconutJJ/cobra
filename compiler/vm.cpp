#include "vm.h"
#include "bytecode.h"
#include "function.h"
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define STACK_SIZE 1024
#define FRAME_SIZE 1024
#define CODE_SIZE  1024

VM::VM ()
{
        this->ip = NULL;
        this->bp = NULL;
        this->frame_no = 0;
}

void VM::assert_valid_stack_location (const char *prefix, void *ptr)
{
        if (ptr > this->stack + STACK_SIZE) {
                fprintf (stderr, "error: stack overflow: %s", prefix);
                exit (EXIT_FAILURE);
        } else if (ptr < this->stack) {
                fprintf (stderr, "error: stack underflow: %s", prefix);
                exit (EXIT_FAILURE);
        }
}

int32_t VM::read_int32 ()
{
        int32_t value = *(int32_t *)this->ip;
        this->ip += sizeof (int32_t);
        return value;
}

enum OpCode VM::read_op ()
{
        int8_t op = *this->ip;
        this->ip += sizeof (int8_t);

        return (enum OpCode)op;
}

int32_t VM::pop ()
{
        this->sp -= 1;

        this->assert_valid_stack_location ("pop: attempted to pop stack with invalid VM configuration", this->sp);

        return *this->sp;
}

void VM::push (int32_t v)
{
        *this->sp = v;
        this->sp += 1;
        assert_valid_stack_location ("push: attempted to push stack with invalid VM configuration", this->sp);
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
        this->ip = (int8_t *)(this->instructions + read_int32 ());
}

void VM::jmpfalse_op ()
{
        int32_t addr = read_int32 ();
        if (!pop ())
                this->ip = (int8_t *)(this->instructions + addr);
}

void VM::store_op ()
{
        int32_t offset = read_int32 ();
        int32_t value = pop ();

        int32_t *store_location = this->bp + offset;

        assert_valid_stack_location ("store: attempted to store with invalid VM configuration", store_location);

        *store_location = value;
}

void VM::load_op ()
{
        int32_t offset = read_int32 ();

        int32_t *load_location = this->bp + offset;

        assert_valid_stack_location ("load: attempted to load with invalid VM configuration", load_location);

        push (*load_location);
}

void VM::call_op ()
{
        int32_t addr = read_int32 ();

        push (this->ip - this->instructions);
        this->stack_frames[this->frame_no++] = this->bp;
        this->bp = this->sp;
        this->ip = this->instructions + addr;
}

void VM::ret_op ()
{
        int32_t ret_addr = pop ();

        this->bp = this->stack_frames[--this->frame_no];
        this->ip = this->instructions + ret_addr;
}

void VM::run ()
{
        for (;;) {
                switch (read_op ()) {
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
                case OPHALT: goto _halt; break;
                case OPCALL: call_op (); break;
                case OPRET: ret_op (); break;
                }
        }
_halt:
        return;
}

int VM::initialize_and_run (int8_t *code, size_t code_size, int32_t entry_address)
{
        if (code_size > CODE_SIZE)
                return -1;

        for (size_t i = 0; i < code_size; i++)
                this->instructions[i] = code[i];

        this->sp = this->stack;
        this->bp = this->stack;
        this->ip = this->instructions + entry_address;
        this->frame_no = 0;

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
