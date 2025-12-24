#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>

#define SHM_SIZE 4096

const char SHM_NAME[] = "/sum_sh_memory";
const char SEM_NAME[] = "/sum_semaphore";

int main() {
    int shared_mem = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (shared_mem == -1) {
        const char message[] = "ERR: cant create shared memory\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        exit(EXIT_FAILURE);
    }

    if (ftruncate(shared_mem, SHM_SIZE) != 0) {
        const char message[] = "ERR: cant resize shared memory\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        exit(EXIT_FAILURE);
    }

    char* const shared_mem_buffer = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem, 0);
    if (shared_mem_buffer == MAP_FAILED) {
        const char message[] = "ERR: cant map shared memory\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        exit(EXIT_FAILURE);
    }

    int* length = (int*)shared_mem_buffer;
    *length = 0;

    sem_t* semaphore = sem_open(SEM_NAME, O_CREAT | O_RDWR, 0666, 1);
    if (semaphore == SEM_FAILED) {
        const char message[] = "ERR: cant create semaphore\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        exit(EXIT_FAILURE);
    }

    char file_path[128];
    const char message[] = "Input file : ";
    write(STDOUT_FILENO, message, sizeof(message) - 1);
    
    int result = read(STDIN_FILENO, file_path, sizeof(file_path) - 1);
    if (result <= 0) {
        const char error_message[] = "ERR: cant read filename\n";
        write(STDERR_FILENO, error_message, sizeof(error_message) - 1);
        exit(EXIT_FAILURE);
    }
    file_path[result - 1] = 0;

    pid_t child = fork();

    if (child == 0) {
        execl("./server", "server", file_path, NULL);
        const char message[] = "ERR: cant execute server\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        exit(EXIT_FAILURE);
    }
    else if (child == -1) {
        const char message[] = "ERR: cant fork\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        exit(EXIT_FAILURE);
    }

    int running = 1;
    while(running) {
        sem_wait(semaphore);

        int* current_length = (int*)shared_mem_buffer;
        char* data = shared_mem_buffer + sizeof(int);

        if (*current_length == INT_MAX) {
            running = 0;
        }
        else if (*current_length > 0) {
            write(STDOUT_FILENO, data, *current_length);
            *current_length = 0;
        }
        
        sem_post(semaphore);
        usleep(1000);
    }

    waitpid(child, NULL, 0);

    sem_close(semaphore);
    sem_unlink(SEM_NAME);
    munmap(shared_mem_buffer, SHM_SIZE);
    shm_unlink(SHM_NAME);
    close(shared_mem);

    return 0;
}