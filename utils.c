#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

void free_memory(char** arr, long long int N)
{
    if (!arr) return;
    for (long long int i = 0; i < N; i++) {
        if (arr[i]) free(arr[i]);
    }
    free(arr);
}

/*-------------------- Печать массива --------------------*/
void Print_String(char** arr, long long int N, FILE* FP_O)
{
    fprintf(FP_O, "\n");
    for (long long int i = 0; i < N; i++) {
        fprintf(FP_O, "%s\n", arr[i]);
    }
}
