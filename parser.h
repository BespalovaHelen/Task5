#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#define SYMBOLS "&|><;()"
#define doubleSYMBOLS "&|>"

/* Функции парсера */
void InputAndPrint(FILE* FP_I, FILE* FP_O);
char* Inp_Str(int* a, FILE* FP_I, long long int* N);
char** Processing_String(char* str, long long int L, long long int* newL);
int Special_Characters(int symb);
char **compact_args(char **arr, long long int *N);
void prepare_for_execution(char** commands_line, int number_of_words);
void rid_of_brackets(char*** commands, int* number_of_commands, int offset);

#endif
