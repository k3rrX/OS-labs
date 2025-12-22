#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <unistd.h>  // Для getopt
#include <getopt.h>  // Для расширенного парсинга

#define MIN_RUN 32

// Структура для передачи данных в поток
typedef struct {
    int* array;
    int left;
    int right;
    sem_t* sem;
} ThreadData;

// Функция сортировки вставками
void insertion_sort(int arr[], int left, int right) {
    for (int i = left + 1; i <= right; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= left && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

// Функция, выполняемая в потоке
void* sort_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    
    // Сортируем свою часть
    insertion_sort(data->array, data->left, data->right);
    
    // Освобождаем семафор
    sem_post(data->sem);
    
    pthread_exit(NULL);
}

// Функция вывода справки
void print_help() {
    printf("Использование: ./parallel_timsort [OPTIONS]\n");
    printf("Параллельная сортировка TimSort\n\n");
    printf("Опции:\n");
    printf("  -n, --size SIZE     Размер массива (по умолчанию: 1000000)\n");
    printf("  -t, --threads NUM   Количество потоков (по умолчанию: 4)\n");
    printf("  -s, --seed SEED     Сид для генератора случайных чисел\n");
    printf("  -c, --check         Проверить корректность сортировки\n");
    printf("  -h, --help          Показать эту справку\n");
    printf("\nПримеры:\n");
    printf("  ./parallel_timsort -n 1000000 -t 4\n");
    printf("  ./parallel_timsort --size 500000 --threads 2 --check\n");
}

int main(int argc, char* argv[]) {
    // Параметры по умолчанию
    int array_size = 1000000;
    int num_threads = 4;
    int check_result = 0;
    unsigned int seed = time(NULL);
    
    // Парсинг аргументов командной строки
    int opt;
    static struct option long_options[] = {
        {"size", required_argument, 0, 'n'},
        {"threads", required_argument, 0, 't'},
        {"seed", required_argument, 0, 's'},
        {"check", no_argument, 0, 'c'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    while ((opt = getopt_long(argc, argv, "n:t:s:ch", long_options, NULL)) != -1) {
        switch (opt) {
            case 'n':
                array_size = atoi(optarg);
                if (array_size <= 0) {
                    fprintf(stderr, "Ошибка: размер массива должен быть положительным\n");
                    return 1;
                }
                break;
            case 't':
                num_threads = atoi(optarg);
                if (num_threads <= 0) {
                    fprintf(stderr, "Ошибка: количество потоков должно быть положительным\n");
                    return 1;
                }
                break;
            case 's':
                seed = atoi(optarg);
                break;
            case 'c':
                check_result = 1;
                break;
            case 'h':
                print_help();
                return 0;
            default:
                fprintf(stderr, "Используйте -h или --help для справки\n");
                return 1;
        }
    }
    
    // Вывод информации о параметрах
    printf("=== Параллельная сортировка TimSort ===\n");
    printf("Размер массива: %d\n", array_size);
    printf("Количество потоков: %d\n", num_threads);
    printf("Сид генератора: %u\n", seed);
    printf("Минимальный размер 'рана': %d\n", MIN_RUN);
    
    // Инициализация генератора случайных чисел
    srand(seed);
    
    // Выделение памяти для массива
    int* array = malloc(array_size * sizeof(int));
    if (!array) {
        perror("Ошибка выделения памяти");
        return 1;
    }
    
    // Заполнение массива случайными числами
    printf("Заполнение массива...\n");
    for (int i = 0; i < array_size; i++) {
        array[i] = rand() % 1000000;
    }
    
    // Создание семафора для ограничения потоков
    sem_t thread_sem;
    sem_init(&thread_sem, 0, num_threads);
    
    // Подготовка данных для потоков
    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    ThreadData* thread_data = malloc(num_threads * sizeof(ThreadData));
    
    if (!threads || !thread_data) {
        perror("Ошибка выделения памяти для потоков");
        free(array);
        return 1;
    }
    
    // Замер времени начала
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
    // Разделение массива на части для потоков
    int chunk_size = array_size / num_threads;
    printf("Запуск %d потоков...\n", num_threads);
    
    // Создание и запуск потоков
    for (int i = 0; i < num_threads; i++) {
        sem_wait(&thread_sem);  // Ожидание свободного "слота" для потока
        
        thread_data[i].array = array;
        thread_data[i].left = i * chunk_size;
        thread_data[i].right = (i == num_threads - 1) ? array_size - 1 : (i + 1) * chunk_size - 1;
        thread_data[i].sem = &thread_sem;
        
        if (pthread_create(&threads[i], NULL, sort_thread, &thread_data[i]) != 0) {
            perror("Ошибка создания потока");
            free(array);
            free(threads);
            free(thread_data);
            return 1;
        }
    }
    
    // Ожидание завершения всех потоков
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Замер времени окончания
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Вычисление времени выполнения
    double execution_time = (end_time.tv_sec - start_time.tv_sec) +
                           (end_time.tv_nsec - start_time.tv_nsec) / 1e9;
    
    printf("Сортировка завершена за %.3f секунд\n", execution_time);
    
    // Проверка корректности сортировки (если запрошено)
    if (check_result) {
        printf("Проверка корректности сортировки...\n");
        int sorted = 1;
        for (int i = 1; i < array_size; i++) {
            if (array[i] < array[i-1]) {
                printf("Ошибка: элемент %d[%d] > %d[%d]\n", 
                       array[i-1], i-1, array[i], i);
                sorted = 0;
                break;
            }
        }
        if (sorted) {
            printf("✓ Массив отсортирован корректно\n");
        } else {
            printf("✗ Массив отсортирован НЕ корректно\n");
        }
    }
    
    // Вывод первых 10 элементов для проверки
    printf("Первые 10 элементов: ");
    for (int i = 0; i < 10 && i < array_size; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");
    
    // Освобождение ресурсов
    free(array);
    free(threads);
    free(thread_data);
    sem_destroy(&thread_sem);
    
    return 0;
}
