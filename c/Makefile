CC := gcc
CFLAGS := -g -Wall -Wpedantic

all: lexer_test rpn_test

lexer_test: src/lexer_test.c src/lexer.c src/lexer.h src/sv.h
	$(CC) $(CFLAGS) $(filter %.c, $^) -o $@

rpn_test: src/rpn_test.c src/lexer.c src/lexer.h src/rpn.c src/rpn.h src/sv.h src/stb_ds.h
	$(CC) $(CFLAGS) $(filter %.c, $^) -o $@
