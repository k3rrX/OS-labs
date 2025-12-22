#!/bin/bash

echo "=== ЛАБОРАТОРНАЯ РАБОТА №2: ИССЛЕДОВАНИЕ ПАРАЛЛЕЛЬНОГО TIMSORT ==="
echo "Дата: $(date)"
echo "Система: $(uname -a)"
echo "Количество ядер CPU: $(nproc)"
echo ""

echo "=== ТАБЛИЦА 1: Время выполнения (секунды) ==="
printf "%-10s | %8s | %8s | %8s | %8s\n" "Размер" "1 поток" "2 потока" "4 потока" "8 потоков"
echo "-----------+----------+----------+----------+----------"

for size in 1000 5000 10000 50000 100000 500000 1000000; do
    printf "%-10d |" $size
    for threads in 1 2 4 8; do
        if [ $size -lt 50000 ] && [ $threads -gt 4 ]; then
            printf "   -      |"
        else
            output=$(./timsort -n $size -t $threads 2>&1 | tail -20)
            time=$(echo "$output" | grep "Время выполнения" | awk '{print $3}')
            printf " %8.3f |" $time
        fi
    done
    echo ""
done

echo ""
echo "=== ТАБЛИЦА 2: Ускорение (Speedup) относительно 1 потока ==="
printf "%-10s | %8s | %8s | %8s | %8s\n" "Размер" "1 поток" "2 потока" "4 потока" "8 потоков"
echo "-----------+----------+----------+----------+----------"

# Получим базовые значения для 1 потока
declare -A base_times
for size in 1000 5000 10000 50000 100000 500000 1000000; do
    output=$(./timsort -n $size -t 1 2>&1 | tail -20)
    base_times[$size]=$(echo "$output" | grep "Время выполнения" | awk '{print $3}')
done

for size in 1000 5000 10000 50000 100000 500000 1000000; do
    printf "%-10d |" $size
    for threads in 1 2 4 8; do
        if [ $threads -eq 1 ]; then
            printf "   1.00x |"
        elif [ $size -lt 50000 ] && [ $threads -gt 4 ]; then
            printf "   -     |"
        else
            output=$(./timsort -n $size -t $threads 2>&1 | tail -20)
            time=$(echo "$output" | grep "Время выполнения" | awk '{print $3}')
            base=${base_times[$size]}
            if [ $(echo "$base > 0" | bc -l) -eq 1 ]; then
                speedup=$(echo "scale=2; $base / $time" | bc)
                printf " %7.2fx |" $speedup
            else
                printf "   -     |"
            fi
        fi
    done
    echo ""
done

echo ""
echo "=== ТАБЛИЦА 3: Эффективность потоков (%) ==="
printf "%-10s | %8s | %8s | %8s | %8s\n" "Размер" "1 поток" "2 потока" "4 потока" "8 потоков"
echo "-----------+----------+----------+----------+----------"

for size in 1000 5000 10000 50000 100000 500000 1000000; do
    printf "%-10d |" $size
    for threads in 1 2 4 8; do
        if [ $threads -eq 1 ]; then
            printf "  100%% |"
        elif [ $size -lt 50000 ] && [ $threads -gt 4 ]; then
            printf "   -    |"
        else
            output=$(./timsort -n $size -t $threads 2>&1 | tail -20)
            time=$(echo "$output" | grep "Время выполнения" | awk '{print $3}')
            base=${base_times[$size]}
            if [ $(echo "$base > 0" | bc -l) -eq 1 ]; then
                speedup=$(echo "scale=2; $base / $time" | bc)
                efficiency=$(echo "scale=0; $speedup / $threads * 100" | bc)
                printf " %6.0f%% |" $efficiency
            else
                printf "   -    |"
            fi
        fi
    done
    echo ""
done

echo ""
echo "=== КОМАНДЫ ДЛЯ ДЕМОНСТРАЦИИ ПОТОКОВ ==="
echo "1. Показать информацию о системе:"
echo "   ./timsort -i"
echo ""
echo "2. Демонстрация на среднем массиве:"
echo "   ./timsort -n 100000 -t 4 -c -p 5"
echo ""
echo "3. Сравнение режимов на большом массиве:"
echo "   ./timsort -n 1000000 -t 1"
echo "   ./timsort -n 1000000 -t 4"
echo "   ./timsort -n 1000000 -t 8"
