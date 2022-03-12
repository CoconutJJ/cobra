#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"

int main (int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "error: no input files.");
        exit(EXIT_FAILURE);
    }

    FILE * fp = fopen(argv[1], "r");

    if (!fp) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    Compiler compiler(fp);

    compiler.compile();


}