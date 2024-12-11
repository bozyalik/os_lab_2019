#include <stdint.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>


typedef struct {
  int begin;
  int end;
} Factthread;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int factorial_value = 1;

void *fact(void *args) {

  Factthread *f_part = (Factthread *) args;
  int res = 1;

  for (int i = f_part->begin; i < f_part->end; i++)
    res *= (i + 1);

  pthread_mutex_lock(&mutex);
  factorial_value *= res;
  pthread_mutex_unlock(&mutex);

}

int main(int argc, char **argv) {

  uint32_t k = -1;
  uint32_t threads_num = -1;
  uint32_t mod = -1;

  while (1) {
    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"pnum", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "?", options, &option_index);

    if (c == -1) break;

    switch (c) {
      case 0:
        switch (option_index) {
          case 0:
          k = atoi(optarg);
            if (k < 0) {
              printf("Число должно быть положительным\n");
              return 1;
            }
            break;
          case 1:
          threads_num = atoi(optarg);
            if (threads_num < 0) {
              printf("Количество потоков должно быть больше 0\n");
              return 1;
            }
            break;
          case 2:
          mod = atoi(optarg);
            if (mod < 0) {
              printf("Модуль должен быть неотрицательным\n");
              return 1;
            }
            break;
          default:
          printf("Index %d is out of options\n", option_index);
        }
        break;

      case '?':break;

      default:
      printf("getopt returned character code 0%o?\n", c);
    }
  }

  if (optind < argc) {
    printf("Has at least one no option argument\n");
    return 1;
  }

  if (k == -1 || threads_num == -1 || mod == -1) {
    printf("Usage: %s --k \"num\" --pnum \"num\" --mod \"num\" \n",
           argv[0]);
    return 1;
  }
  int step = 0;
  if (k>threads_num)
    step = k / threads_num;
  else
    step = 1;

  pthread_mutex_init(&mutex, NULL);
  pthread_t threads[threads_num];
  Factthread factor[threads_num];

  for (uint32_t i = 0; i < threads_num; i++) {
    factor[i].begin = step * i;
    factor[i].end = (i + 1) * step;
    if (pthread_create(&threads[i], NULL, fact, (void *) &factor[i])) {
      perror("\nERROR CREATE THREAD\n");
      return 1;
    }
  }

  for (uint32_t i = 0; i < threads_num; i++) {
    pthread_join(threads[i], NULL);
  }
  pthread_mutex_destroy(&mutex);
  printf("Факториал %i равен %i.\n", k, factorial_value);
  printf("Факториал %i по модулю %i равен %i.\n", k, mod, factorial_value % mod);

  return 0;
  
}