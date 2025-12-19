#ifndef EXECUTOR_H
#define EXECUTOR_H

/* Функции исполнителя */
int execution_line(char** arr, long long int *N);
int i_o_redirection(char** arr);
int is_there_conveyor(char*** arr);
int conveyor_n_processes(char** arr, long long int counter, int num_conv);
int is_there_background_process(char** arr, long long int N);

#endif
