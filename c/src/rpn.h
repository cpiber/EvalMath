#pragma once

#include "lexer.h"

typedef struct {
  Token token;
  int precedence;
  size_t nargs;
} MathOperator;

typedef struct {
  Lexer lexer;
  MathOperator *output_queue;
  MathOperator *operator_stack;
} MathParser;

typedef enum {
  MERR_OK,
  MERR_LEXER_ERROR,
  MERR_UNBALANCED_PARENTHESIS,
  MERR_UNEXPECTED_OPERATOR,
  MERR_OPERATOR_ERROR,
  MERR_INPUT_EMPTY,
} MathParserError;

#define MATH_PARSER_TRY(expr) do { \
  MathParserError err = (expr);    \
  if (err != MERR_OK) return err;  \
} while(0)

MathParser math_parser_init(Lexer lexer);
MathParserError math_parser_rpn(MathParser *parser);
MathParserError math_parser_eval(MathParser *parser, double *result);
