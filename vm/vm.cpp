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

void assert_valid_stack_location (char *prefix, void *ptr)
{
        if (ptr > vm.stack + STACK_SIZE) {
                fprintf (stderr, "error: stack overflow: %s", prefix);
                exit (EXIT_FAILURE);
        } else if (ptr < vm.stack) {
                fprintf (stderr, "error: stack underflow: %s", prefix);
                exit (EXIT_FAILURE);
        }
}

int32_t read_int32 ()
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

        assert_valid_stack_location ("pop: attempted to pop stack with invalid VM configuration", vm.sp);

        return *vm.sp;
}

void push (int32_t v)
{
        *vm.sp = v;
        vm.sp += sizeof (int32_t);
        assert_valid_stack_location ("push: attempted to push stack with invalid VM configuration", vm.sp);
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
        vm.ip = (int8_t *)(vm.instructions + read_int32 ());
}

void jmpfalse_op ()
{
        int32_t addr = read_int32 ();
        if (!pop ())
                vm.ip = (int8_t *)(vm.instructions + addr);
}

void store_op ()
{
        int32_t offset = read_int32 ();
        int32_t value = pop ();

        int32_t *store_location = vm.bp + offset;

        assert_valid_stack_location("store: attempted to store with invalid VM configuration", store_location);

        *store_location = value;
}

void load_op ()
{
        int32_t offset = read_int32 ();

        int32_t *load_location = vm.bp + offset;

        assert_valid_stack_location("load: attempted to load with invalid VM configuration", load_location);

        push (*load_location);
}

void call_op ()
{
        int32_t addr = read_int32 ();

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

int initialize_and_run (int8_t *code, size_t code_size, int32_t entry_address)
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

int load_file_and_run (char *filename)
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

void parse_cmd (int argc, char **argv)
{
        struct option long_opts[] = {
                {"debug", no_argument, 0, '-d'}
        };
        int opt_idx, c;
        while ((c = getopt_long (argc, argv, "", long_opts, &opt_idx)) != -1) {
                switch (c) {
                case 'd': break;
                default: break;
                }
        }

        if (optind == argc) {
                fprintf (stderr, "error: no input files.");
                exit (EXIT_FAILURE);
        }

        load_file_and_run (argv[optind]);
}

int main (int argc, char **argv)
{
        parse_cmd (argc, argv);
        return 0;
}