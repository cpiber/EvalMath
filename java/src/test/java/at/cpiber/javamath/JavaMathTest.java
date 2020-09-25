package at.cpiber.javamath;

import org.junit.Test;
import static org.junit.Assert.*;

public class JavaMathTest {
  private JavaMath math = new JavaMath();

  @Test public void testSimpleEqs() {
    assertEquals(-1.0d, math.eval("-1"), 0.001);
    assertEquals(1.0d, math.eval("2-1"), 0.001);
    assertEquals(6.0d, math.eval("5+1"), 0.001);
    assertEquals(12.0d, math.eval("6*2"), 0.001);
    assertEquals(3.0d/2, math.eval("3/2"), 0.001);
    assertEquals(1.0d, math.eval("5%2"), 0.001);
    assertEquals(2.0d, math.eval("5\\2"), 0.001);
    assertEquals(9.0d, math.eval("3^2"), 0.001);
  }

  @Test public void testFloat() {
    assertEquals(1.0d, math.eval("0.5*2"), 0.001);
    assertEquals(2.0d, math.eval("1/2*4"), 0.001);
    assertEquals(1.125d, math.eval("0.125+1"), 0.001);
  }

  @Test public void testOperatorPrecedence() {
    assertEquals(12.0d, math.eval("2^1^3 6"), 0.001); // with implicit mult
    assertEquals(9.0d, math.eval("2+3*4-5"), 0.001);
    assertEquals(1.0d, math.eval("5\\2/2"), 0.001);
  }

  @Test public void testBrackets() {
    assertEquals(14.0d, math.eval("2*(3+4)"), 0.001);
    assertEquals(314.5d, math.eval("-1+0.5*((2+3)*4+5)^2+3"), 0.001);
  }
}
