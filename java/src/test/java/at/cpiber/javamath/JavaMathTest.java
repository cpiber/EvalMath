package at.cpiber.javamath;

import org.junit.Test;
import static org.junit.Assert.*;

public class JavaMathTest {
  private JavaMath math = new JavaMath();

  @Test
  public void testSimpleEqs() {
    assertEquals(-1.0d, math.eval("-1"), 0.001);
    assertEquals(1.0d, math.eval("2-1"), 0.001);
    assertEquals(6.0d, math.eval("5+1"), 0.001);
    assertEquals(12.0d, math.eval("6*2"), 0.001);
    assertEquals(3.0d / 2, math.eval("3/2"), 0.001);
    assertEquals(1.0d, math.eval("5%2"), 0.001);
    assertEquals(2.0d, math.eval("5\\2"), 0.001);
    assertEquals(9.0d, math.eval("3^2"), 0.001);
  }

  @Test
  public void testFloat() {
    assertEquals(1.0d, math.eval("0.5*2"), 0.001);
    assertEquals(1.0d, math.eval("2 .5"), 0.001);
    assertEquals(2.0d, math.eval("1/2*4"), 0.001);
    assertEquals(1.125d, math.eval("0.125+1"), 0.001);
  }

  @Test
  public void testOperatorPrecedence() {
    assertEquals(12.0d, math.eval("2^1^3 6"), 0.001); // with implicit mult
    assertEquals(9.0d, math.eval("2+3*4-5"), 0.001);
    assertEquals(1.0d, math.eval("5\\2/2"), 0.001);
  }

  @Test
  public void testBrackets() {
    assertEquals(14.0d, math.eval("2*(3+4)"), 0.001);
    assertEquals(14.0d, math.eval("2 (3+4)"), 0.001);
    assertEquals(2.0d, math.eval("2*(-3+4)"), 0.001);
    assertEquals(314.5d, math.eval("-1+0.5*((2+3)*4+5)^2+3"), 0.001);
  }

  @Test
  public void testWhitespace() {
    assertEquals(14.0d, math.eval("  2  *   (  3 + 4 )    "), 0.001);
    assertEquals(-1.0d, math.eval("    -  - -     1 "), 0.001);
  }

  @Test
  public void testMultiStatements() {
    assertEquals(5.0d, math.eval("1+2;5"), 0.001);
    assertEquals(3.0d, math.eval("1+2;"), 0.001);
    assertEquals(3.0d, math.eval(" 1+2 ;   "), 0.001);
    assertEquals(0.0d, math.eval(" 1+2 ;   ;"), 0.001);
    assertEquals(0.0d, math.eval(" 1+2 ;   ;   "), 0.001);
  }

  @Test
  public void testDefVars() {
    assertEquals(Math.PI, math.eval("pi"), 0.001);
    assertEquals(Math.E, math.eval("E"), 0.001);
    assertEquals(null, math.eval("undefinedvar"));
  }

  @Test
  public void testUserVars() {
    assertEquals(Math.E * 2, math.eval("two_e=2E"), 0.001);
    assertEquals(Math.PI * 2, math.eval("two_pi  =   2 * PI  "), 0.001);
    assertEquals(Math.PI * 2, math.eval("two_pi"), 0.001); // test persistence
    assertEquals(4.0d, math.eval("two=2; two 2"), 0.001);
    assertEquals(null, math.eval("two2"));
    assertEquals(0.0d, math.eval("two.2=0"), 0.001);
    assertEquals(2.0d, math.eval("a  =  b   = 2"), 0.001);
    assertNotEquals(null, math.getVar("a"));
    assertNotEquals(null, math.getVar("b"));
    assertEquals(null, math.eval("a = 2"));
  }

  @Test
  public void testDefFunc() {
    assertEquals(Math.sin(2), math.eval("sin2 = sin( t =  2 )"), 0.001);
    assertEquals(2.0d, math.eval("t"), 0.001);
    assertEquals(Math.sin(2), math.eval("sin2"), 0.001);
    assertEquals(Math.log(1000) / Math.log(10), math.eval("log(1000, 10)"), 0.001);
  }

  @Test
  public void testSyntax() {
    assertEquals(null, math.eval("+2")); // unary +
    assertEquals(null, math.eval("   ( 2 + 3  ")); // )
    assertEquals(null, math.eval("  e ( 1 , 2 )")); // ,
    assertEquals(null, math.eval(" sin(pi, pi) ")); // 1 arg (got 2)
    assertEquals(null, math.eval(" log ( 2 )")); // 2 args (got 1)
    assertEquals(null, math.eval("$"));
  }
}
