#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define BUFFER_SIZE 1024
#define READ_END 0
#define WRITE_END 1

int main() {
    char filename[BUFFER_SIZE];
    int pipe_fd[2];
    pid_t pid;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    printf("Родительский процесс: Введите имя файла с числами: ");
    if (fgets(filename, BUFFER_SIZE, stdin) == NULL) {
        perror("Ошибка чтения имени файла");
        exit(EXIT_FAILURE);
    }
    
    filename[strcspn(filename, "\n")] = '\0';
    
    if (pipe(pipe_fd) == -1) {
        perror("Ошибка создания pipe");
        exit(EXIT_FAILURE);
    }
    
    pid = fork();
    if (pid == -1) {
        perror("Ошибка fork");
        exit(EXIT_FAILURE);
    }
    
    if (pid == 0) {
        close(pipe_fd[READ_END]);
        
        if (dup2(pipe_fd[WRITE_END], STDOUT_FILENO) == -1) {
            perror("Ошибка dup2 в дочернем процессе");
            exit(EXIT_FAILURE);
        }
        close(pipe_fd[WRITE_END]);
        
        char child_path[1024];
        snprintf(child_path, sizeof(child_path), "%s/src/child", getcwd(NULL, 0));
        char *child_args[] = {child_path, filename, NULL};  
        
        if (execv("./child", child_args) == -1) {
            perror("Ошибка execv");
            exit(EXIT_FAILURE);
        }
        
    } else {
        close(pipe_fd[WRITE_END]);
        
        printf("Родительский процесс: Дочерний процесс создан (PID: %d)\n", pid);
        printf("Родительский процесс: Читаю результат из pipe...\n");
        
        bytes_read = read(pipe_fd[READ_END], buffer, BUFFER_SIZE - 1);
        if (bytes_read == -1) {
            perror("Ошибка чтения из pipe");
            close(pipe_fd[READ_END]);
            wait(NULL);
            exit(EXIT_FAILURE);
        }
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Родительский процесс: Получен результат: %s", buffer);
        } else {
            printf("Родительский процесс: Не получено данных\n");
        }
        
        close(pipe_fd[READ_END]);
        wait(NULL);
        printf("Родительский процесс: Дочерний процесс завершился.\n");
    }
    
    return EXIT_SUCCESS;
}