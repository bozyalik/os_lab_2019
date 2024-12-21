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
#include "libModule.h"

// Структура для хранения информации о сервере
struct Server {
  char ip[255]; // IP-адрес сервера
  int port;     // Порт сервера
};

// Функция для проверки существования файла
bool is_file_exist(const char *fileName) {
  FILE *file;
  if ((file = fopen(fileName, "r"))) {
      fclose(file); 
      return true; 
  } else {
      return false;
  }   
}

// Функция для конвертации строки в uint64_t
bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10); // Преобразуем строку в число
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0) // Проверка на другие ошибки
    return false;

  *val = i;
  return true;
}

int main(int argc, char **argv) {
  uint64_t k = -1; 
  uint64_t mod = -1; 
  char servers[255] = {'\0'}; // Массив для хранения имени файла с серверами

  while (true) {
    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index); 

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k); 
        if (k <= 0) { 
          printf("Invalid arguments (k)!\n");
          exit(EXIT_FAILURE);
        }
        break;
      case 1: 
        ConvertStringToUI64(optarg, &mod);
        if (mod <= 0) {
          printf("Invalid arguments (mod)!\n");
          exit(EXIT_FAILURE);
        }
        break;
      case 2:
        if (is_file_exist(optarg)) { // Проверка существования файла с серверами
          memcpy(servers, optarg, strlen(optarg)); // Копируем имя файла в массив servers
        } else {
          printf("Invalid arguments (servers)!\n");
          exit(EXIT_FAILURE);
        }
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?': 
      printf("Arguments error\n"); 
      break; 
      
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  //обязательные параметры
  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n", argv[0]);
    return 1;
  }

  unsigned int servers_num = 0; 
  FILE* fp;
  
  fp = fopen(servers, "r"); //серверы для чтения
  
  while (!feof(fp)) { 
    char test1[255]; 
    char test2[255]; 
    fscanf(fp, "%s : %s\n", test1, test2); // Читаем строки из файла и увеличиваем счетчик серверов
    servers_num++; 
  }

  struct Server *to = malloc(sizeof(struct Server) * servers_num); 
  fseek(fp, 0L, SEEK_SET);

  int index = 0; 
  
  while (!feof(fp)) { 
    fscanf(fp, "%s : %d\n", to[index].ip, &to[index].port); // Читаем IP и порт каждого сервера из файла
    printf("ip: %s, port: %d\n", to[index].ip, to[index].port); 
    index++; 
  }
  
  fclose(fp); 

  int factorial_part = k / servers_num; // Вычисляем размер части факториала для каждого сервера
  uint64_t result = 1; 

  // Цикл для подключения к каждому серверу
  for (int i = 0; i < servers_num; i++) {
    // Получаем информацию о сервере по IP
    struct hostent *hostname = gethostbyname(to[i].ip); 

    // Проверяем, удалось ли получить информацию о сервере
    if (hostname == NULL) { 
      fprintf(stderr, "gethostbyname failed with %s\n", to[i].ip); 
      exit(1); 
    }

    // хранение информации о сервере
    struct sockaddr_in server; 
    server.sin_family = AF_INET; // Указываем, что используем IPv4
    server.sin_port = htons(to[i].port); // Устанавливаем порт сервера в сетевом порядке байтов
    server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr); // Устанавливаем IP-адрес сервера

    // Создаем сокет для подключения к серверу
    // Сокет — программный интерфейс, который позволяет устанавливать связь между двумя устройствами в сети и обмениваться данными. 
    // Он представляет собой комбинацию IP-адреса и номера порта, что обеспечивает уникальность подключения. 
    // Сокеты используются в клиент-серверной модели, где клиент отправляет запросы на сервер, а сервер отвечает на эти запросы.
    int sck = socket(AF_INET, SOCK_STREAM, 0); 
   
    // Проверяем, удалось ли создать сокет
    if (sck < 0) { 
      fprintf(stderr, "Socket creation failed!\n"); 
      exit(1);
    }

    // Подключаемся к серверу
    if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) { 
      fprintf(stderr, "Connection failed\n"); 
      exit(1); 
    }

    // начало и конец диапазона для текущего потока
    uint64_t begin = (i * factorial_part) + 1; 
    uint64_t end;

    if (i != servers_num - 1) { 
      end = (i + 1) * factorial_part; // Если это не последний сервер, устанавливаем конец диапазона
    } else { 
      end = k; // Для последнего сервера устанавливаем конец равным k
    }

    // Создаем массив для передачи задачи на сервер
    char task[sizeof(uint64_t) * 3]; 

    // Копируем значения начала, конца и модуля в массив задачи
    memcpy(task, &begin, sizeof(uint64_t)); 
    memcpy(task + sizeof(uint64_t), &end, sizeof(uint64_t)); 
    memcpy(task + sizeof(uint64_t) * 2, &mod, sizeof(uint64_t)); 

    // Отправляем задачу на сервер
    if (send(sck , task , sizeof(task), 0) < 0){ 
      fprintf(stderr,"Send failed\n"); 
      exit(1); 
    }

    // Массив для получения ответа от сервера
    char response[sizeof(uint64_t)];

    // Получаем ответ от сервера
    if(recv(sck,response,sizeof(response),0) < 0){ 
      fprintf(stderr,"Receive failed\n"); 
      exit(1); // Завершаем программу в случае ошибки получения ответа
    }

    uint64_t answer = 0; 

    // Копируем ответ из массива response в переменную answer
    memcpy(&answer,response,sizeof(uint64_t)); 

    // Объединяем результат с использованием функции libModule
    result = libModule(result , answer , mod ); 
    close(sck);
  }

  printf("answer: %lu\n", result ); 
  free(to ); 
  return 0; 
}
