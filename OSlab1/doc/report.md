# Лабораторная работа №1
## Управление процессами и межпроцессорное взаимодействие

**Вариант:** 6  
**Студент:** Янкавцев Кирилл  
**Группа:** 209  
**Дата:** 20.12.2025

### Цель работы
Приобретение практических навыков в управлении процессами и межпроцессным взаимодействием через каналы (pipes).

### Задание
Родительский процесс создает дочерний процесс. Пользователь вводит имя файла. Дочерний процесс открывает файл, перенаправляет stdin на него, вычисляет сумму чисел и выводит результат через канал родителю.

### Код программы

**parent.c:**
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

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
    fgets(filename, BUFFER_SIZE, stdin);
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
        dup2(pipe_fd[WRITE_END], STDOUT_FILENO);
        close(pipe_fd[WRITE_END]);
        
        char *child_args[] = {"./child", filename, NULL};
        execv("./child", child_args);
        perror("Ошибка execv");
        exit(EXIT_FAILURE);
        
    } else {
        close(pipe_fd[WRITE_END]);
        
        printf("Родительский процесс: Дочерний процесс создан (PID: %d)\n", pid);
        printf("Родительский процесс: Читаю результат из pipe...\n");
        
        bytes_read = read(pipe_fd[READ_END], buffer, BUFFER_SIZE - 1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Родительский процесс: Получен результат: %s", buffer);
        }
        
        close(pipe_fd[READ_END]);
        wait(NULL);
        printf("Родительский процесс: Дочерний процесс завершился.\n");
    }
    
    return EXIT_SUCCESS;
}

child.c:
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <имя_файла>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        exit(EXIT_FAILURE);
    }
    
    dup2(fd, STDIN_FILENO);
    close(fd);
    
    int sum = 0, num;
    while (scanf("%d", &num) == 1) {
        sum += num;
    }
    
    printf("%d\n", sum);
    return EXIT_SUCCESS;
}

Пример работы
Родительский процесс: Введите имя файла с числами: ../test_file.txt
Родительский процесс: Дочерний процесс создан (PID: 534)
Родительский процесс: Читаю результат из pipe...
Родительский процесс: Получен результат: 550
Родительский процесс: Дочерний процесс завершился.

Анализ strace
Ключевые системные вызовы:

execve("./child", ["./child", "../test_file.txt"], ...) - запуск дочернего процесса

dup2(4, 1) - перенаправление stdout дочернего процесса в канал

dup2(3, 0) = 0 - перенаправление stdin дочернего процесса на файл

openat(AT_FDCWD, "../test_file.txt", O_RDONLY) = 3 - открытие файла

read(0, "10 20 30...", 512) = 32 - чтение чисел из файла

write(1, "550\n", 4) - запись результата в канал

<... read resumed>"550\n", 1023) = 4 - чтение результата из канала

Выводы

Освоены системные вызовы для работы с процессами: fork/exec, pipe, dup2. Реализовано межпроцессное взаимодействие через канал согласно варианту задания.
