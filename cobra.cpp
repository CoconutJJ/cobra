#include "bytecode.h"
#include "compiler.h"
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
        if (argc < 2) {
                fprintf (stderr, "error: no input files.");
                exit (EXIT_FAILURE);
        }

        FILE *fp = fopen (argv[1], "r");

        if (!fp) {
                perror ("fopen");
                exit (EXIT_FAILURE);
        }

        Compiler compiler (fp);

        Bytecode *bytes = compiler.compile ();

        if (!bytes)
                exit (EXIT_FAILURE);
        
        FILE *outfp = fopen("a.bin", "w");
        
        if (fwrite(bytes->chunk, bytes->count, 1, outfp) != 1) {
                fprintf(stderr, "fatal error: failed to write to out file");
                exit(EXIT_FAILURE);
        }
}