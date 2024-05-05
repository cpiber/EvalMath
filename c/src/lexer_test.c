#include "lexer.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
  for (size_t i = 1; i < argc; ++i)
  {
    Lexer lex = lexer_init("arg", sv_from_cstr(argv[i]));
    printf("Tokenizing input %zu: " SV_Fmt "\n", i, SV_Arg(lex.content));
    LexerError err;
    Token token;
    while ((err = lexer_next_token(&lex, &token)) == LERR_OK)
    {
      lexer_dump_token(token);
    }
    if (err != LERR_EOF)
      fprintf(stderr, "Lexer stopped abnormally with error %d (%s)\n", err, lexer_strerr(err));
  }
}
