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
} MathParserError;

MathParser math_parser_init(Lexer lexer);
MathParserError math_parser_rpn(MathParser *parser);
