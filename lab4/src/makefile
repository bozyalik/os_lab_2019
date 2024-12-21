# Указываем компилятор и флаги
CC = gcc
CFLAGS = -I.

# Указываем целевые программы
TARGETS = parallel_min_max process_memory

# Основная цель (по умолчанию собирает все программы)
all: parallel_min_max process_memory

# таргет для сборки параллельной программы
parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

# Правило для сборки объекта utils.o из utils.c
utils.o : utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

# Правило для сборки объекта find_min_max.o из find_min_max.c
find_min_max.o : utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

# Цель clean для удаления скомпилированных файлов и объектов
clean :
	rm utils.o find_min_max.o sequential_min_max parallel_min_max ex5
