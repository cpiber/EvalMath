package at.cpiber.javamath;

import java.io.InvalidObjectException;
import java.util.Deque;
import java.util.function.BinaryOperator;

public class MathJava2Function extends MathJavaFunction<BinaryOperator<Double>> {
  MathJava2Function(final BinaryOperator<Double> op) {
    super(op);
  }

  MathJava2Function(final BinaryOperator<Double> op, final String sym) {
    super(op, sym);
  }

  /**
   * Get result
   */
  public MathSymbol get(final Deque<MathSymbol> stack) throws InvalidObjectException {
    final double op2 = MathFunction.pop(stack).get();
    final double op1 = MathFunction.pop(stack).get();
    return new MathSymbol(this.op.apply(op1, op2));
  }

  /**
   * How many arguments to expect
   */
  public int getArgCount() {
    return 2;
  }
}
