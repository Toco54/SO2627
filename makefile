CC = gcc
CFLAGS = -Wall -Wextra

all: central

central: src/central.c src/central.h
	$(CC) $(CFLAGS) src/central.c -o central

clean:
	rm -f central