#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "executor.h"
#include "parser.h"
#include "utils.h"

/*--------------------- Выполнение ----------------------*/
int execution_line(char** arr, long long int *N_ptr)
{
    // Убираем зомби от предыдущих фоновых процессов
    int status;
    pid_t pid_bg_pr = waitpid(-1, &status, WNOHANG);
    if (pid_bg_pr > 0) {
        if (WIFEXITED(status)) {
            printf("-----Процесс %d завершился с кодом %d-----\n",
                   pid_bg_pr, WEXITSTATUS(status));
        } else {
            printf("-----Процесс %d прерван сигналом %d-----\n",
                   pid_bg_pr, WTERMSIG(status));
        }
    }

    long long int N = *N_ptr;
    if (N <= 0 || !arr)
        return 1;

    // Проверяем фоновые процессы (&)
    int num_amp = is_there_background_process(arr, N);
    if (num_amp > 1) {
        fprintf(stderr, "Ошибка: более одного '&'\n");
        return -1;
    }

    // Компактируем аргументы
    arr = compact_args(arr, &N);
    *N_ptr = N;

    if (N == 0)
        return 1;

    // Встроенная команда cd
    if (strcmp(arr[0], "cd") == 0) {
        if (N == 1) {
            char *home = getenv("HOME");
            if (!home) home = "/";
            if (chdir(home) != 0)
                perror("cd");
        } else {
            if (chdir(arr[1]) != 0)
                perror("cd");
        }
        return 0;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    } else if (pid == 0) {
        // Дочерний процесс
        signal(SIGINT, SIG_DFL);

        // Фоновый режим - перенаправляем ввод
        if (num_amp) {
            signal(SIGINT, SIG_IGN);
            int fd = open("/dev/null", O_RDWR);
            if (fd != -1) {
                dup2(fd, 0);
                close(fd);
            }
        }

        // Перенаправление ввода-вывода
        if (i_o_redirection(arr))
            exit(2);

        // Конвейер
        int num_conv = is_there_conveyor(&arr);
        if (num_conv == 1) {
            // Скобки
            if (arr[0] && strcmp(arr[0], "(") == 0) {
                int temp_N = (int)N;
                rid_of_brackets(&arr, &temp_N, 0);
                while (wait(NULL) != -1);
                exit(0);
            } else {
                execvp(arr[0], arr);
            }
        } else if (num_conv > 1) {
            conveyor_n_processes(arr, N, num_conv);
        } else {
            execvp(arr[0], arr);
        }
	perror("execvp");
        exit(1);
    }

    // Родительский процесс
    if (!num_amp) {
        waitpid(pid, &status, 0);
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;  // если нормально завершился - статус, иначе -1
    }

    return 0;
}

/*--------------- Перенаправление потоков ----------------*/
int i_o_redirection(char** arr)
{
    char* word = " ";
    char* pred_word = " ";

    for (int i = 0; arr[i] != NULL; i++) {
        word = arr[i];

        if (strcmp(pred_word, "<") == 0) {
            int fd = open(word, O_RDONLY);
            if (fd == -1) {
                perror("Error: open file\n");
                return 1;
            }
            dup2(fd, 0);
            close(fd);

            // Удаляем "<" и имя файла из аргументов
            int j = i;
            while (arr[j] != NULL) {
                arr[j-1] = arr[j+1];
                j++;
            }
            arr[j-1] = arr[j-2] = NULL;
            i--;
        }

	// > файл (перезапись)
        if (strcmp(pred_word, ">") == 0) {
            int fd = open(word, O_WRONLY | O_CREAT | O_TRUNC,
                         S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd == -1) {
                perror("Error: open or creat file\n");
                return 1;
            }
            dup2(fd, 1);
            close(fd);

            int j = i;
            while (arr[j] != NULL) {
                arr[j-1] = arr[j+1];
                j++;
            }
            arr[j-1] = arr[j-2] = NULL;
            i--;
        }
	
	// >> файл (дозапись)
        if (strcmp(pred_word, ">>") == 0) {
            int fd = open(word, O_WRONLY | O_CREAT | O_APPEND,
                         S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (fd == -1) {
                perror("Error: open or creat file\n");
                return 1;
            }
            dup2(fd, 1);
            close(fd);

            int j = i;
            while (arr[j] != NULL) {
                arr[j-1] = arr[j+1];
                j++;
            }
            arr[j-1] = arr[j-2] = NULL;
            i--;
        }

        pred_word = word;
    }
    return 0;
}

/*--------------- Проверка конвейера --------------*/
int is_there_conveyor(char*** arr)
{
    int N = 1;
    for (int i = 0; (*arr)[i] != NULL; i++) {
        if (strcmp((*arr)[i], "|") == 0) {
            N++;
            (*arr)[i] = NULL;
        }
    }
    return N;
}

/*--------------- Выполнение конвейера ---------------*/
int conveyor_n_processes(char** arr, long long int counter, int num_conv)
{
    int fd[2], prev_fd = 0;
    int start_idx = 0;

    for (int i = 0; i < num_conv; i++) {
        // Создаем pipe для всех команд кроме последней
        if (i < num_conv - 1) {
            if (pipe(fd) < 0) {
                perror("pipe");
                exit(1);
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            // Дочерний процесс
            // Перенаправляем ввод с предыдущего pipe
            if (i > 0) {
                dup2(prev_fd, 0);
                close(prev_fd);
            }

            // Перенаправляем вывод в следующий pipe (если НЕ последняя команда)
            if (i < num_conv - 1) {
                close(fd[0]);
                dup2(fd[1], 1);
                close(fd[1]);
            }

            // Находим конец текущей команды
            int end_idx = start_idx;
            while (arr[end_idx] != NULL && end_idx < counter) {
                end_idx++;
            }

            // Создаем массив аргументов для текущей команды
            char** cmd_args = malloc((end_idx - start_idx + 1) * sizeof(char*));
            if (!cmd_args) {
                perror("malloc");
                exit(1);
            }

            int k = 0;
            for (int j = start_idx; j < end_idx; j++) {
                cmd_args[k++] = arr[j];
            }
            cmd_args[k] = NULL;

            //скобки
            if (cmd_args[0] && strcmp(cmd_args[0], "(") == 0) {
                int temp = end_idx - start_idx;
	        rid_of_brackets(&cmd_args, &temp, 0);
                while (wait(NULL) != -1);
                exit(0);
            } else if (cmd_args[0]) {
                //выполняем команду
                execvp(cmd_args[0], cmd_args);
            }

            perror("execvp in conveyor");
            free(cmd_args);
            exit(1);
        }

        // Родительский процесс
        if (i > 0) close(prev_fd);

        if (i < num_conv - 1) {
            close(fd[1]);
            prev_fd = fd[0];
        }

        // Находим начало следующей команды
        int temp = start_idx;
        while (arr[temp] != NULL && temp < counter) temp++;
        start_idx = temp + 1;

        // Ожидаем завершения всех дочерних процессов
        if (i == num_conv - 1) {
            while (wait(NULL) > 0);
        }
    }

    exit(0);
}

/*------------- Обработка фонового режима -------------*/
int is_there_background_process(char** arr, long long int N)
{
    int num_amp = 0;
    for (long long int i = 0; i < N; i++) {
        if (arr[i] && strcmp(arr[i], "&") == 0) {
            num_amp++;
            //НЕ освобождаем память - она принадлежит оригинальному массиву
            //просто помечаем как NULL для compact_args
            arr[i] = NULL;
        }
    }
    return num_amp;
}
