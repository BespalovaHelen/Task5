CC = gcc
CFLAGS = -g -Wall
TARGET = myshell
OBJS = main.o parser.o executor.o utils.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: main.c parser.h executor.h utils.h
	$(CC) $(CFLAGS) -c main.c

parser.o: parser.c parser.h executor.h utils.h
	$(CC) $(CFLAGS) -c parser.c

executor.o: executor.c executor.h parser.h utils.h
	$(CC) $(CFLAGS) -c executor.c

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c utils.c

clean:
	rm -f $(OBJS) $(TARGET)

run: $(TARGET)
	./$(TARGET)


.PHONY: all clean run
