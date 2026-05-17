# Компиляторы и флаги
BISON = bison
CC    = gcc
CFLAGS = -Wall

# Цели по умолчанию — собрать обе программы
all: extent calc

# Задача 1: протяжение скобочной системы
extent: 1.c
	$(CC) $(CFLAGS) -o extent 1.c

1.c: 1.y
	$(BISON) -o 1.c 1.y

# Задача 2: калькулятор
calc: 2.c
	$(CC) $(CFLAGS) -o calc 2.c

2.c: 2.y
	$(BISON) -o 2.c 2.y

# Очистка
clean:
	rm -f 1.c 2.c extent calc

# Запуск
run-extent: extent
	./extent

run-calc: calc
	./calc

.PHONY: all clean run-extent run-calc
