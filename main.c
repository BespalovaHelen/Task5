#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "parser.h"
#include "executor.h"
#include "utils.h"

int main(int argc, char **argv)
{
    signal(SIGINT, SIG_IGN);
    FILE* FP_O = stdout;
    FILE* FP_I;

    if (argc > 1) {
        char* f_i = argv[1];
        FP_I = fopen(f_i, "r");
        if (!FP_I) {
            perror("fopen");
            return 1;
        }
    } else {
        FP_I = stdin;
    }

    InputAndPrint(FP_I, FP_O);

    if (FP_I != stdin) {
        fclose(FP_I);
    }

    return 0;
}
