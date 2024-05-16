#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>
#include <errno.h>

#include "lexer.h"
#define SV_IMPLEMENTATION
#include "sv.h"

// Helpers

static bool isspace_except_newline(char c)
{
  return c != '\n' && isspace(c);
}

static bool isdigit_(char c)
{
  return isdigit(c);
}

static bool not_isspace(char c)
{
  return !isspace(c);
}

static bool isident(char c)
{
  return isalnum(c) || c == '_';
}

static LexerError sv_parse_double(Location loc, String_View sv, double *result)
{
  char *end;
  errno = 0;
  *result = strtod(sv.data, &end);
  if (end > sv.data + sv.count)
  {
    lexer_dump_err(loc, stderr, "(internal) Conversion of " SV_Fmt " consumed more tokens than expected, result is %lf", SV_Arg(sv), *result);
    return LERR_INVALID_LITERAL;
  }
  if ((*result == HUGE_VAL || *result == -HUGE_VAL) && errno == ERANGE)
  {
    lexer_dump_err(loc, stderr, "Overflow caused by conversion of literal " SV_Fmt, SV_Arg(sv));
    return LERR_INVALID_LITERAL;
  }
  if (errno == ERANGE)
  {
    lexer_dump_err(loc, stderr, "Underflow caused by conversion of literal " SV_Fmt, SV_Arg(sv));
    return LERR_INVALID_LITERAL;
  }
  if (end < sv.data + sv.count)
  {
    lexer_dump_err(loc, stderr, "(internal) Conversion of " SV_Fmt " consumed less tokens than expected, result is %lf", SV_Arg(sv), *result);
    return LERR_INVALID_LITERAL;
  }
  return LERR_OK;
}

static LexerError sv_parse_longlong(Location loc, String_View sv, int base, long long *result)
{
  char *end;
  errno = 0;
  *result = strtoll(sv.data, &end, base);
  if (end > sv.data + sv.count)
  {
    lexer_dump_err(loc, stderr, "(internal) Conversion of " SV_Fmt " consumed more tokens than expected, result is %lld", SV_Arg(sv), *result);
    return LERR_INVALID_LITERAL;
  }
  if (*result == LLONG_MAX && errno == ERANGE)
  {
    lexer_dump_err(loc, stderr, "Overflow caused by conversion of literal " SV_Fmt, SV_Arg(sv));
    return LERR_INVALID_LITERAL;
  }
  if (*result == LLONG_MIN && errno == ERANGE)
  {
    lexer_dump_err(loc, stderr, "Underflow caused by conversion of literal " SV_Fmt, SV_Arg(sv));
    return LERR_INVALID_LITERAL;
  }
  if (end < sv.data + sv.count)
  {
    lexer_dump_err(loc, stderr, "(internal) Conversion of " SV_Fmt " consumed less tokens than expected, result is %lld", SV_Arg(sv), *result);
    return LERR_INVALID_LITERAL;
  }
  return LERR_OK;
}

// Private functions

static void lexer_remove_whitespace(Lexer *lexer)
{
  String_View removed;
  do {
    removed = sv_chop_left_while(&lexer->content, isspace_except_newline);
    lexer->loc.col += removed.count;
    if (lexer->content.count > 0 && lexer->content.data[0] == '\n')
    {
      removed = sv_chop_left(&lexer->content, 1);
      lexer->loc.line += 1;
      lexer->loc.col = 0;
    }
  } while (removed.count > 0);
}

// TODO: other literal bases (hex, oct?), scientific notation
static LexerError lexer_consume_digit(Lexer *lexer, Token *token)
{
  String_View dig = sv_chop_left_while(&lexer->content, isdigit_);
  if (lexer->content.count > 0 && lexer->content.data[0] == '.')
  {
    sv_chop_left(&lexer->content, 1);
    dig.count += 1;
    dig.count += sv_chop_left_while(&lexer->content, isdigit_).count;
    if (dig.count == 1) // . only
    {
      lexer_dump_err(lexer->loc, stderr, "Invalid number literal " SV_Fmt, SV_Arg(dig));
      return LERR_INVALID_LITERAL;
    }
    double value;
    LEXER_TRY(sv_parse_double(lexer->loc, dig, &value));
    *token = (Token) {
      .loc = lexer->loc,
      .content = dig,
      .kind = TK_REAL,
      .as = {
        .real = {
          .value = value,
        }
      }
    };
  }
  else
  {
    long long value;
    LEXER_TRY(sv_parse_longlong(lexer->loc, dig, 10, &value));
    *token = (Token) {
      .loc = lexer->loc,
      .content = dig,
      .kind = TK_INTEGER,
      .as = {
        .integer = {
          .value = value,
        }
      }
    };
  }
  assert(dig.count > 0 && "how did we get here");
  lexer->loc.col += dig.count;
  return LERR_OK;
}

// Implementation

Lexer lexer_init(char *file, String_View content)
{
  return (Lexer) {
    .file = file,
    .content = content,
    .start = content,
    .loc = (Location) {
      .file = file,
      .line = 1,
      .col = 0,
    },
  };
}

LexerError lexer_peek(Lexer *lexer, Token *token)
{
  assert(lexer != NULL);
  assert(token != NULL);
  if (lexer->loc.line == 0) lexer->loc.line = 1;
  if (lexer->content.count == 0) return LERR_EOF;
  Lexer peek = *lexer; // NOTE: copy is cheap, only a few pointers and integers
  return lexer_next_token(&peek, token);
}

LexerError lexer_next_token(Lexer *lexer, Token *token)
{
  assert(lexer != NULL);
  assert(token != NULL);
  if (lexer->loc.line == 0) lexer->loc.line = 1;
  if (lexer->content.count == 0) return LERR_EOF;
  lexer_remove_whitespace(lexer);
  if (lexer->content.count == 0) return LERR_EOF;

  char c = lexer->content.data[0];
  assert(!isspace(c) && "lexer_remove_whitespace did not do its job");
  if (isdigit(c) || c == '.') // allow .5
  {
    return lexer_consume_digit(lexer, token);
  }
#define MAP(_c, _kind)                                 \
  else if (c == _c)                                    \
  {                                                    \
    String_View op = sv_chop_left(&lexer->content, 1); \
    *token = (Token) {                                 \
      .loc = lexer->loc,                               \
      .content = op,                                   \
      .kind = TK_OP,                                   \
      .as = {                                          \
        .op = _kind,                                   \
      }                                                \
    };                                                 \
    lexer->loc.col += 1;                               \
    return LERR_OK;                                    \
  }
  MAP('-', OP_SUB)
  MAP('+', OP_ADD)
  MAP('*', OP_MUL)
  MAP('/', OP_DIV)
  MAP('^', OP_EXP)
#undef MAP
#define MAP(_c, _kind)                                 \
  else if (c == _c)                                    \
  {                                                    \
    String_View op = sv_chop_left(&lexer->content, 1); \
    *token = (Token) {                                 \
      .loc = lexer->loc,                               \
      .content = op,                                   \
      .kind = _kind,                                   \
    };                                                 \
    lexer->loc.col += 1;                               \
    return LERR_OK;                                    \
  }
  MAP('(', TK_OPEN_PAREN)
  MAP(')', TK_CLOSE_PAREN)
  MAP(',', TK_SEPARATOR)
  MAP(';', TK_SEPARATOR)
  MAP('=', TK_ASSIGN)
#undef MAP
  else if (isalpha(c))
  {
    String_View symb = sv_chop_left_while(&lexer->content, isident);
    assert(symb.count > 0 && "how did we get here");
    *token = (Token) {
      .loc = lexer->loc,
      .content = symb,
      .kind = TK_SYMBOL,
    };
    lexer->loc.col += symb.count;
    return LERR_OK;
  }

  String_View preview = lexer->content;
  preview = sv_chop_left_while(&preview, not_isspace);
  if (preview.count > 10) preview.count = 10;
  lexer_dump_err(lexer->loc, stderr, "Unrecognized token starts with " SV_Fmt, SV_Arg(preview));
  return LERR_UNRECOGNIZED_TOKEN;
}

void lexer_dump_err(Location loc, FILE *stream, char *fmt, ...) {
  fprintf(stream, LOC_FMT ": ERROR: ", LOC_ARG(loc));
  va_list args;
  va_start(args, fmt);
  vfprintf(stream, fmt, args);
  va_end(args);
  fputc('\n', stream);
}

const char *lexer_strtokenkind(TokenKind kind)
{
  switch (kind) {
    case TK_INTEGER: return "INTEGER";
    case TK_REAL: return "REAL";
    case TK_OP: return "OP";
    case TK_OPEN_PAREN: return "OPEN_PAREN";
    case TK_CLOSE_PAREN: return "CLOSE_PAREN";
    case TK_SYMBOL: return "SYMBOL";
    case TK_SEPARATOR: return "SEPARATOR";
    case TK_ASSIGN: return "ASSIGN";
  }
  assert(0 && "unreachable");
}

void lexer_dump_token(Token token)
{
  printf(LOC_FMT ": %s " SV_Fmt, LOC_ARG(token.loc), lexer_strtokenkind(token.kind), SV_Arg(token.content));
  switch (token.kind) {
    case TK_INTEGER:
      printf(" (%" PRIi64 ")\n", token.as.integer.value);
      break;
    case TK_REAL:
      printf(" (%lf)\n", token.as.real.value);
      break;
    case TK_OP:
    case TK_OPEN_PAREN:
    case TK_CLOSE_PAREN:
    case TK_SYMBOL:
    case TK_SEPARATOR:
    case TK_ASSIGN:
      printf("\n");
      break;
  }
}

const char *lexer_strerr(LexerError err)
{
  switch (err) {
    case LERR_OK: return "OK";
    case LERR_EOF: return "EOF";
    case LERR_INVALID_LITERAL: return "INVALID_LITERAL";
    case LERR_UNRECOGNIZED_TOKEN: return "UNRECOGNIZED_TOKEN";
  }
  assert(0 && "unreachable");
}
