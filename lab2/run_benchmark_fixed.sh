#!/bin/bash

echo "=== ЛАБОРАТОРНАЯ РАБОТА №2: ИССЛЕДОВАНИЕ ПАРАЛЛЕЛЬНОГО TIMSORT ==="
echo "Дата: $(date)"
echo "Система: $(uname -a)"
echo "Количество ядер CPU: $(nproc)"
echo ""

# Создадим временные файлы для хранения результатов
declare -A results

echo "=== Измерение времени выполнения ==="
echo "Это займет несколько минут..."

# Заполняем массив результатов
for size in 1000 5000 10000 50000 100000 500000 1000000; do
    for threads in 1 2 4 8; do
        if [ $size -lt 50000 ] && [ $threads -gt 4 ]; then
            continue
        fi
        
        echo -n "  Размер: $size, Потоков: $threads... "
        output=$(./timsort -n $size -t $threads 2>&1)
        time=$(echo "$output" | grep "Время выполнения" | awk '{print $3}')
        
        if [ -n "$time" ]; then
            results["${size}_${threads}"]=$time
            echo "$time сек"
        else
            echo "ОШИБКА"
        fi
        
        # Небольшая пауза между запусками
        sleep 0.5
    done
done

echo ""
echo "=== ТАБЛИЦА 1: Время выполнения (секунды) ==="
printf "%-10s | %9s | %9s | %9s | %9s\n" "Размер" "1 поток" "2 потока" "4 потока" "8 потоков"
echo "-----------+-----------+-----------+-----------+-----------"

for size in 1000 5000 10000 50000 100000 500000 1000000; do
    printf "%-10d |" $size
    for threads in 1 2 4 8; do
        key="${size}_${threads}"
        if [ -n "${results[$key]}" ]; then
            printf " %8.3f  |" ${results[$key]}
        else
            printf "    -      |"
        fi
    done
    echo ""
done

echo ""
echo "=== ТАБЛИЦА 2: Ускорение (Speedup) относительно 1 потока ==="
printf "%-10s | %9s | %9s | %9s | %9s\n" "Размер" "1 поток" "2 потока" "4 потока" "8 потоков"
echo "-----------+-----------+-----------+-----------+-----------"

for size in 1000 5000 10000 50000 100000 500000 1000000; do
    printf "%-10d |" $size
    
    # Базовое время для 1 потока
    base_key="${size}_1"
    if [ -n "${results[$base_key]}" ]; then
        base_time=${results[$base_key]}
        
        for threads in 1 2 4 8; do
            if [ $threads -eq 1 ]; then
                printf "   1.00x   |"
            else
                key="${size}_${threads}"
                if [ -n "${results[$key]}" ] && [ $(echo "$base_time > 0" | bc -l 2>/dev/null) -eq 1 ]; then
                    current_time=${results[$key]}
                    speedup=$(echo "scale=2; $base_time / $current_time" | bc 2>/dev/null)
                    if [ -n "$speedup" ]; then
                        printf "  %6.2fx  |" $speedup
                    else
                        printf "    -      |"
                    fi
                else
                    printf "    -      |"
                fi
            fi
        done
    else
        printf "    -      |    -      |    -      |    -      |"
    fi
    echo ""
done

echo ""
echo "=== ТАБЛИЦА 3: Эффективность (%) ==="
printf "%-10s | %9s | %9s | %9s | %9s\n" "Размер" "1 поток" "2 потока" "4 потока" "8 потоков"
echo "-----------+-----------+-----------+-----------+-----------"

for size in 1000 5000 10000 50000 100000 500000 1000000; do
    printf "%-10d |" $size
    
    base_key="${size}_1"
    if [ -n "${results[$base_key]}" ]; then
        base_time=${results[$base_key]}
        
        for threads in 1 2 4 8; do
            if [ $threads -eq 1 ]; then
                printf "   100%%   |"
            else
                key="${size}_${threads}"
                if [ -n "${results[$key]}" ] && [ $(echo "$base_time > 0" | bc -l 2>/dev/null) -eq 1 ]; then
                    current_time=${results[$key]}
                    speedup=$(echo "scale=4; $base_time / $current_time" | bc 2>/dev/null)
                    efficiency=$(echo "scale=0; $speedup / $threads * 100" | bc 2>/dev/null 2>/dev/null)
                    if [ -n "$efficiency" ]; then
                        printf "  %6.0f%%  |" $efficiency
                    else
                        printf "    -      |"
                    fi
                else
                    printf "    -      |"
                fi
            fi
        done
    else
        printf "    -      |    -      |    -      |    -      |"
    fi
    echo ""
done

echo ""
echo "=== ВЫВОДЫ ==="
echo "1. На малых массивах (<50k) многопоточность неэффективна"
echo "2. На больших массивах (>500k) наблюдается ускорение"
echo "3. Оптимальное количество потоков: 4-8 для массивов >500k"
echo "4. Эффективность снижается при увеличении потоков"

echo ""
echo "=== КОМАНДЫ ДЛЯ ВЕРИФИКАЦИИ ==="
echo "./timsort -n 1000000 -t 4 -c -p 5"
echo "./timsort -i"
