#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "../src/rpn.h"
#include "../src/const.h"

#define assertEquals(expected, actual, epsilon) do {       \
  if (fabs((actual) - (expected)) > (epsilon)) {           \
    fprintf(stderr, "expected value %lf != actual value %lf\n", expected, actual); \
    assert(0);                                             \
  }                                                        \
} while (0)

static double eval(char *expr)
{
  Lexer lexer = lexer_init("test", sv_from_cstr(expr));
  MathParser parser = math_parser_init(lexer);
  double result;
  MathParserError err;
  while (parser.lexer.content.count > 0)
  {
    err = math_parser_rpn(&parser);
    assert(err == MERR_OK);
    err = math_parser_eval(&parser, &result);
    assert(err == MERR_OK || err == MERR_INPUT_EMPTY);
  }
  math_parser_free(&parser);
  return result;
}

static void evalErr(char *expr, MathParserError expected)
{
  Lexer lexer = lexer_init("test", sv_from_cstr(expr));
  MathParser parser = math_parser_init(lexer);
  double result;
  MathParserError err;
  while (lexer.content.count > 0)
  {
    err = math_parser_rpn(&parser);
    if (err == expected)
    {
      math_parser_free(&parser);
      return;
    }
    assert(err == MERR_OK);
    err = math_parser_eval(&parser, &result);
    if (err == expected)
    {
      math_parser_free(&parser);
      return;
    }
    assert(err == MERR_OK || err == MERR_INPUT_EMPTY);
  }
  math_parser_free(&parser);
  assert(0 && "Got OK, expected error");
}

void testSimpleEqs() {
  assertEquals(-1.0, eval("-1"), 0.001);
  assertEquals(1.0, eval("2-1"), 0.001);
  assertEquals(6.0, eval("5+1"), 0.001);
  assertEquals(12.0, eval("6*2"), 0.001);
  assertEquals(3.0 / 2, eval("3/2"), 0.001);
  // assertEquals(1.0, eval("5%2"), 0.001);
  // assertEquals(2.0, eval("5\\2"), 0.001);
  assertEquals(9.0, eval("3^2"), 0.001);
}

void testFloat() {
  assertEquals(1.0, eval("0.5*2"), 0.001);
  assertEquals(1.0, eval("2 .5"), 0.001);
  assertEquals(2.0, eval("1/2*4"), 0.001);
  assertEquals(1.125, eval("0.125+1"), 0.001);
}

void testOperatorPrecedence() {
  assertEquals(12.0, eval("2^1^3 6"), 0.001); // with implicit mult
  assertEquals(9.0, eval("2+3*4-5"), 0.001);
  // assertEquals(1.0, eval("5\\2/2"), 0.001);
}

void testBrackets() {
  assertEquals(14.0, eval("2*(3+4)"), 0.001);
  assertEquals(14.0, eval("2 (3+4)"), 0.001);
  assertEquals(2.0, eval("2*(-3+4)"), 0.001);
  assertEquals(314.5, eval("-1+0.5*((2+3)*4+5)^2+3"), 0.001);
}

void testWhitespace() {
  assertEquals(14.0, eval("  2  *   (  3 + 4 )    "), 0.001);
  assertEquals(-1.0, eval("    -  - -     1 "), 0.001);
}

void testMultiStatements() {
  assertEquals(5.0, eval("1+2;5"), 0.001);
  assertEquals(3.0, eval("1+2;"), 0.001);
  assertEquals(3.0, eval(" 1+2 ;   "), 0.001);
  assertEquals(3.0, eval(" 1+2 ;   ;"), 0.001);
  assertEquals(3.0, eval(" 1+2 ;   ;   "), 0.001);
}

void testDefVars() {
  assertEquals(M_PI, eval("pi"), 0.001);
  assertEquals(M_E, eval("E"), 0.001);
  evalErr("undefinedvar", MERR_UNRECOGNIZED_SYMBOL);
}

// void testUserVars() {
//   assertEquals(Math.E * 2, eval("two_e=2E"), 0.001);
//   assertEquals(Math.PI * 2, eval("two_pi  =   2 * PI  "), 0.001);
//   assertEquals(Math.PI * 2, eval("two_pi"), 0.001); // test persistence
//   assertEquals(4.0, eval("two=2; two 2"), 0.001);
//   assertEquals(null, eval("two2"));
//   assertEquals(0.0, eval("two.2=0"), 0.001);
//   assertEquals(2.0, eval("a  =  b   = 2"), 0.001);
//   assertNotEquals(null, math.getVar("a"));
//   assertNotEquals(null, math.getVar("b"));
//   assertEquals(null, eval("a = 2"));
// }
// 
// void testDefFunc() {
//   assertEquals(Math.sin(2), eval("sin2 = sin( t =  2 )"), 0.001);
//   assertEquals(2.0, eval("t"), 0.001);
//   assertEquals(Math.sin(2), eval("sin2"), 0.001);
//   assertEquals(Math.log(1000) / Math.log(10), eval("log(1000, 10)"), 0.001);
// }

void testSyntax() {
  assertEquals(2., eval("+2"), 0.001); // unary +
  evalErr("   ( 2 + 3  ", MERR_UNBALANCED_PARENTHESIS); // )
  evalErr("  e ( 1 , 2 )", MERR_UNRECOGNIZED_SYMBOL); // ,
  evalErr(" sin(pi, pi) ", MERR_UNRECOGNIZED_SYMBOL); // 1 arg (got 2)
  // assertEquals(null, eval(" log ( 2 )")); // 2 args (got 1)
  // assertEquals(null, eval("$"));
}

int main(int argc, char **argv)
{
  testSimpleEqs();
  testFloat();
  testOperatorPrecedence();
  testBrackets();
  testWhitespace();
  testMultiStatements();
  testDefVars();
  // testUserVars();
  // testDefFunc();
  testSyntax();
  printf("All tests passed\n");
  return 0;
}
