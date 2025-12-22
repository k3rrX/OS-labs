#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <имя_файла>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    char *filename = argv[1];
    int fd;
    
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("Ошибка открытия файла");
        exit(EXIT_FAILURE);
    }
    
    if (dup2(fd, STDIN_FILENO) == -1) {
        perror("Ошибка dup2 для stdin");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);
    
    int sum = 0;
    int num;
    
    while (scanf("%d", &num) == 1) {
        sum += num;
    }
    
    printf("%d\n", sum);
    return EXIT_SUCCESS;
}