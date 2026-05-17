%{
/* Калькулятор: +, -, *, /, скобки, целые числа */
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

int yylex(void);
void yyerror(const char *);
%}


%token NUMBER

/* Приоритеты */
%left '+' '-'
%left '*' '/'
%left NEG   /* унарный минус */

%%

P       : expr              { printf("result: %d\n", $1); }
        ;

expr    : expr '+' expr     { $$ = $1 + $3; }
        | expr '-' expr     { $$ = $1 - $3; }
        | expr '*' expr     { $$ = $1 * $3; }
        | expr '/' expr     {
                                if ($3 == 0) {
                                    yyerror("division by zero");
                                    $$ = 0;
                                } else {
                                    $$ = $1 / $3;
                                }
                            }
        | '(' expr ')'      { $$ = $2; }
        | '-' expr %prec NEG{ $$ = -$2; }   /* унарный минус */
        | NUMBER            { $$ = $1; }
        ;

%%

int main(void) {
    printf("enter expression:\n");
    while(1){
	printf("> ");
	if (yyparse()!=0)
	    break;
    }
    return 0;
}

int yylex(void) {
    int c;

    /* пропускаем пробелы */
    while ((c = getchar()) == ' ' || c == '\t');

    if (c == EOF || c == '\n')
        return 0;

    if (isdigit(c)) {
        int val = 0;
        while (isdigit(c)) {
            val = val * 10 + (c - '0');
            c = getchar();
        }
        ungetc(c, stdin);        /* возвращаем не-цифру */
        yylval = val;
        return NUMBER;
    }

    /* операторы и скобки возвр.как есть */
    return c;
}

void yyerror(const char *s) {
    fprintf(stderr, "Error: %s\n", s);
}
