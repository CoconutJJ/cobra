
#include "bytecode.h"
#include "compiler.h"
#include "vm.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#define DEBUG_MODE        0
#define EXEC_MODE         1
#define VERBOSE           2
#define SET_OPTION(opt)   (options |= (1 << (opt)))
#define OPTION_ISSET(opt) (options & (1 << (opt)))
int32_t options = 0;

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

        if (OPTION_ISSET (VERBOSE))
                vm.verbose = true;

        vm.load_file_and_run (filename);
}

void parse_cmd (int argc, char **argv)
{
        struct option long_options[] = {
                {  "debug",       no_argument, 0, 'd'},
                {   "exec",       no_argument, 0, 'e'},
                {"verbose",       no_argument, 0, 'v'},
                { "output", required_argument, 0, 'o'},
                {     NULL,                 0, 0,   0}
        };

        int c, option_index = 0;

        char *outfile_name = NULL;

        while ((c = getopt_long (argc, argv, "devo:", long_options, &option_index)) != -1) {
                switch (c) {
                case 'd': SET_OPTION (DEBUG_MODE); break;
                case 'e': SET_OPTION (EXEC_MODE); break;
                case 'v': SET_OPTION (VERBOSE); break;
                case 'o': outfile_name = optarg; break;
                default: break;
                }
        }

        if (optind == argc) {
                fprintf (stderr, "error: no input files");
                exit (EXIT_FAILURE);
        }

        if (OPTION_ISSET (DEBUG_MODE))
                debug (argv[optind]);
        else if (OPTION_ISSET (EXEC_MODE))
                exec (argv[optind]);
        else
                compile (argv[optind], outfile_name ? outfile_name : "a.bin");
}

int main (int argc, char **argv)
{
        parse_cmd (argc, argv);
}