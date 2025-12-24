CC = gcc
CFLAGS = -Wall -Wextra -O2 -pthread -lm -Wno-unused-result
TARGET = timsort
SRC_DIR = src
SOURCES = $(SRC_DIR)/main.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

debug:
	$(CC) $(CFLAGS) -g -o $(TARGET)_debug $(SOURCES)

clean:
	rm -f $(TARGET) $(TARGET)_debug

test: $(TARGET)
	@echo "=== ТЕСТИРОВАНИЕ НА МАЛЕНЬКОМ МАССИВЕ ==="
	./$(TARGET) -n 1000 -t 1 -c
	./$(TARGET) -n 1000 -t 2 -c
	./$(TARGET) -n 1000 -t 4 -c

demo: $(TARGET)
	@echo "=== ДЕМОНСТРАЦИЯ НА СРЕДНЕМ МАССИВЕ ==="
	./$(TARGET) -n 100000 -t 4 -c -p 5

info: $(TARGET)
	./$(TARGET) -i

benchmark: $(TARGET)
	@echo "=== БЕНЧМАРК 100К ЭЛЕМЕНТОВ ==="
	@for t in 1 2 4 8; do \
		echo -n "Потоков: $$t -> "; \
		./$(TARGET) -n 100000 -t $$t 2>/dev/null | grep "Время выполнения"; \
	done

fullbench: $(TARGET)
	@echo "=== ПОЛНЫЙ БЕНЧМАРК ==="
	@echo "Размер\Потоки |     1     |     2     |     4     |     8     "
	@echo "--------------+-----------+-----------+-----------+-----------"
	@for size in 1000 10000 50000 100000 500000; do \
		printf "%12d |" $$size; \
		for threads in 1 2 4 8; do \
			if [ $$size -lt 100000 ] && [ $$threads -gt 4 ]; then \
				printf "   -      |"; \
			else \
				time=$$(./$(TARGET) -n $$size -t $$threads 2>/dev/null | grep "Время выполнения" | awk '{print $$3}'); \
				printf " %8.3f |" $$time; \
			fi; \
		done; \
		echo ""; \
	done

.PHONY: all clean test demo info benchmark fullbench
