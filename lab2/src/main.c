#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/sysinfo.h>
#include <string.h>

// ==================== КОНСТАНТЫ ====================
#define MIN_MERGE 32
#define MAX_THREADS 64
#define MIN_PARALLEL_SIZE 100000  // Минимальный размер для параллельной сортировки

// ==================== СТРУКТУРЫ ====================
typedef struct {
    int* array;
    int start;
    int end;
    int thread_id;
} ThreadData;

// ==================== УТИЛИТЫ ====================
int min(int a, int b) { return a < b ? a : b; }

// ==================== СОРТИРОВКА ВСТАВКАМИ ====================
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

// ==================== СЛИЯНИЕ ====================
void merge(int arr[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;
    
    int* L = malloc(n1 * sizeof(int));
    int* R = malloc(n2 * sizeof(int));
    
    for (int i = 0; i < n1; i++) L[i] = arr[left + i];
    for (int j = 0; j < n2; j++) R[j] = arr[mid + 1 + j];
    
    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) arr[k++] = L[i++];
        else arr[k++] = R[j++];
    }
    
    while (i < n1) arr[k++] = L[i++];
    while (j < n2) arr[k++] = R[j++];
    
    free(L);
    free(R);
}

// ==================== TIMSORT (однопоточный) ====================
void timsort_single(int arr[], int n) {
    // Сортировка маленьких runs
    for (int i = 0; i < n; i += MIN_MERGE) {
        insertion_sort(arr, i, min(i + MIN_MERGE - 1, n - 1));
    }
    
    // Слияние runs
    for (int size = MIN_MERGE; size < n; size = 2 * size) {
        for (int left = 0; left < n; left += 2 * size) {
            int mid = left + size - 1;
            int right = min(left + 2 * size - 1, n - 1);
            
            if (mid < right) {
                merge(arr, left, mid, right);
            }
        }
    }
}

// ==================== ПОТОКОВАЯ ФУНКЦИЯ (Timsort части) ====================
void* timsort_thread(void* arg) {
    ThreadData* data = (ThreadData*)arg;
    
    // Применяем TimSort к своей части массива
    int n = data->end - data->start + 1;
    
    // Сортировка маленьких runs
    for (int i = data->start; i <= data->end; i += MIN_MERGE) {
        insertion_sort(data->array, i, min(i + MIN_MERGE - 1, data->end));
    }
    
    // Слияние runs внутри части
    for (int size = MIN_MERGE; size < n; size = 2 * size) {
        for (int left = data->start; left <= data->end; left += 2 * size) {
            int mid = left + size - 1;
            int right = min(left + 2 * size - 1, data->end);
            
            if (mid < right && mid <= data->end && right <= data->end) {
                merge(data->array, left, mid, right);
            }
        }
    }
    
    free(data);
    return NULL;
}

// ==================== МНОГОПОТОЧНЫЙ TIMSORT ====================
void timsort_parallel(int arr[], int n, int max_threads) {
    printf("\n=== ПАРАЛЛЕЛЬНАЯ СОРТИРОВКА TIMSORT ===\n");
    printf("Всего элементов: %d\n", n);
    printf("Используется потоков: %d\n", max_threads);
    printf("MIN_MERGE: %d\n", MIN_MERGE);
    
    pthread_t threads[MAX_THREADS];
    int thread_count = 0;
    
    // Разделение массива между потоками
    int chunk_size = n / max_threads;
    
    for (int i = 0; i < max_threads; i++) {
        ThreadData* data = malloc(sizeof(ThreadData));
        data->array = arr;
        data->start = i * chunk_size;
        data->end = (i == max_threads - 1) ? n - 1 : (i + 1) * chunk_size - 1;
        data->thread_id = i + 1;
        
        if (pthread_create(&threads[thread_count], NULL, timsort_thread, data) == 0) {
            thread_count++;
        } else {
            free(data);
        }
    }
    
    // Ожидание завершения потоков
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Слияние отсортированных частей
    printf("Слияние отсортированных частей...\n");
    for (int size = chunk_size; size < n; size = 2 * size) {
        for (int left = 0; left < n; left += 2 * size) {
            int mid = left + size - 1;
            int right = min(left + 2 * size - 1, n - 1);
            
            if (mid < right) {
                merge(arr, left, mid, right);
            }
        }
    }
}

// ==================== УМНЫЙ ВЫБОР РЕЖИМА ====================
void smart_timsort(int arr[], int n, int max_threads) {
    // Для маленьких массивов используем однопоточный режим
    if (n < MIN_PARALLEL_SIZE || max_threads == 1) {
        printf("Используется однопоточный режим (массив слишком мал)\n");
        timsort_single(arr, n);
        return;
    }
    
    // Ограничиваем количество потоков разумным значением
    int optimal_threads = max_threads;
    int cpu_cores = get_nprocs();
    
    if (optimal_threads > cpu_cores * 2) {
        optimal_threads = cpu_cores * 2;
        printf("Ограничение потоков: %d -> %d (оптимально для %d ядер)\n", 
               max_threads, optimal_threads, cpu_cores);
    }
    
    timsort_parallel(arr, n, optimal_threads);
}

// ==================== ПРОВЕРКА СОРТИРОВКИ ====================
int is_sorted(int arr[], int n) {
    for (int i = 1; i < n; i++) {
        if (arr[i] < arr[i-1]) {
            return 0;
        }
    }
    return 1;
}

// ==================== ПОКАЗАТЬ ИНФОРМАЦИЮ О ПОТОКАХ ====================
void show_threads_info() {
    printf("\n=== ИНФОРМАЦИЯ О СИСТЕМЕ ===\n");
    printf("PID процесса: %d\n", getpid());
    printf("Количество ядер CPU: %d\n", get_nprocs());
    
    printf("\nКоманды для мониторинга потоков:\n");
    printf("1. ps -T -p %d\n", getpid());
    printf("2. top -H -p %d\n", getpid());
    
    printf("\nТекущие потоки системы:\n");
    char command[100];
    snprintf(command, sizeof(command), "ps -L -p %d 2>/dev/null | head -10", getpid());
    system(command);
}

// ==================== ЗАПОЛНЕНИЕ МАССИВА ====================
void fill_array(int arr[], int n, unsigned int seed) {
    srand(seed);
    for (int i = 0; i < n; i++) {
        arr[i] = rand() % (n * 10);
    }
}

// ==================== ВЫВОД СПРАВКИ ====================
void print_usage(const char* program_name) {
    printf("Использование: %s [ОПЦИИ]\n", program_name);
    printf("Параллельная сортировка TimSort с интеллектуальным выбором режима\n\n");
    
    printf("Обязательные опции:\n");
    printf("  -n SIZE      Размер массива (например: 1000000)\n");
    printf("  -t THREADS   Максимальное количество потоков (например: 4)\n\n");
    
    printf("Дополнительные опции:\n");
    printf("  -s SEED      Сид для генератора случайных чисел\n");
    printf("  -c           Проверить корректность сортировки\n");
    printf("  -p NUM       Вывести первые NUM элементов\n");
    printf("  -i           Показать информацию о потоках системы\n");
    printf("  -h           Показать эту справку\n\n");
    
    printf("Примеры:\n");
    printf("  %s -n 1000000 -t 4\n", program_name);
    printf("  %s -n 500000 -t 2 -c -p 10\n", program_name);
    printf("  %s -i\n", program_name);
}

// ==================== ГЛАВНАЯ ФУНКЦИЯ ====================
int main(int argc, char* argv[]) {
    // Параметры
    int array_size = 0;
    int max_threads = 0;
    unsigned int seed = time(NULL);
    int check_sort = 0;
    int print_elements = 0;
    int show_info = 0;
    
    // Парсинг аргументов
    int opt;
    while ((opt = getopt(argc, argv, "n:t:s:cp:ih")) != -1) {
        switch (opt) {
            case 'n':
                array_size = atoi(optarg);
                if (array_size <= 0) {
                    fprintf(stderr, "Ошибка: размер массива должен быть > 0\n");
                    return 1;
                }
                break;
            case 't':
                max_threads = atoi(optarg);
                if (max_threads <= 0 || max_threads > MAX_THREADS) {
                    fprintf(stderr, "Ошибка: количество потоков должно быть от 1 до %d\n", MAX_THREADS);
                    return 1;
                }
                break;
            case 's':
                seed = atoi(optarg);
                break;
            case 'c':
                check_sort = 1;
                break;
            case 'p':
                print_elements = atoi(optarg);
                break;
            case 'i':
                show_info = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                fprintf(stderr, "Неизвестная опция. Используйте -h для справки\n");
                return 1;
        }
    }
    
    // Режим показа информации о потоках
    if (show_info) {
        show_threads_info();
        return 0;
    }
    
    // Проверка обязательных параметров
    if (array_size == 0 || max_threads == 0) {
        fprintf(stderr, "\nОшибка: необходимо указать размер массива (-n) и количество потоков (-t)\n");
        fprintf(stderr, "Пример: %s -n 1000000 -t 4\n\n", argv[0]);
        print_usage(argv[0]);
        return 1;
    }
    
    printf("========================================\n");
    printf("Лабораторная работа №2: Параллельный TimSort\n");
    printf("========================================\n");
    
    // Выделение памяти
    int* array = malloc(array_size * sizeof(int));
    if (!array) {
        perror("Ошибка выделения памяти");
        return 1;
    }
    
    // Заполнение массива
    printf("Заполнение массива (%d элементов)...\n", array_size);
    fill_array(array, array_size, seed);
    
    // Замер времени
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Умная сортировка
    smart_timsort(array, array_size, max_threads);
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken = (end.tv_sec - start.tv_sec) + 
                       (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("\n=== РЕЗУЛЬТАТЫ ===\n");
    printf("Время выполнения: %.3f секунд\n", time_taken);
    printf("Скорость: %.0f элементов/сек\n", array_size / time_taken);
    
    // Проверка сортировки
    if (check_sort) {
        printf("Проверка корректности сортировки... ");
        if (is_sorted(array, array_size)) {
            printf("✓ УСПЕШНО\n");
        } else {
            printf("✗ ОШИБКА\n");
        }
    }
    
    // Вывод элементов
    if (print_elements > 0) {
        printf("Первые %d элементов: ", print_elements);
        int limit = min(print_elements, array_size);
        for (int i = 0; i < limit; i++) {
            printf("%d ", array[i]);
        }
        printf("\n");
    }
    
    // Освобождение памяти
    free(array);
    
    printf("\nРабота завершена успешно!\n");
    return 0;
}
