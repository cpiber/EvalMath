#include "rpn.h"
#include "lexer.h"
#define STB_DS_IMPLEMENTATION
#include "stb_ds.h"

// Private functions

static MathOperator math_parser_last_op(MathParser *parser)
{
  size_t len = arrlenu(parser->operator_stack);
  assert(len > 0);
  return parser->operator_stack[len - 1];
}

static int math_parser_precedence(Token operator, bool unary)
{
  assert(operator.kind == TK_OP);
  switch (operator.as.op) {
    case OP_ADD:
    case OP_SUB:
      return unary ? 2 : 4;
    case OP_MUL:
    case OP_DIV:
      return 3;
  }
  assert(0 && "unreachable");
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
    switch (token.kind) {
      case TK_INTEGER:
      case TK_REAL:
        arrput(parser->output_queue, (MathOperator) {.token=token});
        break;
      case TK_OP: {
        bool unary = false;
        if ((token.as.op == OP_ADD || token.as.op == OP_SUB) && (lasttoken.kind == TK_OPEN_PAREN || lasttoken.kind == TK_OP))
        {
          unary = true;
        }
        else if (lasttoken.kind == TK_OP)
        {
          lexer_dump_err(token.loc, stderr, "Unexpected operator " SV_Fmt, SV_Arg(token.content));
          fprintf(stderr, LOC_FMT ": NOTE: Preceded by this operator " SV_Fmt "\n", LOC_ARG(lasttoken.loc), SV_Arg(lasttoken.content));
          return MERR_UNEXPECTED_OPERATOR;
        }
        MathOperator op = (MathOperator) {
          .token = token,
          .precedence = math_parser_precedence(token, unary),
          .nargs = unary ? 1 : 2,
        };

        MathOperator top_op;
        while (arrlenu(parser->operator_stack) > 0)
        {
          top_op = math_parser_last_op(parser);
          if (top_op.token.kind != TK_OP) break; // not an operator -> parenthesis, stop
          if (op.precedence < top_op.precedence) break; // must have greater precedence
          // TODO: handle associativity
          arrput(parser->output_queue, arrpop(parser->operator_stack));
        }
        arrput(parser->operator_stack, op);
      } break;
      case TK_OPEN_PAREN: {
        MathOperator op = (MathOperator) {
          .token = token,
          .precedence = 100,
        };
        arrput(parser->operator_stack, op);
      } break;
      case TK_CLOSE_PAREN: {
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
        // TODO: handle function
      } break;
    }
    lasttoken = token;
  }
  MathOperator top_op;
  while (arrlenu(parser->operator_stack) > 0)
  {
    top_op = math_parser_last_op(parser);
    if (top_op.token.kind != TK_OP)
    {
      lexer_dump_err(token.loc, stderr, "Unbalanced parenthesis, this ( was not closed");
      return MERR_UNBALANCED_PARENTHESIS;
    }
    arrput(parser->output_queue, arrpop(parser->operator_stack));
  }
  return MERR_OK;
}
