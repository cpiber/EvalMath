#include "rpn.h"
#include "lexer.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

#define ALEN(x) (sizeof(x)/sizeof((x)[0]))

static double math_parser_log(double a, double b)
{
  return log(b) / log(a);
}

static MathBuiltinFunction MATH_PARSER_BUILTIN_FUNCTIONS[] = {
#define UNARY(fname)           \
  {                            \
    .name = SV_STATIC(#fname), \
    .nargs = 1,                \
    .as = {                    \
      .unary = fname,          \
    },                         \
  },
  UNARY(sin)
  UNARY(cos)
  UNARY(tan)
  UNARY(asin)
  UNARY(acos)
  UNARY(atan)
  UNARY(sqrt)
  UNARY(log2)
  UNARY(log10)
  UNARY(log)
#undef UNARY
  {
    .name = SV_STATIC("ln"),
    .nargs = 1,
    .as = {
      .unary = log,
    },
  },
  {
    .name = SV_STATIC("log"),
    .nargs = 2,
    .as = {
      .binary = math_parser_log,
    },
  },
};

static MathVariable MATH_PARSER_BUILTIN_CONSTANTS[] = {
#define _VAL(X) X
#define CONST(cname) \
  { \
    .name = SV_STATIC(#cname), \
    .value = _VAL(M_ ## cname), \
  },
  CONST(PI)
  CONST(PI_2)
  CONST(PI_4)
  CONST(E)
  CONST(LOG2E)
  CONST(LOG10E)
  CONST(LN2)
  CONST(LN10)
  CONST(LN2)
  CONST(LN2)
  CONST(SQRT2)
#undef CONST
#undef _VAL
};

// Private functions

static String_View sv_dup(const String_View sv)
{
  char *ndata = malloc(sv.count * sizeof(sv.data[0]));
  assert(ndata != NULL);
  memcpy(ndata, sv.data, sv.count * sizeof(sv.data[0]));
  return (String_View) {
    .data = ndata,
    .count = sv.count,
  };
}

static bool math_parser_has_function(const MathParser *const parser, String_View name, ssize_t nargs)
{
  for (size_t i = 0; i < ALEN(MATH_PARSER_BUILTIN_FUNCTIONS); ++i)
  {
    if (sv_eq_ignorecase(name, MATH_PARSER_BUILTIN_FUNCTIONS[i].name) && (nargs < 0 || nargs == MATH_PARSER_BUILTIN_FUNCTIONS[i].nargs))
    {
      return true;
    }
  }
  size_t size = arrlenu(parser->functions);
  for (size_t i = 0; i < size; ++i)
  {
    if (sv_eq_ignorecase(name, parser->functions[i].name) && (nargs < 0 || nargs == parser->functions[i].nargs))
    {
      return true;
    }
  }
  return false;
}

static bool math_parser_has_variable(const MathParser *const parser, String_View name)
{
  for (size_t i = 0; i < ALEN(MATH_PARSER_BUILTIN_CONSTANTS); ++i)
  {
    if (sv_eq_ignorecase(name, MATH_PARSER_BUILTIN_CONSTANTS[i].name))
    {
      return true;
    }
  }
  size_t size = arrlenu(parser->variables);
  for (size_t i = 0; i < size; ++i)
  {
    if (sv_eq_ignorecase(name, parser->variables[i].name))
    {
      return true;
    }
  }
  return false;
}

static MathOperator math_parser_last_op(const MathParser *const parser)
{
  size_t len = arrlenu(parser->operator_stack);
  assert(len > 0);
  return parser->operator_stack[len - 1];
}

static int math_parser_precedence(const Token operator, bool unary)
{
  assert(operator.kind == TK_OP);
  switch (operator.as.op) {
    case OP_ADD:
    case OP_SUB:
      return unary ? 2 : 4;
    case OP_MUL:
    case OP_DIV:
      return 3;
    case OP_EXP:
      return 1;
  }
  assert(0 && "unreachable");
}

static MathParserError math_parser_parse_one_token(MathParser *parser, const Token token, const Token lasttoken);
static void math_parser_check_implicit_mult(MathParser *parser, const Token token, const Token lasttoken)
{
  // two consequtive numbers -> implicit multiplication
  // could also be a variable (`x`) or compound expression (`(1+2)`)
  // symbol followed by ( is recognized as function
  if (lasttoken.kind == TK_REAL || lasttoken.kind == TK_INTEGER || lasttoken.kind == TK_CLOSE_PAREN || (lasttoken.kind == TK_SYMBOL && token.kind != TK_OPEN_PAREN))
  {
    Token newtoken = (Token) {
      .kind = TK_OP,
      .content = {
        .data = "*",
        .count = 1,
      },
      .loc = token.loc,
      .as = {
        .op = OP_MUL,
      }
    };
    MathParserError err = math_parser_parse_one_token(parser, newtoken, lasttoken);
    assert(err == MERR_OK);
  }
}

static MathParserError math_parser_parse_one_token(MathParser *parser, const Token token, const Token lasttoken)
{
  switch (token.kind) {
    case TK_INTEGER:
    case TK_REAL:
      math_parser_check_implicit_mult(parser, token, lasttoken);
      arrput(parser->output_queue, (MathOperator) {.token=token});
      break;
    case TK_SYMBOL: {
      math_parser_check_implicit_mult(parser, token, lasttoken);
      Token peek;
      Lexer peek_lexer = parser->lexer;
      bool function = false;
      if (math_parser_has_function(parser, token.content, -1)) function = true;
      else if (math_parser_has_variable(parser, token.content)) function = false;
      else if (lexer_next_token(&peek_lexer, &peek) == LERR_OK && peek.kind == TK_OPEN_PAREN) function = true;

      if (function)
      {
        MathOperator fn = (MathOperator) {
          .token = token,
          .function = true,
          .nargs = lexer_next_token(&peek_lexer, &peek) == LERR_OK && peek.kind != TK_CLOSE_PAREN ? 1 : 0,
        };
        arrput(parser->operator_stack, fn);
      }
      else
      {
        // variable
        arrput(parser->output_queue, (MathOperator) {.token=token});
      }
    } break;
    case TK_OP: {
      bool unary = false;
      if ((token.as.op == OP_ADD || token.as.op == OP_SUB) && (lasttoken.kind == TK_OPEN_PAREN || lasttoken.kind == TK_OP))
      {
        unary = true;
      }
      else if (lasttoken.kind == TK_OP)
      {
        lexer_dump_err(token.loc, stderr, "Unexpected operator " SV_Fmt ", expected expression", SV_Arg(token.content));
        fprintf(stderr, LOC_FMT ": NOTE: Preceded by this operator " SV_Fmt "\n", LOC_ARG(lasttoken.loc), SV_Arg(lasttoken.content));
        return MERR_UNEXPECTED_OPERATOR;
      }
      else if (lasttoken.kind == TK_OPEN_PAREN || lasttoken.kind == TK_SEPARATOR)
      {
        lexer_dump_err(token.loc, stderr, "Unexpected operator " SV_Fmt ", expected expression", SV_Arg(token.content));
        fprintf(stderr, LOC_FMT ": NOTE: Preceded by this " SV_Fmt "\n", LOC_ARG(lasttoken.loc), SV_Arg(lasttoken.content));
        return MERR_UNEXPECTED_OPERATOR;
      }
      MathOperator op = (MathOperator) {
        .token = token,
        .precedence = math_parser_precedence(token, unary),
        .right_associative = token.as.op == OP_EXP || unary,
        .nargs = unary ? 1 : 2,
      };

      MathOperator top_op;
      while (arrlenu(parser->operator_stack) > 0)
      {
        top_op = math_parser_last_op(parser);
        if (top_op.token.kind != TK_OP) break; // not an operator -> parenthesis, stop
        if (op.precedence == top_op.precedence && op.right_associative) break; // same precedence must be left associative
        if (op.precedence < top_op.precedence) break; // must have greater precedence
        arrput(parser->output_queue, arrpop(parser->operator_stack));
      }
      arrput(parser->operator_stack, op);
    } break;
    case TK_SEPARATOR: {
      if (lasttoken.kind == TK_OP)
      {
        lexer_dump_err(token.loc, stderr, "Unexpected " SV_Fmt ", expected expression", SV_Arg(token.content));
        fprintf(stderr, LOC_FMT ": NOTE: Preceded by this operator " SV_Fmt "\n", LOC_ARG(lasttoken.loc), SV_Arg(lasttoken.content));
        return MERR_UNEXPECTED_OPERATOR;
      }
      else if (lasttoken.kind == TK_OPEN_PAREN || lasttoken.kind == TK_SEPARATOR)
      {
        lexer_dump_err(token.loc, stderr, "Unexpected " SV_Fmt ", expected expression", SV_Arg(token.content));
        fprintf(stderr, LOC_FMT ": NOTE: Preceded by " SV_Fmt "\n", LOC_ARG(lasttoken.loc), SV_Arg(lasttoken.content));
        return MERR_UNEXPECTED_OPERATOR;
      }
      MathOperator top_op;
      size_t len;
      while ((len = arrlenu(parser->operator_stack)) > 0)
      {
        top_op = math_parser_last_op(parser);
        if (top_op.token.kind != TK_OP) break; // drain until (
        arrput(parser->output_queue, arrpop(parser->operator_stack));
      }
      assert(len > 0 && "Should have been caught at top");
      assert(parser->operator_stack[len - 1].token.kind == TK_OPEN_PAREN);
      if (len == 1 || !parser->operator_stack[len - 2].function)
      {
        lexer_dump_err(token.loc, stderr, "Got separator without function call in parenthesis");
        return MERR_UNBALANCED_PARENTHESIS;
      }
      parser->operator_stack[len - 2].nargs += 1;
    } break;
    case TK_OPEN_PAREN: {
      math_parser_check_implicit_mult(parser, token, lasttoken);
      MathOperator op = (MathOperator) {
        .token = token,
        .precedence = 100,
      };
      arrput(parser->operator_stack, op);
      ++parser->paren_depth;
    } break;
    case TK_CLOSE_PAREN: {
      if (lasttoken.kind == TK_OP)
      {
        lexer_dump_err(token.loc, stderr, "Unexpected ), expected expression");
        fprintf(stderr, LOC_FMT ": NOTE: Preceded by this operator " SV_Fmt "\n", LOC_ARG(lasttoken.loc), SV_Arg(lasttoken.content));
        return MERR_UNEXPECTED_OPERATOR;
      }
      else if (lasttoken.kind == TK_OPEN_PAREN || lasttoken.kind == TK_SEPARATOR)
      {
        lexer_dump_err(token.loc, stderr, "Unexpected " SV_Fmt ", expected expression", SV_Arg(token.content));
        fprintf(stderr, LOC_FMT ": NOTE: Preceded by " SV_Fmt "\n", LOC_ARG(lasttoken.loc), SV_Arg(lasttoken.content));
        return MERR_UNEXPECTED_OPERATOR;
      }
      MathOperator top_op;
      size_t len;
      while ((len = arrlenu(parser->operator_stack)) > 0)
      {
        top_op = math_parser_last_op(parser);
        if (top_op.token.kind != TK_OP) break; // drain until (
        arrput(parser->output_queue, arrpop(parser->operator_stack));
      }
      if (len == 0)
      {
        lexer_dump_err(token.loc, stderr, "Unbalanced parenthesis, got ) without prior (");
        return MERR_UNBALANCED_PARENTHESIS;
      }
      assert(parser->operator_stack[len - 1].token.kind == TK_OPEN_PAREN);
      (void) arrpop(parser->operator_stack);
      assert(parser->paren_depth > 0);
      --parser->paren_depth;

      len = arrlenu(parser->operator_stack);
      if (len > 0 && parser->operator_stack[len - 1].function)
      {
        arrput(parser->output_queue, arrpop(parser->operator_stack));
      }
    } break;
    case TK_ASSIGN: {
      lexer_dump_err(token.loc, stderr, "Unexpected assignment inside expression");
        fprintf(stderr, LOC_FMT ": NOTE: Assignment is only legal at the beginning of an expression, immediately following a variable, or in a chain of assignments.\n", LOC_ARG(token.loc));
      return MERR_UNEXPECTED_OPERATOR;
    } break;
  }
  return MERR_OK;
}

static MathParserError math_parser_parse_function_def(MathParser *parser, MathUserFunction *fn)
{
  // NOTE: This function will always return OK if input is not faulty
  // To check if a function was actually parsed, check it's contents
  LexerError lerr;
  MathParserError err = MERR_OK;
  Lexer peek = parser->lexer;
  Token function_name, peek_token;
  Token error_token;
  String_View *arguments = NULL;
  // doesn't start with a symbol -> give up immediately
  if ((lerr = lexer_next_token(&peek, &function_name)) != LERR_OK || function_name.kind != TK_SYMBOL) return MERR_OK;
  if ((lerr = lexer_next_token(&peek, &peek_token)) != LERR_OK || peek_token.kind != TK_OPEN_PAREN) return MERR_OK;
  // now we get the parameters, first a symbol, then a closing bracket or comma
  if ((lerr = lexer_peek(&peek, &peek_token)) == LERR_OK && peek_token.kind == TK_CLOSE_PAREN)
  {
    lerr = lexer_next_token(&peek, &function_name);
    assert(lerr == LERR_OK && "how, we just peeked fine");
    assert(function_name.content.count > 0 && "how did we get here lexer");
    *fn = (MathUserFunction) {
      .name = sv_dup(function_name.content),
      .nargs = 0,
    };
    parser->lexer = peek; // make sure RPN starts from here
    return MERR_OK;
  }
  for (;;)
  {
    Token argument_name;
    if ((lerr = lexer_next_token(&peek, &argument_name)) != LERR_OK || argument_name.kind != TK_SYMBOL) goto check_is_fn;
    String_View name = sv_dup(argument_name.content);
    arrput(arguments, name);
    if ((lerr = lexer_next_token(&peek, &peek_token)) != LERR_OK) RETURN(MERR_OK);
    if (peek_token.kind == TK_SEPARATOR) continue; // next argument
    if (peek_token.kind == TK_CLOSE_PAREN) break; // done
    goto check_is_fn;
  }
  if ((lerr = lexer_next_token(&peek, &peek_token)) != LERR_OK || peek_token.kind != TK_ASSIGN) RETURN(MERR_OK);
  if (math_parser_has_function(parser, function_name.content, arrlenu(arguments)))
  {
    lexer_dump_err(function_name.loc, stderr, "Function " SV_Fmt "already defined", SV_Arg(function_name.content));
    return MERR_SYMBOL_ALREADY_SET;
  }
  *fn = (MathUserFunction) {
    .name = sv_dup(function_name.content),
    .nargs = arrlenu(arguments),
    .argument_names = arguments,
  };
  printf("FN: " SV_Fmt ", args: %zu\n", SV_Arg(fn->name), arrlen(fn->argument_names));
  arguments = NULL;
  parser->lexer = peek; // make sure RPN starts from here

return_defer:
  if (arguments) arrfree(arguments); // error case, make sure all arguments are free'd properly
  return err;

check_is_fn:
  // error case
  // it looked like a function definition, but we got something that wasn't a valid argument definition
  // now check if it actually was a function definition, or if we are trying to parse a function call actually
  error_token = peek_token;
  do {
    if (peek_token.kind == TK_CLOSE_PAREN)
    {
      // got a ) followed by something that wasn't =, assume this is not a function definition and bail
      if ((lerr = lexer_next_token(&peek, &peek_token)) != LERR_OK || peek_token.kind != TK_ASSIGN) return MERR_OK;
      lexer_dump_err(error_token.loc, stderr, "Token %s not valid in function definition, expected a list of arguments, got " SV_Fmt, lexer_strtokenkind(error_token.kind), SV_Arg(error_token.content));
      RETURN(MERR_OPERATOR_ERROR);
    }
  } while ((lerr = lexer_next_token(&peek, &peek_token)) == LERR_OK);
  goto return_defer;
}

static MathParserError math_parser_check_operand(const MathOperator operand, double *doubleval)
{
  if (operand.token.kind != TK_INTEGER && operand.token.kind != TK_REAL)
  {
    lexer_dump_err(operand.token.loc, stderr, "Expected a number, got " SV_Fmt, SV_Arg(operand.token.content));
    return MERR_OPERATOR_ERROR;
  }
  if (doubleval)
  {
    *doubleval = operand.token.kind == TK_INTEGER ? (double) operand.token.as.integer.value : operand.token.as.real.value;
  }
  return MERR_OK;
}

static MathParserError math_parser_handle_binary(const MathOperator left, const MathOperator right, const MathOperator op, MathOperator *res)
{
  MathParserError err = MERR_OK;
  double left_value, right_value;
  MATH_PARSER_TRY(math_parser_check_operand(left, &left_value));
  MATH_PARSER_TRY(math_parser_check_operand(right, &right_value));
  assert(op.token.kind == TK_OP);
  if (left.token.kind == TK_INTEGER && right.token.kind == TK_INTEGER && op.token.as.op != OP_DIV && op.token.as.op != OP_EXP)
  {
    int16_t result;
    switch (op.token.as.op) {
      case OP_ADD:
        result = left.token.as.integer.value + right.token.as.integer.value;
        break;
      case OP_SUB:
        result = left.token.as.integer.value - right.token.as.integer.value;
        break;
      case OP_MUL:
        result = left.token.as.integer.value * right.token.as.integer.value;
        break;
      case OP_DIV:
      case OP_EXP:
        assert(0 && "unreachable");
    }
    *res = (MathOperator) {
      .token = {
        .kind = TK_INTEGER,
        .loc = op.token.loc,
        .as = {
          .integer = {
            .value = result,
          }
        }
      }
    };
    return MERR_OK;
  }
  double result;
  switch (op.token.as.op) {
    case OP_ADD:
      result = left_value + right_value;
      break;
    case OP_SUB:
      result = left_value - right_value;
      break;
    case OP_MUL:
      result = left_value * right_value;
      break;
    case OP_DIV:
      result = left_value / right_value;
      break;
    case OP_EXP:
      result = pow(left_value, right_value);
      break;
  }
  *res = (MathOperator) {
    .token = {
      .kind = TK_REAL,
      .loc = op.token.loc,
      .as = {
        .real = {
          .value = result,
        }
      }
    }
  };
return_defer:
  return err;
}

static MathParserError math_parser_handle_unary(const MathOperator operand, const MathOperator op, MathOperator *res)
{
  MathParserError err = MERR_OK;
  MATH_PARSER_TRY(math_parser_check_operand(operand, NULL));
  if (op.token.as.op == OP_ADD)
  {
    *res = operand;
    RETURN(MERR_OK);
  }
  assert(op.token.as.op == OP_SUB && "only + and - should be allowed as unary");
  if (operand.token.kind == TK_INTEGER)
  {
    *res = (MathOperator) {
      .token = {
        .kind = TK_INTEGER,
        .loc = op.token.loc,
        .as = {
          .integer = {
            .value = -operand.token.as.integer.value,
          }
        }
      }
    };
    RETURN(MERR_OK);
  }
  *res = (MathOperator) {
    .token = {
      .kind = TK_REAL,
      .loc = op.token.loc,
      .as = {
        .real = {
          .value = -operand.token.as.real.value,
        }
      }
    }
  };
return_defer:
  return err;
}

static MathParserError math_parser_handle_function(MathParser *parser, const MathOperator *stack, const MathOperator op, MathOperator *res)
{
  const MathOperator arg = arrpop(stack);
  MathParserError err;
  double dvalue;
  MATH_PARSER_TRY(math_parser_check_operand(arg, &dvalue));
  for (size_t i = 0; i < ALEN(MATH_PARSER_BUILTIN_FUNCTIONS); ++i)
  {
    if (sv_eq_ignorecase(op.token.content, MATH_PARSER_BUILTIN_FUNCTIONS[i].name) && op.nargs == MATH_PARSER_BUILTIN_FUNCTIONS[i].nargs)
    {
      double result;
      switch (MATH_PARSER_BUILTIN_FUNCTIONS[i].nargs) {
        case 1: result = MATH_PARSER_BUILTIN_FUNCTIONS[i].as.unary(dvalue); break;
        case 2: {
          const MathOperator arg1 = arrpop(stack);
          double dvalue1;
          MATH_PARSER_TRY(math_parser_check_operand(arg1, &dvalue1));
          result = MATH_PARSER_BUILTIN_FUNCTIONS[i].as.binary(dvalue1, dvalue);
        } break;
        default: assert(0 && "unreachable"); break;
      }
      *res = (MathOperator) {
        .token = {
          .kind = TK_REAL,
          .loc = op.token.loc,
          .as = {
            .real = {
              .value = result,
            }
          }
        }
      };
      return MERR_OK;
    }
  }
  // TODO: Implement calling function RPN
  lexer_dump_err(op.token.loc, stderr, "Unrecognized function " SV_Fmt " with %zu argument(s)", SV_Arg(op.token.content), op.nargs);
  for (size_t i = 0; i < ALEN(MATH_PARSER_BUILTIN_FUNCTIONS); ++i)
  {
    if (sv_eq_ignorecase(op.token.content, MATH_PARSER_BUILTIN_FUNCTIONS[i].name))
    {
      fprintf(stderr, "NOTE: This function exists with %zu argument(s)\n", MATH_PARSER_BUILTIN_FUNCTIONS[i].nargs);
    }
  }
  size_t size = arrlenu(parser->functions);
  for (size_t i = 0; i < size; ++i)
  {
    if (sv_eq_ignorecase(op.token.content, parser->functions[i].name))
    {
      fprintf(stderr, "NOTE: This function exists with %zu argument(s)\n", parser->functions[i].nargs);
    }
  }
  return MERR_UNRECOGNIZED_SYMBOL;
return_defer:
  return err;
}

static MathParserError math_parser_handle_variable(MathParser *parser, const MathOperator var, MathOperator *res)
{
  double value;
  if (math_parser_get_var(parser, var.token.content, &value))
  {
    *res = (MathOperator) {
      .token = {
        .kind = TK_REAL,
        .loc = var.token.loc,
        .as = {
          .real = {
            .value = value,
          }
        }
      }
    };
    return MERR_OK;
  }
  lexer_dump_err(var.token.loc, stderr, "Unrecognized variable " SV_Fmt, SV_Arg(var.token.content));
  return MERR_UNRECOGNIZED_SYMBOL;
}

static void math_parser_output_dup(MathParser *parser, MathUserFunction *fn)
{
  String_View new_full = sv_dup(parser->lexer.start);
  fn->lexer_content = new_full;
  ptrdiff_t diff = new_full.data - parser->lexer.start.data;
  // now update all strings to new pointer
  size_t size = arrlenu(parser->output_queue);
  for (size_t i = 0; i < size; ++i)
  {
    parser->output_queue[i].token.content.data += diff;
  }
  fn->rpn = parser->output_queue;
  parser->output_queue = NULL;
  MathOperator zero = (MathOperator) {
    .token = {
      .kind = TK_INTEGER,
      .as = {
        .integer = {
          .value = 0,
        },
      },
    },
  };
  arrput(parser->output_queue, zero);
}

static void math_parser_function_free(MathUserFunction function)
{
  free((char *)function.name.data);
  size_t size = arrlenu(function.argument_names);
  for (size_t j = 0; j < size; ++j)
  {
    free((char *)function.argument_names[j].data);
  }
  arrfree(function.argument_names);
  free((char *)function.lexer_content.data);
  arrfree(function.rpn);
}

// Implementation

MathParser math_parser_init(Lexer lexer)
{
  return (MathParser) {
    .lexer = lexer,
    .output_queue = NULL,
    .operator_stack = NULL,
  };
}

MathParserError math_parser_rpn(MathParser *parser)
{
  assert(parser != NULL);
  LexerError lerr;
  MathParserError err = MERR_OK;
  Token token;
  Token lasttoken = (Token) {
    .kind = TK_OPEN_PAREN,
  };
  MathUserFunction function = {0};
  parser->paren_depth = 0;
  {
    Lexer peek = parser->lexer;
    while ((lerr = lexer_next_token(&peek, &token)) == LERR_OK)
    {
      if (token.kind != TK_SYMBOL) break;
      Token peektoken;
      if (lexer_next_token(&peek, &peektoken) != LERR_OK || peektoken.kind != TK_ASSIGN) break;
      // got `<var> =`, push the var to the operator stack -> will be evaluated last
      // make sure to not include it in actual parsing
      MathOperator var = (MathOperator) {
        .token = token,
        .precedence = -1000,
        .assignment = true,
      };
      arrput(parser->operator_stack, var);
      parser->lexer = peek;
    }
  }
  {
    // Nothing happened yet -> function definition still legal
    if (arrlenu(parser->operator_stack) == 0)
    {
      MATH_PARSER_TRY(math_parser_parse_function_def(parser, &function));
    }
  }
  while ((lerr = lexer_next_token(&parser->lexer, &token)) == LERR_OK)
  {
    // separate equations
    if (token.kind == TK_SEPARATOR && parser->paren_depth == 0) break;
    MATH_PARSER_TRY(math_parser_parse_one_token(parser, token, lasttoken));
    lasttoken = token;
  }
  if (lerr != LERR_EOF && lerr != LERR_OK)
    RETURN(MERR_LEXER_ERROR);
  if (lasttoken.kind == TK_OP)
  {
    lexer_dump_err(parser->lexer.loc, stderr, "Unexpected end of input, expected expression");
    fprintf(stderr, LOC_FMT ": NOTE: Preceded by this operator " SV_Fmt "\n", LOC_ARG(lasttoken.loc), SV_Arg(lasttoken.content));
    RETURN(MERR_UNEXPECTED_OPERATOR);
  }
  MathOperator top_op;
  while (arrlenu(parser->operator_stack) > 0)
  {
    top_op = math_parser_last_op(parser);
    if (top_op.token.kind != TK_OP && !top_op.assignment)
    {
      lexer_dump_err(top_op.token.loc, stderr, "Unbalanced parenthesis, this ( was not closed");
      RETURN(MERR_UNBALANCED_PARENTHESIS);
    }
    arrput(parser->output_queue, arrpop(parser->operator_stack));
  }
  assert(parser->paren_depth == 0);
  if (function.name.count > 0)
  {
    // this moves the output_queue to the function
    math_parser_output_dup(parser, &function);
    arrput(parser->functions, function);
    return err; // NOTE: make sure not to go to defer, since it frees us
  }
return_defer:
  math_parser_function_free(function);
  return err;
}

MathParserError math_parser_eval(MathParser *parser, double *result)
{
  assert(parser != NULL);
  MathOperator op, opresult;
  MathOperator *stack = NULL;
  MathParserError err = MERR_OK;
  size_t size = arrlenu(parser->output_queue), i = 0;
  for (; i < size; ++i)
  {
    op = parser->output_queue[i];
    if (op.assignment) break;
    if (op.token.kind == TK_INTEGER || op.token.kind == TK_REAL)
    {
      arrput(stack, op);
      continue;
    }
    else if (op.token.kind == TK_SYMBOL && !op.function)
    {
      MATH_PARSER_TRY(math_parser_handle_variable(parser, op, &opresult));
      arrput(stack, opresult);
      continue;
    }
    else if (op.token.kind != TK_OP && op.token.kind != TK_SYMBOL)
    {
      lexer_dump_err(op.token.loc, stderr, "Expected operator, got " SV_Fmt, SV_Arg(op.token.content));
      RETURN(MERR_OPERATOR_ERROR);
    }
    if (arrlenu(stack) < op.nargs)
    {
      lexer_dump_err(op.token.loc, stderr, "Not enough operands for operator " SV_Fmt, SV_Arg(op.token.content));
      RETURN(MERR_OPERATOR_ERROR);
    }
    if (op.function)
    {
      MATH_PARSER_TRY(math_parser_handle_function(parser, stack, op, &opresult));
    }
    else if (op.nargs == 1)
    {
      MATH_PARSER_TRY(math_parser_handle_unary(arrpop(stack), op, &opresult));
    }
    else if (op.nargs == 2)
    {
      MathOperator right = arrpop(stack);
      MathOperator left  = arrpop(stack);
      MATH_PARSER_TRY(math_parser_handle_binary(left, right, op, &opresult));
    }
    else
    {
      assert(0 && "unexpected arity");
    }
    arrput(stack, opresult);
  }
  size = arrlenu(stack);
  if (size == 0)
  {
    RETURN(MERR_INPUT_EMPTY);
  }
  if (size > 1)
  {
    lexer_dump_err(parser->lexer.loc, stderr, "Unconsumed input on stack");
    RETURN(MERR_OPERATOR_ERROR);
  }
  opresult = arrpop(stack);
  MATH_PARSER_TRY(math_parser_check_operand(opresult, result));
  // handle assignments now
  size = arrlenu(parser->output_queue);
  for (; i < size; ++i)
  {
    op = parser->output_queue[i];
    assert(op.assignment && "Expected to only have assignments on the stack by now");
    if (!math_parser_set_var(parser, op.token.content, *result))
    {
      lexer_dump_err(op.token.loc, stderr, "Variable with name " SV_Fmt " already set", SV_Arg(op.token.content));
      double val;
      bool worked = math_parser_get_var(parser, op.token.content, &val);
      assert(worked && "We just got a fail...");
      fprintf(stderr, "NOTE: It has this value: %lf\n", val);
      RETURN(MERR_SYMBOL_ALREADY_SET);
    }
  }
  // should be pop from front -> iterate, then clear
  // allows to reuse allocated memory for next run
  arrsetlen(parser->output_queue, 0);
return_defer:
  arrfree(stack);
  return err;
}

void math_parser_free(MathParser *parser)
{
  assert(parser != NULL);
  arrfree(parser->output_queue);
  arrfree(parser->operator_stack);
  size_t size = arrlenu(parser->variables);
  for (size_t i = 0; i < size; ++i)
  {
    free((void *) parser->variables[i].name.data);
  }
  arrfree(parser->variables);
  size = arrlenu(parser->functions);
  for (size_t i = 0; i < size; ++i)
  {
    MathUserFunction function = parser->functions[i];
    math_parser_function_free(function);
  }
  arrfree(parser->functions);
}

void math_parser_clear(MathParser *parser)
{
  assert(parser != NULL);
  arrsetlen(parser->output_queue, 0);
  arrsetlen(parser->operator_stack, 0);
}

MathParserError math_parser_evaluate_input(MathParser *parser, Lexer input, double *result)
{
  assert(parser != NULL);
  assert(result != NULL);
  assert(arrlenu(parser->operator_stack) == 0 && "Unclean parser given");
  assert(arrlenu(parser->output_queue) == 0 && "Unclean parser given");
  parser->lexer = input;
  MathParserError err = MERR_INPUT_EMPTY;
  while (parser->lexer.content.count > 0)
  {
    MATH_PARSER_TRY(math_parser_rpn(parser));
    err = math_parser_eval(parser, result);
    if (err == MERR_INPUT_EMPTY) continue;
    MATH_PARSER_TRY(err);
    err = MERR_OK;
  }
return_defer:
  if (err == MERR_INPUT_EMPTY)
  {
    lexer_dump_err(parser->lexer.loc, stderr, "Input empty");
  }
  return err;
}

bool math_parser_set_var(MathParser *parser, String_View name, double value)
{
  if (math_parser_get_var(parser, name, NULL)) return false;
  MathVariable var = (MathVariable) {
    .name = sv_dup(name),
    .value = value,
  };
  arrput(parser->variables, var);
  return true;
}

bool math_parser_get_var(MathParser *parser, String_View name, double *value)
{
  for (size_t i = 0; i < ALEN(MATH_PARSER_BUILTIN_CONSTANTS); ++i)
  {
    if (sv_eq_ignorecase(name, MATH_PARSER_BUILTIN_CONSTANTS[i].name))
    {
      if (value) *value = MATH_PARSER_BUILTIN_CONSTANTS[i].value;
      return true;
    }
  }
  size_t size = arrlenu(parser->variables);
  for (size_t i = 0; i < size; ++i)
  {
    if (sv_eq_ignorecase(name, parser->variables[i].name))
    {
      if (value) *value = parser->variables[i].value;
      return true;
    }
  }
  return false;
}
