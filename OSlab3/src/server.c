#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <semaphore.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define SHM_SIZE 4096

const char SHM_NAME[] = "/sum_sh_memory";
const char SEM_NAME[] = "/sum_semaphore";

int main(int argc, char** argv) {
    if (argc < 2) {
        const char message[] = "ERR: not enough arguments\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        exit(EXIT_FAILURE);
    }

    const char* filename = argv[1];

    int shared_mem = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shared_mem == -1) {
        const char message[] = "ERR: cant open shared memory\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        exit(EXIT_FAILURE);
    }

    char* const shared_mem_buffer = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shared_mem, 0);
    if (shared_mem_buffer == MAP_FAILED) {
        const char message[] = "ERR: cant map shared memory\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        exit(EXIT_FAILURE);
    }

    sem_t* semaphore = sem_open(SEM_NAME, O_RDWR);
    if (semaphore == SEM_FAILED) {
        const char message[] = "ERR: cant open semaphore\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        exit(EXIT_FAILURE);
    }

    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        const char message[] = "ERR: cant open file\n";
        write(STDERR_FILENO, message, sizeof(message) - 1);
        exit(EXIT_FAILURE);
    }

    char line[256];
    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\r\n")] = 0;
        
        if (strlen(line) == 0) {
            continue;
        }

        int numbers[100];
        int count = 0;
        char* token = strtok(line, " ");
        int valid = 1;

        while (token != NULL && count < 100) {
            char* p = token;
            
            if (*p == '-') p++;
            
            if (*p == '\0') {
                valid = 0;
                break;
            }
            
            int has_digits = 0;
            while (*p) {
                if (!isdigit(*p)) {
                    valid = 0;
                    break;
                }
                has_digits = 1;
                p++;
            }
            
            if (!valid || !has_digits) break;
            
            numbers[count] = atoi(token);
            count++;
            token = strtok(NULL, " ");
        }

        if (!valid || count == 0) {
            const char error_msg[] = "ERR: invalid input\n";
            sem_wait(semaphore);
            int* length = (int*)shared_mem_buffer;
            char* data = shared_mem_buffer + sizeof(int);
            *length = sizeof(error_msg) - 1;
            memcpy(data, error_msg, sizeof(error_msg) - 1);
            sem_post(semaphore);
        }
        else {
            int sum = 0;
            for (int i = 0; i < count; i++) {
                sum += numbers[i];
            }

            char result_str[64];
            int len = snprintf(result_str, sizeof(result_str), "%d\n", sum);
            
            sem_wait(semaphore);
            int* length = (int*)shared_mem_buffer;
            char* data = shared_mem_buffer + sizeof(int);
            *length = len;
            memcpy(data, result_str, len);
            sem_post(semaphore);
        }
        
        usleep(1000);
    }

    fclose(file);

    sem_wait(semaphore);
    int* length = (int*)shared_mem_buffer;
    *length = INT_MAX;
    sem_post(semaphore);

    sem_close(semaphore);
    munmap(shared_mem_buffer, SHM_SIZE);
    close(shared_mem);

    return 0;
}