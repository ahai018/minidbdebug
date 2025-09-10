CC=gcc
CFLAGS=-Iinclude -Wall -Wextra -O2
SRC=src/main.c src/executor.c src/catalog.c src/record.c src/page.c src/expr.c
OBJ=$(SRC:.c=.o)
all: minidb
minidb: $(OBJ)
	$(CC) $(OBJ) -o $@
clean:
	rm -f $(OBJ) minidb
