#include "bytecode.h"
#include "compiler.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

void parse_cmd (int argc, char **argv)
{
        struct option long_options[] = {
                {"debug", no_argument, 0, 'd'}
        };

        int c, option_index = 0;
        bool debug_mode = false;

        while ((c = getopt_long (argc, argv, "", long_options, &option_index)) != -1) {
                switch (c) {
                case 'd': debug_mode = true; break;
                default: break;
                }
        }

        if (optind == argc) {
                fprintf (stderr, "error: no input files");
                exit (EXIT_FAILURE);
        }

        FILE *fp = fopen (argv[optind], "r");

        if (!fp) {
                perror ("fopen");
                exit (EXIT_FAILURE);
        }

        Compiler compiler (fp);

        Bytecode *bytes = compiler.compile ();

        if (debug_mode) {
                bytes->dump_bytecode ();
                return;
        }

        FILE *outfp = fopen ("a.bin", "w");

        if (fwrite (bytes->chunk, bytes->count, 1, outfp) != 1) {
                fprintf (stderr, "fatal error: failed to write to out file");
                exit (EXIT_FAILURE);
        }
}

int main (int argc, char **argv)
{
        parse_cmd (argc, argv);
}