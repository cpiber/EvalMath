#pragma once

#include "lexer.h"
#include "const.h"

typedef struct {
  Token token;
  int precedence;
  bool right_associative;
  bool function;
  bool assignment;
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
  size_t nargs;
  String_View *argument_names;
  String_View lexer_content;
  MathOperator *rpn;
} MathUserFunction;

typedef struct {
  String_View name;
  double value;
} MathVariable;

typedef struct {
  Lexer lexer;
  MathOperator *output_queue;
  MathOperator *operator_stack;
  MathVariable *variables;
  MathUserFunction *functions;
  size_t paren_depth;
} MathParser;

typedef enum {
  MERR_OK,
  MERR_LEXER_ERROR,
  MERR_UNBALANCED_PARENTHESIS,
  MERR_UNEXPECTED_OPERATOR,
  MERR_OPERATOR_ERROR,
  MERR_INPUT_EMPTY,
  MERR_UNRECOGNIZED_SYMBOL,
  MERR_SYMBOL_ALREADY_SET,
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
// Converts infix string contained in lexer to RPN notation.
// Result will be in `output_queue` member, can be evaluated with `math_parser_eval`.
MathParserError math_parser_rpn(MathParser *parser);
// Evaluates a previously-parsed result by iterating `output_queue`. Clears the queue.
MathParserError math_parser_eval(MathParser *parser, double *result);
void math_parser_free(MathParser *parser);
// Use to clear all internal data structures. Only necessary after an error.
// Does *NOT* clear variables.
// Must set new lexer after use, or MERR_INPUT_EMPTY will be returned.
void math_parser_clear(MathParser *parser);
// Takes lexer `input` as new lexer and evaluates all expressions contained (multiple possible).
// Returns the result of the last expression.
// Essentially calls `math_parser_rpn` and `math_parser_eval` until all input is consumed.
MathParserError math_parser_evaluate_input(MathParser *parser, Lexer input, double *result);
bool math_parser_set_var(MathParser *parser, String_View name, double value);
bool math_parser_get_var(MathParser *parser, String_View name, double *value);
