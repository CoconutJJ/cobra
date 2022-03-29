#include "../compiler/bytecode.h"
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define STACK_SIZE 1024
#define FRAME_SIZE 1024
#define CODE_SIZE  1024
struct VM {
        int8_t *ip;
        int8_t instructions[CODE_SIZE];

        int32_t *sp;
        int32_t *bp;
        int32_t stack[STACK_SIZE];

        int32_t frame_no;
        int32_t *stack_frames[FRAME_SIZE];
};

struct VM vm;

int32_t read_uint32 ()
{
        int32_t value = *(int32_t *)vm.ip;
        vm.ip += sizeof (int32_t);
        return value;
}

enum OpCode read_op ()
{
        int8_t op = *vm.ip;
        vm.ip += sizeof (int8_t);

        return (enum OpCode)op;
}

int32_t pop ()
{
        vm.sp -= sizeof (int32_t);

        return *vm.sp;
}

void push (int32_t v)
{
        *vm.sp = v;
        vm.sp += sizeof (int32_t);
}

void add_op ()
{
        push (pop () + pop ());
}

void neg_op ()
{
        push (-pop ());
}

void mult_op ()
{
        push (pop () * pop ());
}

void div_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a / b);
}

void mod_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a % b);
}

void eq_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a == b);
}

void gt_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a > b);
}

void lt_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a < b);
}

void gteq_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a >= b);
}

void lteq_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a <= b);
}

void and_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a && b);
}

void or_op ()
{
        int32_t b = pop ();
        int32_t a = pop ();

        push (a || b);
}

void jmp_op ()
{
        vm.ip = (int8_t *)(vm.instructions + read_uint32 ());
}

void jmpfalse_op ()
{
        int32_t addr = read_uint32 ();
        if (!pop ())
                vm.ip = (int8_t *)(vm.instructions + addr);
}

void store_op ()
{
        int32_t offset = read_uint32 ();
        int32_t value = pop ();

        *(vm.bp + offset) = value;
}

void load_op ()
{
        int32_t offset = read_uint32 ();

        push (*(vm.bp + offset));
}

void call_op ()
{
        int32_t addr = read_uint32 ();

        push (vm.ip - vm.instructions);
        vm.stack_frames[vm.frame_no++] = vm.bp;
        vm.bp = (int32_t *)vm.ip;
        vm.ip = vm.instructions + addr;
}

void ret_op ()
{
        int32_t ret_addr = pop ();

        vm.bp = vm.stack_frames[--vm.frame_no];
        vm.ip = vm.instructions + ret_addr;
}

void run ()
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
                case OPPUSH: push (read_uint32 ()); break;
                case OPPOP: pop (); break;
                case OPHALT: goto _halt; break;
                case OPCALL: call_op (); break;
                case OPRET: ret_op (); break;
                }
        }
_halt:
        return;
}

int initialize_and_run (uint8_t *code, size_t code_size, int32_t entry_address)
{
        if (code_size > CODE_SIZE)
                return -1;

        for (size_t i = 0; i < code_size; i++)
                vm.instructions[i] = code[i];

        vm.sp = vm.stack;
        vm.bp = vm.stack;
        vm.ip = vm.instructions + entry_address;
        vm.frame_no = 0;

        run ();

        return 1;
}

void parse_cmd (int argc, char **argv)
{
}

int main (int argc, char **argv)
{
}