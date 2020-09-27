package at.cpiber.javamath;

import java.io.InvalidObjectException;
import java.util.Deque;
import java.util.function.UnaryOperator;

public class MathJava1Function extends MathJavaFunction<UnaryOperator<Double>> {
  MathJava1Function(final UnaryOperator<Double> op) {
    super(op);
  }

  MathJava1Function(final UnaryOperator<Double> op, final String sym) {
    super(op, sym);
  }

  /**
   * Get result
   */
  public MathSymbol get(final Deque<MathSymbol> stack) throws InvalidObjectException {
    final double op = MathFunction.pop(stack).get();
    return new MathSymbol(this.op.apply(op));
  }

  /**
   * How many arguments to expect
   */
  public int getArgCount() {
    return 1;
  }
}
