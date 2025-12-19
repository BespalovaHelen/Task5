#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "parser.h"
#include "executor.h"
#include "utils.h"

/*------------------------ Ввод ------------------------*/
void InputAndPrint(FILE* FP_I, FILE* FP_O)
{
    long long int L = 0;
    int a;
    char** new_str = NULL;

    while (1)
    {
        // Удаляем зомби-процессы, не блокируя цикл
        while (waitpid(-1, NULL, WNOHANG) > 0);

        // Показываем приглашение только если это stdin
        if (FP_I == stdin)
        {
            char path[100];
            if (getcwd(path, sizeof(path)) != NULL) {
                fprintf(stdout, "%s: ", path);
            } else {
                fprintf(stdout, "shell: ");
            }
            fflush(stdout);
        }

        char* str = Inp_Str(&a, FP_I, &L);
        if (a == EOF && str == NULL)
            break;		// конец файла
        if (str == NULL) {
            free(str);
            continue;		// пустая строка
        }

        long long int newL = 0;
        new_str = Processing_String(str, L, &newL);
        prepare_for_execution(new_str, (int)newL);
        free_memory(new_str, newL);
        free(str);
        
        if (a == EOF)
            break;
        L = 0;
    }
}

/*------------------ Чтение строки -----------------*/

char* Inp_Str(int* a, FILE* FP_I, long long int* N)
{
    long long int L = 1;
    int symb;
    char* C = NULL;
    *a = 1;
    C = (char*)malloc(L * sizeof(char));
    if (C == NULL)
        return NULL;

    C[L-1] = '\0';

    while (1) {
        symb = fgetc(FP_I);
        if (symb == EOF) {
            *a = EOF;
            break;
        }
        if (symb == '\n')
            break;

        C[L-1] = symb;
        L++;

        char *tmp = (char*)realloc(C, L * sizeof(char));
        if (tmp == NULL) {
            free(C);
            printf("Ошибка выделения памяти\n");
            exit(1);
        }
        C = tmp;
        C[L-1] = '\0';
    }

    *(N) = L;
    return C;
}

/*------------------ Разбор строки ------------------*/
char** Processing_String(char* str, long long int L, long long int* newL)
{
    int counter = 1, word_cout = 1;

    char* word = (char*)malloc(1 * sizeof(char));
    if (!word) {
        perror("malloc");
        exit(1);
    }
    word[0] = '\0';

    char** new_arr = (char**)malloc(1 * sizeof(char*));
    if (!new_arr) {
        free(word);
        perror("malloc");
        exit(1);
    }
    new_arr[0] = NULL;

    int symb, pred_symb = 'f';
    int FLAG = 0;  // если внутри кавычек
    int FL_SC = 0; // если перед этим был спец. символ

    for (long long int i = 0; i < L; i++) {
        symb = str[i];
	
	if (symb == '"') {
            FLAG = 1 - FLAG;
            continue;
        }
	
	// Внутри кавычек - пишем всё подряд
        if (FLAG == 1) {
            word[word_cout-1] = symb;
            word_cout++;

            char *tmp = realloc(word, word_cout * sizeof(char));
            if (!tmp) { perror("realloc"); free(word); free(new_arr); exit(1); }
            word = tmp;

            word[word_cout-1] = '\0';
        }
	
	// Если до этого был спец.символ
        else if (FL_SC == 1) {
            FL_SC = Special_Characters(symb);

            if (FL_SC) {
                // двойные спецсимволы: &&, ||, >>, >>
                if ((symb == pred_symb) && (strchr(doubleSYMBOLS, symb))) {
                    word[word_cout-1] = symb;
                    word_cout++;

                    char *tmp = realloc(word, word_cout * sizeof(char));
                    if (!tmp) { perror("realloc"); free(word); free(new_arr); exit(1); }
                    word = tmp;

                    word[word_cout-1] = '\0';

                    new_arr[counter-1] = word;
                    counter++;

                    char **tmp_arr = realloc(new_arr, counter * sizeof(char*));
                    if (!tmp_arr) { perror("realloc"); free(word); free(new_arr); exit(1); }
                    new_arr = tmp_arr;

                    word = malloc(1 * sizeof(char));
                    if (!word) { free(new_arr); exit(1); }
                    word_cout = 1;
                    word[0] = '\0';

                    FL_SC = 0;
                    continue;
                }
                else {
                    // другой спецсимвол
                    new_arr[counter-1] = word;
                    counter++;
                    char **tmp_arr = realloc(new_arr, counter * sizeof(char*));
                    if (!tmp_arr) { free(word); free(new_arr); exit(1); }
                    new_arr = tmp_arr;

                    word = malloc(2 * sizeof(char));
                    if (!word) { free(new_arr); exit(1); }
                    word_cout = 2;
                    word[0] = symb;
                    word[1] = '\0';
                    pred_symb = symb;
                    continue;
                }
            }
            else {
                // предыдущий был спецсимвол, этот — нет
                new_arr[counter-1] = word;
                counter++;

                char **tmp_arr = realloc(new_arr, counter * sizeof(char*));
                if (!tmp_arr) { free(word); free(new_arr); exit(1); }
                new_arr = tmp_arr;

                if (!isspace(symb)) {
                    word = malloc(2 * sizeof(char));
                    if (!word) { free(new_arr); exit(1); }
                    word_cout = 2;
                    word[0] = symb;
                    word[1] = '\0';
                }
                else {
                    word = malloc(1 * sizeof(char));
                    if (!word) { free(new_arr); exit(1); }
                    word_cout = 1;
                    word[0] = '\0';
                }
                pred_symb = symb;
            }
        }

	// Если текущий символ - спец.символ
        else if ((FL_SC = Special_Characters(symb))) {
            if (word_cout > 1) {
                new_arr[counter-1] = word;
                counter++;
                char **tmp_arr = realloc(new_arr, counter * sizeof(char*));
                if (!tmp_arr) { free(word); free(new_arr); exit(1); }
                new_arr = tmp_arr;
            }
            else if (word) {
                free(word);
            }

            word = malloc(2 * sizeof(char));
            if (!word) { free(new_arr); exit(1); }

            word_cout = 2;
            word[0] = symb;
            word[1] = '\0';
            pred_symb = symb;
        }

	// Пробел вне кавычек
        else if (isspace(symb)) {
            if (word_cout > 1) {
                new_arr[counter-1] = word;
                counter++;

                char **tmp_arr = realloc(new_arr, counter * sizeof(char*));
                if (!tmp_arr) { free(word); free(new_arr); exit(1); }
                new_arr = tmp_arr;

                word = malloc(1 * sizeof(char));
		if (!word) { free(new_arr); exit(1); }
                word_cout = 1;
                word[0] = '\0';
                pred_symb = symb;
                continue;
            }
        }

	// Обычный символ
        else {
            word[word_cout-1] = symb;
            word_cout++;

            char *tmp = realloc(word, word_cout * sizeof(char));
            if (!tmp) { free(word); free(new_arr); exit(1); }
            word = tmp;

            word[word_cout-1] = '\0';
        }
    }

    // Последнее слово
    if (word && word[0] != '\0') {
        new_arr[counter-1] = word;
        counter++;
    } else {
        free(word);
        counter--; // Уменьшаем счетчик, если слово пустое
    }

    // Добавляем финальный NULL для execvp
    char **tmp_arr = realloc(new_arr, (counter + 1) * sizeof(char*));
    if (!tmp_arr) { free(new_arr); exit(1); }
    new_arr = tmp_arr;
    new_arr[counter] = NULL;

    *newL = counter;
    return new_arr;
}

/*------------ Проверка спец.символа ------------*/
int Special_Characters(int symb)
{
    char* F = strchr(SYMBOLS, symb);
    return (F != NULL);
}

/*--------------- Удаление пустых элементов --------------*/
char **compact_args(char **arr, long long int *N)
{
    long long int w = 0;
    for (long long int i = 0; i < *N; i++) {
        if (arr[i] && arr[i][0] != '\0') {
            arr[w++] = arr[i];
        } else if (arr[i]) {
            free(arr[i]);	// освобождаем пустую строку
            arr[i] = NULL;
        }
    }
    arr[w] = NULL;
    *N = w;
    return arr;
}

/*----------------ПОСЛЕДНЯЯ ЧАСТЬ SHELL----------------*/
void prepare_for_execution(char** commands_line, int number_of_words)
{
    //отлавливаем завершение фонового процесса
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

    if (number_of_words == 0 || !commands_line[0])
        return;

    //создаем копию commands_line для безопасной работы
    char*** commands = calloc(number_of_words, sizeof(char**));
    if (!commands) return;
    
    char** separators = calloc(number_of_words, sizeof(char*));
    if (!separators) { free(commands); return; }
    
    long long int* n_commands_cur = calloc(number_of_words, sizeof(long long int));
    if (!n_commands_cur) { free(commands); free(separators); return; }
    
    //инициализируем нулями
    for (int i = 0; i < number_of_words; i++) {
        n_commands_cur[i] = 0;
    }
    
    int n_commands = 0, n_separators = 0;
    int n_opened_brackets = 0;

    //разделение команд по ; && ||
    int cmd_start = 0;
    for (int i = 0; i < number_of_words; i++) {
        if (!commands_line[i]) break;
        
        if (strcmp(commands_line[i], "(") == 0) {
            n_opened_brackets++;
        }
        if (strcmp(commands_line[i], ")") == 0) {
            n_opened_brackets--;
        }
        
        //если разделитель НЕ внутри скобок
        if ((strcmp(commands_line[i], "||") == 0 || 
             strcmp(commands_line[i], "&&") == 0 ||
             strcmp(commands_line[i], ";") == 0) && n_opened_brackets == 0) {
            
            //выделяем память для текущей команды
            commands[n_commands] = calloc(number_of_words, sizeof(char*));
            if (!commands[n_commands]) {
                for (int j = 0; j < n_commands; j++) free(commands[j]);
                free(commands); free(separators); free(n_commands_cur);
                return;
            }
            
            //копируем слова команды
            int k = 0;  //кол-во слов
            for (int j = cmd_start; j < i; j++) {
                commands[n_commands][k++] = commands_line[j];
            }
            n_commands_cur[n_commands] = k;
            
            //сохраняем разделитель
            separators[n_separators++] = commands_line[i];
            
            //начинаем новую команду
            cmd_start = i + 1;
            n_commands++;
        }
    }
    
    //последняя команда (после последнего разделителя или если разделителей нет)
    commands[n_commands] = calloc(number_of_words, sizeof(char*));
    if (!commands[n_commands]) {
        for (int j = 0; j < n_commands; j++) free(commands[j]);
        free(commands); free(separators); free(n_commands_cur);
        return;
    }
    
    int k = 0;
    for (int j = cmd_start; j < number_of_words && commands_line[j]; j++) {
        commands[n_commands][k++] = commands_line[j];
    }
    n_commands_cur[n_commands] = k;
    n_commands++;  //порядок команды 

    //запускаем все команды в нужном порядке, учитывая разделители
    int last_status = 0;
    for (int i = 0; i < n_commands; i++) {
        //создаем копию команды для execution_line
        char** cmd_copy = malloc((n_commands_cur[i] + 1) * sizeof(char*));
        if (!cmd_copy) {
            for (int j = 0; j < n_commands; j++) free(commands[j]);
            free(commands); free(separators); free(n_commands_cur);
            return;
        }
        
        for (int j = 0; j < n_commands_cur[i]; j++) {
            cmd_copy[j] = commands[i][j];
        }
        cmd_copy[n_commands_cur[i]] = NULL;
        
        long long int cmd_len = n_commands_cur[i];
        last_status = execution_line(cmd_copy, &cmd_len);
        
        //освобождаем копию команды (строки не освобождаем - они принадлежат оригинальному массиву)
        free(cmd_copy);
        
        if (i == n_commands - 1) {
            continue;  //последняя
        }

        //логика для операторов || и &&
        if (last_status == 0 && separators[i] && !strcmp(separators[i], "||")) {
            //успех и оператор || - пропускаем следующую команду
            i++; // пропускаем следующую команду
        } else if (last_status != 0 && separators[i] && !strcmp(separators[i], "&&")) {
            //неудача и оператор && - пропускаем следующую команду
            i++; // пропускаем следующую команду
        }
        //оператор ; всегда запускает следующую команду
    }

    //освобождаем только структуры управления
    free(n_commands_cur);
    free(separators);
    for (int i = 0; i < n_commands; i++) {
        free(commands[i]);
    }
    free(commands);
}

void rid_of_brackets(char*** commands, int* n_commands, int offset) {  //int offset - позиция начала скобок
    if (offset < *n_commands && (*commands)[offset]) {
        //НЕ освобождаем здесь - строка принадлежит оригинальному массиву
        (*commands)[offset] = NULL;  //вместо "("
    }
    
    int j = 1 + offset;  //сдвигаем все элементы после скобки
    while (j < *n_commands && (*commands)[j]) {
        (*commands)[j-1] = (*commands)[j];
        j++;
    }
    
    if (j - 1 < *n_commands) {
        (*commands)[j-1] = NULL;  //NULL на последнюю позицию (вместо дублирующегося последнего элемента)
    }
    
    if (j >= 2 && (*commands)[j-2] && strcmp((*commands)[j-2], ")") == 0) {
        //НЕ освобождаем здесь - строка принадлежит оригинальному массиву
        (*commands)[j-2] = NULL;  //вместо ")"
    }
    
    //создаем новый массив для содержимого скобок
    int new_size = *n_commands - 2 - offset;  //размер содержимого скобок
    if (new_size <= 0) return;
    
    char** new_cmds = malloc((new_size + 1) * sizeof(char*));
    if (!new_cmds) return;
    
    for (int i = 0; i < new_size; i++) {
        new_cmds[i] = (*commands)[offset + i];
    }
    new_cmds[new_size] = NULL;  //на последнее место
    
    prepare_for_execution(new_cmds, new_size);  //вызывает для содержимого скобок рекурсивно
    
    //после выхода из рекурсии команда продолжается
    
    free(new_cmds);
}
