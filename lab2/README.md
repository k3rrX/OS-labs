# Лабораторная работа №2: Параллельный TimSort

## Описание
Реализация параллельной сортировки массива алгоритмом TimSort на языке C с использованием POSIX Threads (pthread).

## Требования
- Операционная система: Linux (или WSL для Windows)
- Компилятор: GCC
- Библиотеки: pthread, math

## Сборка
```bash
make

cd ~/OS-labs/lab2/src
echo "1. Маленький массив (однопоточный):"
./parallel_timsort -n 50000 -t 4

echo -e "\n2. Большой массив (параллельный):"
./parallel_timsort -n 100000 -t 4

echo -e "\n3. Максимальное ускорение:"
for t in 1 2 4 8; do
    echo -n "  $t поток(ов): "
    ./parallel_timsort -n 500000 -t $t 2>&1 | grep "Время выполнения:"
done
