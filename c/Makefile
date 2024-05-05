CC := gcc
CFLAGS := -g -Wall -Wpedantic

all: lexer_test

lexer_test: src/lexer_test.c src/lexer.c src/lexer.h src/sv.h
	$(CC) $(CFLAGS) $(filter %.c, $^) -o $@
