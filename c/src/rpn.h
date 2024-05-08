#pragma once

#include "lexer.h"
#include "const.h"

typedef struct {
  Token token;
  int precedence;
  bool right_associative;
  bool function;
  size_t nargs;
} MathOperator;

typedef struct {
  String_View name;
  size_t nargs;
  union {
    double (*unary)(double);
    double (*binary)(double, double);
  } as;
} MathBuiltinFunction;

typedef struct {
  String_View name;
  double value;
} MathBuiltinConstant;

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
  MERR_UNRECOGNIZED_SYMBOL,
} MathParserError;

#define RETURN(v) do { \
  err = (v);           \
  goto return_defer;   \
} while (0)
#define MATH_PARSER_TRY(expr) do {   \
  MathParserError _err = (expr);     \
  if (_err != MERR_OK) RETURN(_err); \
} while(0)

MathParser math_parser_init(Lexer lexer);
MathParserError math_parser_rpn(MathParser *parser);
MathParserError math_parser_eval(MathParser *parser, double *result);
void math_parser_free(MathParser *parser);
