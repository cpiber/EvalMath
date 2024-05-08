#include <stdio.h>
#include <string.h>
#include "lexer.h"
#include "rpn.h"
#include "stb_ds.h"

#define CHECK(e) do { \
  ssize_t ret = (e);  \
  if (ret < 0) {      \
    exitcode = ret;   \
    goto ret;         \
  }                   \
} while(0)

int main(int argc, char **argv)
{
  MathParser parser = math_parser_init(EMPTY_LEXER);
  if (argc <= 1)
  {
    int exitcode = 0;
    char *input = NULL;
    size_t size = 0;
    double result;
    printf("Enter equation: ");
    ssize_t len = getline(&input, &size, stdin);
    CHECK(len);

    while (!feof(stdin) && !ferror(stdin))
    {
      result = 0;
      Lexer lex = lexer_init("stdin", sv_from_parts(input, len));
      if (input[len - 1] == '\n') lex.content.count -= 1;
      MathParserError err = math_parser_evaluate_input(&parser, lex, &result);
      if (err == MERR_OK)
      {
        printf("Result: %lf\n", result);
      }
      else if (err == MERR_INPUT_EMPTY)
      {
        break;
      }
      math_parser_clear(&parser);
      printf("Enter equation: ");
      len = getline(&input, &size, stdin);
      CHECK(len);
    }
ret:
    free(input);
    math_parser_free(&parser);
    return exitcode;
  }
  else
  {
    char *concat = NULL;
    double result = 0;
    for (size_t i = 1; i < argc; ++i)
    {
      size_t len = strlen(argv[i]);
      char *insert = arraddnptr(concat, len + 1);
      strncpy(insert, argv[i], len);
      insert[len] = ' ';
    }
    // -1 to account for extra space at end
    Lexer lex = lexer_init("args", sv_from_parts(concat, arrlenu(concat) - 1));
    MathParserError err = math_parser_evaluate_input(&parser, lex, &result);
    if (err == MERR_OK)
    {
      printf("Result: %lf\n", result);
    }
    arrfree(concat);
    math_parser_free(&parser);
    return 0;
  }
}
