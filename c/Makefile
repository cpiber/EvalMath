CC := gcc
CFLAGS := -g -Wall -Wpedantic

all: main lexer_test rpn_test
.PHONY: test

main: src/main.c src/lexer.c src/lexer.h src/rpn.c src/rpn.h src/sv.h src/stb_ds.h
	$(CC) $(CFLAGS) $(filter %.c, $^) -o $@ -lm

lexer_test: src/lexer_test.c src/lexer.c src/lexer.h src/sv.h
	$(CC) $(CFLAGS) $(filter %.c, $^) -o $@

rpn_test: src/rpn_test.c src/lexer.c src/lexer.h src/rpn.c src/rpn.h src/sv.h src/stb_ds.h
	$(CC) $(CFLAGS) $(filter %.c, $^) -o $@ -lm

test_eval: test/eval.c src/lexer.c src/lexer.h src/rpn.c src/rpn.h src/sv.h src/stb_ds.h
	$(CC) $(CFLAGS) $(filter %.c, $^) -o $@ -lm

test: test_eval
	valgrind ./test_eval
