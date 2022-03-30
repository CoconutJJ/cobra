
#include "bytecode.h"
#include "compiler.h"
#include "vm.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

void compile (const char *filename, const char *outfile)
{
        FILE *fp = fopen (filename, "r");

        if (!fp) {
                perror ("fopen");
                exit (EXIT_FAILURE);
        }

        Compiler compiler (fp);

        Function *bytes = compiler.compile ();

        FILE *outfp = fopen (outfile, "w");

        if (fwrite (bytes->bytecode->chunk, bytes->bytecode->count, 1, outfp) != 1) {
                fprintf (stderr, "fatal error: failed to write to out file");
                exit (EXIT_FAILURE);
        }
        fclose (outfp);
}

void debug (char *filename)
{
        FILE *fp = fopen (filename, "r");

        if (!fp) {
                perror ("fopen");
                exit (EXIT_FAILURE);
        }

        Compiler compiler (fp);

        Function *bytes = compiler.compile ();

        bytes->bytecode->dump_bytecode ();
}

void exec (char *filename)
{
        VM vm;

        vm.load_file_and_run (filename);
}

void parse_cmd (int argc, char **argv)
{
        struct option long_options[] = {
                {"debug",       no_argument, 0, 'd'},
                { "exec", no_argument, 0, 'e'},
                {   NULL,                 0, 0,   0}
        };

        int c, option_index = 0;
        bool debug_mode = false, exec_mode = false;

        while ((c = getopt_long (argc, argv, "", long_options, &option_index)) != -1) {
                switch (c) {
                case 'd': debug_mode = true; break;
                case 'e': exec_mode = true; break;
                default: break;
                }
        }

        if (optind == argc) {
                fprintf (stderr, "error: no input files");
                exit (EXIT_FAILURE);
        }

        if (debug_mode)
                debug (argv[optind]);
        else if (exec_mode)
                exec (argv[optind]);
        else
                compile (argv[optind], "a.bin");
}

int main (int argc, char **argv)
{
        parse_cmd (argc, argv);
}