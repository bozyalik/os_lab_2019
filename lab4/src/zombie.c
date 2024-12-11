#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    pid_t child_pid = fork();

    if (child_pid == 0) {
        // Дочерний процесс
        printf("Дочерний процесс: Я завершился.\n");
    } else if (child_pid > 0) {
        // Родительский процесс
        printf("Родительский процесс: Дочерний процесс с PID %d создан.\n", child_pid);
        wait(NULL); // Родительский процесс ждет завершения дочернего процесса
    } else {
        // Ошибка при создании дочернего процесса
        perror("fork");
        return 1;
    }

    return 0;
}