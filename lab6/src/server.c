#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "pthread.h"
#include "libModule.h" 

// Инициализация мьютекса для синхронизации потоков
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

// Структура для хранения аргументов факториала
struct FactorialArgs {
  uint64_t begin; 
  uint64_t end; 
  uint64_t mod; 
};

// Функция для вычисления факториала в заданном диапазоне
uint64_t Factorial(const struct FactorialArgs *args) {
  uint64_t ans = 1;
  int start = args->begin; 
  int end = args->end; 
  int mod = args->mod; 

  pthread_mutex_lock(&mut); // Блокировка мьютекса перед началом вычислений

  for (int i = start; i <= end; i++) {
    ans = libModule(ans, i, mod); 
  }
  pthread_mutex_unlock(&mut);
  return ans;
}

// Функция потока для вычисления факториала
void *ThreadFactorial(void *args) {
  struct FactorialArgs *fargs = (struct FactorialArgs *)args; // Приведение типа аргументов
  return (void *)(uint64_t *)Factorial(fargs); // Возвращаем результат вычисления факториала
}

int main(int argc, char **argv) {
  int tnum = -1; // количество потоков
  int port = -1; // номер порта

  while (true) {
    static struct option options[] = {{"port", required_argument, 0, 0}, 
                                      {"tnum", required_argument, 0, 0}, 
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index); 

    if (c == -1)
      break;

    switch (c) {
    case 0: { 
      switch (option_index) {
      case 0: 
        port = atoi(optarg); 
        if (port <= 0) { 
          printf("Invalid arguments (port)!\n");
          exit(EXIT_FAILURE);
        }
        break;
      case 1: 
        tnum = atoi(optarg); 
        if (tnum <= 0) {
          printf("Invalid arguments (tnum)!\n");
          exit(EXIT_FAILURE);
        }
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?': 
      printf("Unknown argument\n"); 
      break; 
      
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (port == -1 || tnum == -1) {
    fprintf(stderr, "Using: %s --port 20001 --tnum 4\n", argv[0]);
    return 1;
  }

  // Создание сокета для сервера
  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    fprintf(stderr, "Can not create server socket!");
    return 1;
  }

  struct sockaddr_in server; // Структура для хранения информации о сервере
  server.sin_family = AF_INET; // Указываем IPv4
  server.sin_port = htons((uint16_t)port); // Устанавливаем порт сервера в сетевом порядке байтов
  server.sin_addr.s_addr = htonl(INADDR_ANY); // Устанавливаем адрес сервера на любой интерфейс

  int opt_val = 1;
  
   // Настройка параметров сокета для повторного использования адреса
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

  // Привязываем сокет к адресу и порту
  int err = bind(server_fd, (struct sockaddr *)&server, sizeof(server)); 
  if (err < 0) { 
    fprintf(stderr, "Can not bind to socket!"); // Если привязка не удалась, выводим сообщение об ошибке
    return 1; 
  }

  // Начинаем прослушивание входящих соединений на сокете
  err = listen(server_fd, 128); 
  if (err < 0) { 
    fprintf(stderr, "Could not listen on socket\n");
    return 1; 
  }

  printf("Server listening at %d\n", port); // Уведомляем о том, что сервер начал прослушивание на указанном порту

  while (true) { 
    struct sockaddr_in client; // Структура для хранения информации о клиенте
    socklen_t client_len = sizeof(client); // Длина структуры клиента

    // Принимаем входящее соединение от клиента
    int client_fd = accept(server_fd, (struct sockaddr *)&client, &client_len); 

    if (client_fd < 0) { 
      fprintf(stderr, "Could not establish new connection\n"); // Если соединение не удалось установить, выводим сообщение
      continue; // Переходим к следующему циклу для ожидания новых соединений
    }

    while (true) { 
      unsigned int buffer_size = sizeof(uint64_t) * 3; // Размер буфера для получения данных от клиента
      char from_client[buffer_size]; // Буфер для хранения данных от клиента

      // Получаем данные от клиента
      int read = recv(client_fd, from_client, buffer_size, 0); 

      if (!read) // Если ничего не прочитали (клиент закрыл соединение)
        break;
      if (read < 0) { 
        fprintf(stderr, "Client read failed\n");
        break; 
      }
      if (read < buffer_size) { 
        fprintf(stderr, "Client send wrong data format\n");
        break; 
      }

      pthread_t threads[tnum]; // Массив для хранения идентификаторов потоков

      uint64_t begin = 0; 
      uint64_t end = 0; 
      uint64_t mod = 0;

      // Извлечение значений begin, end и mod из полученных данных
      memcpy(&begin, from_client, sizeof(uint64_t)); 
      memcpy(&end, from_client + sizeof(uint64_t), sizeof(uint64_t)); 
      memcpy(&mod, from_client + sizeof(uint64_t) *2 , sizeof(uint64_t)); 

      fprintf(stdout, "Receive: %lu %lu %lu\n", (unsigned long)begin,
              (unsigned long)end,
              (unsigned long)mod);

      struct FactorialArgs args[tnum]; // Массив для хранения аргументов для потоков
      uint64_t step = (end - begin) / tnum;

      for (uint32_t i = 0; i < tnum; i++) { 
        args[i].mod = mod; 
        args[i].begin = begin + i * step;
        if (i == tnum - 1) { 
          args[i].end = end + 1; // Если это последний поток, устанавливаем конец диапазона на конец +1
        } else { 
          args[i].end = args[i].begin + step; // Иначе устанавливаем конец на основе шага
        }

      // Создаем новый поток для вычисления факториала
      if (pthread_create(&threads[i], NULL,
                         ThreadFactorial,
                         (void *)&args[i])) { 
        printf("Error: pthread_create failed!\n"); 
        return 1; 
      }
    }

    uint64_t total = 1; // Переменная для общего результата

    for (uint32_t i = 0; i < tnum; i++) { 
      uint64_t result = 0;
      pthread_join(threads[i], (void **)&result); // Ожидаем завершения потока и получаем результат
      total = libModule(total, result, mod); // Объединяем результаты с использованием функции libModule с модулем
    }

    printf("Total: %lu\n", total);
    char buffer[sizeof(total)];
    memcpy(buffer, &total, sizeof(total)); // Копируем общий результат в буфер

    err = send(client_fd, buffer, sizeof(total), 0); // Отправляем результат клиенту

    if (err < 0) { 
      fprintf(stderr,"Can't send data to client\n");
      break; 
    }
  }

  shutdown(client_fd, SHUT_RDWR); // Завершаем соединение с клиентом и закрываем сокет
  close(client_fd); // Закрываем дескриптор сокета клиента
  }
  return 0; 
}
