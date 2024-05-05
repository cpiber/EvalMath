#include "lexer.h"
#include "rpn.h"
#include "stb_ds.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
  for (size_t i = 1; i < argc; ++i)
  {
    Lexer lex = lexer_init("arg", sv_from_cstr(argv[i]));
    MathParser parser = math_parser_init(lex);
    MathParserError err = math_parser_rpn(&parser);
    if (err != MERR_OK)
    {
      fprintf(stderr, "Parser stopped abnormally with error %d\n", err);
    }
    else
    {
      printf("RPN:");
      size_t size = arrlenu(parser.output_queue);
      for (size_t i = 0; i < size; ++i)
      {
        printf(" " SV_Fmt, SV_Arg(parser.output_queue[i].token.content));
      }
      printf("\n");
      double result;
      err = math_parser_eval(&parser, &result);
      if (err != MERR_OK)
        fprintf(stderr, "Evaluation stopped abnormally with error %d\n", err);
      else printf("Result: %lf\n", result);
    }
    math_parser_free(&parser);
  }
}
