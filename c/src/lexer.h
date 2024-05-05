#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "sv.h"

typedef struct {
  const char *file; // 0-terminated
  size_t line;
  size_t col;
} Location;
#define LOC_FMT "%s:%zu:%zu"
#define LOC_ARG(loc) (loc).file, (loc).line, (loc).col

typedef enum {
  TK_INTEGER,
  TK_REAL,
  TK_OP,
  TK_OPEN_PAREN,
  TK_CLOSE_PAREN,
} TokenKind;

typedef struct {
  TokenKind kind;
  String_View content;
  Location loc;
  union {
    struct {
      int64_t value;
    } integer;
    struct {
      double value;
    } real;
    enum {
      OP_ADD,
      OP_SUB,
      OP_MUL,
      OP_DIV,
      OP_EXP,
    } op;
  } as;
} Token;

typedef struct {
  const char *file; // 0-terminated
  String_View content;
  Location loc;
} Lexer;

typedef enum {
  LERR_OK,
  LERR_EOF,
  LERR_INVALID_LITERAL,
  LERR_UNRECOGNIZED_TOKEN,
} LexerError;

#define LEXER_TRY(expr) do {      \
  LexerError err = (expr);        \
  if (err != LERR_OK) return err; \
} while(0)


Lexer lexer_init(char *file, String_View content);
LexerError lexer_peek(Lexer *lexer, Token *token);
LexerError lexer_next_token(Lexer *lexer, Token *token);
__attribute__((format(printf,3,4)))
void lexer_dump_err(Location, FILE*, char *fmt, ...);
const char *lexer_strtokenkind(TokenKind);
void lexer_dump_token(Token);
const char *lexer_strerr(LexerError);
