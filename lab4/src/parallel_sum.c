#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <getopt.h>
#include "utils.h"  // Библиотека для генерации массива
#include "sum.h"  // Библиотека для подсчета суммы

// Функция получает аргументы (структуру SumArgs из sum.h) и вызывает функцию sum
void *ThreadSum(void *args) {
    struct SumArgs *sum_args = (struct SumArgs *)args;
    return (void *)(size_t)Sum(sum_args);
}

int main(int argc, char **argv) {
    int seed = -1;
    int array_size = -1;
    int threads_num = -1;

    static struct option options[] = {
        {"seed", required_argument, 0, 0},
        {"array_size", required_argument, 0, 0},
        {"threads_num", required_argument, 0, 0},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;
    while ((c = getopt_long(argc, argv, "", options, &option_index)) != -1) {
        switch (c) {
            case 0:
                switch (option_index) {
                    case 0: seed = atoi(optarg); break;
                    case 1: array_size = atoi(optarg); break;
                    case 2: threads_num = atoi(optarg); break;
                    default: printf("Index %d is out of options\n", option_index); break;
                }
                break;
            case '?':
                break;
            default:
                printf("getopt returned character code 0%o?\n", c);
                break;
        }
    }

    if (seed == -1 || array_size == -1 || threads_num == -1) {
        printf("Usage: %s --seed <num> --array_size <num> --threads_num <num>\n", argv[0]);
        return 1;
    }

    // Генерация массива
    int *array = malloc(sizeof(int) * array_size);
    GenerateArray(array, array_size, seed);

    // Начало времени подсчета суммы
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);

    // Разделяем работу между потоками
    pthread_t threads[threads_num]; // хранит индефикаторы потоков
    struct SumArgs args[threads_num]; // хранит структуры SumArgs для каждого потока
    int chunk_size = array_size / threads_num;

    // данные как из структуры
    for (uint32_t i = 0; i < threads_num; i++) {
        args[i].array = array; 
        args[i].begin = i * chunk_size; 
        if (i == threads_num - 1) 
          args[i].end = array_size;
        else 
          args[i].end = (i + 1) * chunk_size;
        
        // создание нового потока для подсчета суммы в ThreadSum
        if (pthread_create(&threads[i], NULL, ThreadSum, (void *)&args[i])) {
            printf("Error: pthread_create failed!\n");
            return 1;
        }
    }

    // ожидание завершения всех потоков и подсчет суммы.
    int total_sum = 0;
    for (uint32_t i = 0; i < threads_num; i++) {
        int sum = 0;
        pthread_join(threads[i], (void **)&sum);
        total_sum += sum;
    }

    // время
    gettimeofday(&end_time, NULL);
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000.0;
    elapsed_time += (end_time.tv_usec - start_time.tv_usec) / 1000.0;

    free(array);

    printf("Total sum: %d\n", total_sum);
    printf("Elapsed time: %f ms\n", elapsed_time);

    return 0;
}
