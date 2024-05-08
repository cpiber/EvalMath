#include "rpn.h"
#include "lexer.h"
#include <math.h>
#include <math.h>
#include <stdint.h>
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

// Private functions

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
      if (lexer_next_token(&peek_lexer, &peek) == LERR_OK && peek.kind == TK_OPEN_PAREN)
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
      if (len == 0)
      {
        lexer_dump_err(token.loc, stderr, "Got separator without function call; multiple equations are not implemented yet");
        return MERR_UNBALANCED_PARENTHESIS;
      }
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

      len = arrlenu(parser->operator_stack);
      if (len > 0 && parser->operator_stack[len - 1].function)
      {
        arrput(parser->output_queue, arrpop(parser->operator_stack));
      }
    } break;
  }
  return MERR_OK;
}

static MathParserError math_parser_check_operand(const MathOperator operand)
{
  if (operand.token.kind != TK_INTEGER && operand.token.kind != TK_REAL)
  {
    lexer_dump_err(operand.token.loc, stderr, "Expected a number, got " SV_Fmt, SV_Arg(operand.token.content));
    return MERR_OPERATOR_ERROR;
  }
  return MERR_OK;
}

static MathParserError math_parser_handle_binary(const MathOperator left, const MathOperator right, const MathOperator op, MathOperator *res)
{
  MathParserError err = MERR_OK;
  MATH_PARSER_TRY(math_parser_check_operand(left));
  MATH_PARSER_TRY(math_parser_check_operand(right));
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
  double left_value = left.token.kind == TK_INTEGER ? (double) left.token.as.integer.value : left.token.as.real.value;
  double right_value = right.token.kind == TK_INTEGER ? (double) right.token.as.integer.value : right.token.as.real.value;
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
  MATH_PARSER_TRY(math_parser_check_operand(operand));
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

static MathParserError math_parser_handle_function(MathParser *parser, MathOperator *stack, const MathOperator op, MathOperator *res)
{
  const MathOperator arg = arrpop(stack);
  MathParserError err;
  MATH_PARSER_TRY(math_parser_check_operand(arg));
  const double dvalue = arg.token.kind == TK_REAL ? arg.token.as.real.value : (double)arg.token.as.integer.value;
  for (size_t i = 0; i < ALEN(MATH_PARSER_BUILTIN_FUNCTIONS); ++i)
  {
    if (sv_eq_ignorecase(op.token.content, MATH_PARSER_BUILTIN_FUNCTIONS[i].name) && op.nargs == MATH_PARSER_BUILTIN_FUNCTIONS[i].nargs)
    {
      double result;
      switch (MATH_PARSER_BUILTIN_FUNCTIONS[i].nargs) {
        case 1: result = MATH_PARSER_BUILTIN_FUNCTIONS[i].as.unary(dvalue); break;
        case 2: {
          const MathOperator arg1 = arrpop(stack);
          MATH_PARSER_TRY(math_parser_check_operand(arg1));
          const double dvalue1 = arg1.token.kind == TK_REAL ? arg1.token.as.real.value : (double)arg1.token.as.integer.value;
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
  lexer_dump_err(op.token.loc, stderr, "Unrecognized function " SV_Fmt " with %zu argument(s)", SV_Arg(op.token.content), op.nargs);
  for (size_t i = 0; i < ALEN(MATH_PARSER_BUILTIN_FUNCTIONS); ++i)
  {
    if (sv_eq_ignorecase(op.token.content, MATH_PARSER_BUILTIN_FUNCTIONS[i].name))
    {
      fprintf(stderr, "NOTE: This function exists with %zu argument(s)\n", MATH_PARSER_BUILTIN_FUNCTIONS[i].nargs);
    }
  }
  return MERR_UNRECOGNIZED_SYMBOL;
return_defer:
  return err;
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
  LexerError err;
  Token token;
  Token lasttoken = (Token) {
    .kind = TK_OPEN_PAREN,
  };
  while ((err = lexer_next_token(&parser->lexer, &token)) == LERR_OK)
  {
    MathParserError err = math_parser_parse_one_token(parser, token, lasttoken);
    if (err != MERR_OK) return err;
    lasttoken = token;
  }
  if (err != LERR_EOF)
    return MERR_LEXER_ERROR;
  MathOperator top_op;
  while (arrlenu(parser->operator_stack) > 0)
  {
    top_op = math_parser_last_op(parser);
    if (top_op.token.kind != TK_OP)
    {
      lexer_dump_err(top_op.token.loc, stderr, "Unbalanced parenthesis, this ( was not closed");
      return MERR_UNBALANCED_PARENTHESIS;
    }
    arrput(parser->output_queue, arrpop(parser->operator_stack));
  }
  return MERR_OK;
}

MathParserError math_parser_eval(MathParser *parser, double *result)
{
  assert(parser != NULL);
  MathOperator op, opresult;
  MathOperator *stack = NULL;
  MathParserError err = MERR_OK;
  size_t size = arrlenu(parser->output_queue);
  for (size_t i = 0; i < size; ++i)
  {
    op = parser->output_queue[i];
    if (op.token.kind == TK_INTEGER || op.token.kind == TK_REAL)
    {
      arrput(stack, op);
      continue;
    }
    else if (op.token.kind == TK_SYMBOL && !op.function)
    {
      lexer_dump_err(op.token.loc, stderr, "Sorry, variables are not implemented yet");
      RETURN(MERR_OPERATOR_ERROR);
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
    lexer_dump_err(parser->lexer.loc, stderr, "Input empty");
    RETURN(MERR_INPUT_EMPTY);
  }
  if (size > 1)
  {
    lexer_dump_err(parser->lexer.loc, stderr, "Unconsumed input on stack");
    RETURN(MERR_OPERATOR_ERROR);
  }
  opresult = arrpop(stack);
  MATH_PARSER_TRY(math_parser_check_operand(opresult));
  *result = opresult.token.kind == TK_REAL ? opresult.token.as.real.value : (double) opresult.token.as.integer.value;
return_defer:
  arrfree(stack);
  return err;
}

void math_parser_free(MathParser *parser)
{
  assert(parser != NULL);
  arrfree(parser->output_queue);
  arrfree(parser->operator_stack);
}
