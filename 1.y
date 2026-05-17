%{
/* Программа вычисления протяжения скобочной системы */
#include <stdio.h>
%}

%%
P   : S               { printf("extent: %d\n", $1); }
    ;

S   : '(' S ')' S     { $$ = $4 + 1; }   /* протяжение = протяжение хвоста + 1 */
    | /* empty */     { $$ = 0; }        /* пустая цепочка -> 0 */
    ;
%%

int main(void) {
    printf("type a string: ");
    yyparse();
    return 0;
}

int yylex(void) {
    int c = getchar();
    if (c == '\n') return 0;   /* 0 — признак конца ввода для yacc */
    yylval = c;
    return c;
}

void yyerror(const char *s) {
    printf("Extent eval error: %s\n", s);
}
