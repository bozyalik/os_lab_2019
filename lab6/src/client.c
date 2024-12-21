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

#include <pthread.h>
#include <limits.h>

struct Server {
  char ip[255];
  int port;
};

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
  uint64_t result = 0;
  a = a % mod;
  while (b > 0) {
    if (b % 2 == 1)
      result = (result + a) % mod;
    a = (a * 2) % mod;
    b /= 2;
  }
  return result % mod;
}

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }
  if (errno != 0)
    return false;
  *val = i;
  return true;
}

struct TaskArgs {
  struct Server server;
  uint64_t k;
  uint64_t mod;
};

void* SendTaskToServer(void* arg) {
  struct TaskArgs* task_args = (struct TaskArgs*)arg;
  struct Server* server = &task_args->server;
  uint64_t k = task_args->k;
  uint64_t mod = task_args->mod;

  struct hostent *hostname = gethostbyname(server->ip);
  if (hostname == NULL) {
    fprintf(stderr, "gethostbyname failed with %s\n", server->ip);
    return NULL;
  }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(server->port);
  server_addr.sin_addr.s_addr = *((unsigned long *)hostname->h_addr);

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    fprintf(stderr, "Socket creation failed!\n");
    return NULL;
  }

  if (connect(sck, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    fprintf(stderr, "Connection failed!\n");
    return NULL;
  }

  uint64_t begin = 1;
  uint64_t end = k;

  char task[sizeof(uint64_t) * 3];
  memcpy(task, &begin, sizeof(uint64_t));
  memcpy(task + sizeof(uint64_t), &end, sizeof(uint64_t));
  memcpy(task + 2 * sizeof(uint64_t), &mod, sizeof(uint64_t));

  if (send(sck, task, sizeof(task), 0) < 0) {
    fprintf(stderr, "Send failed!\n");
    return NULL;
  }

  char response[sizeof(uint64_t)];
  if (recv(sck, response, sizeof(response), 0) < 0) {
    fprintf(stderr, "Receive failed!\n");
    return NULL;
  }

  uint64_t answer = 0;
  memcpy(&answer, response, sizeof(uint64_t));
  close(sck);

  return (void*)answer;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {'\0'}; // TODO: explain why 255

  while (true) {
    int current_optind = optind ? optind : 1;

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
          printf("k is a positive number.\n");
          return 1;
        }
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        if (mod <= 0) {
          printf("mod is a positive number.\n");
          return 1;
        }
        break;
      case 2:
        if (access(optarg, F_OK) == -1) {
          fprintf(stderr, "file does not exist: %s\n", optarg);
          return 1;
        }
        memcpy(servers, optarg, strlen(optarg));
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

  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }
  
  FILE* file = fopen(servers, "r");
  if (file == NULL) {
    perror("Failed to open servers file");
    return 1;
  }

  char line[255];
  unsigned int servers_num = 0;
  while (fgets(line, sizeof(line), file)) {
    servers_num++;
  }
  fseek(file, 0, SEEK_SET);

  struct Server* to = malloc(sizeof(struct Server) * servers_num);
  for (int i = 0; i < servers_num; i++) {
    fgets(line, sizeof(line), file);
    sscanf(line, "%255[^:]:%d", to[i].ip, &to[i].port);
  }
  fclose(file);

  pthread_t threads[servers_num];
  uint64_t results[servers_num];
  struct TaskArgs task_args[servers_num];

  for (int i = 0; i < servers_num; i++) {
    task_args[i].server = to[i];
    task_args[i].k = k;
    task_args[i].mod = mod;
    if (pthread_create(&threads[i], NULL, SendTaskToServer, (void*)&task_args[i])) {
      printf("Error: pthread_create failed!\n");
      return 1;
    }
  }

  for (int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], (void**)&results[i]);
  }

  uint64_t total = 1;
  for (int i = 0; i < servers_num; i++) {
    total = MultModulo(total, results[i], mod);
  }

  printf("Total: %lu\n", total);

  free(to);
  return 0;
}