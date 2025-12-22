CC = gcc
CFLAGS = -Wall -Wextra -std=c11
TARGETS = parent child

all: $(TARGETS)

parent: parent.c
	$(CC) $(CFLAGS) -o parent parent.c

child: child.c
	$(CC) $(CFLAGS) -o child child.c

clean:
	rm -f $(TARGETS)

.PHONY: all clean