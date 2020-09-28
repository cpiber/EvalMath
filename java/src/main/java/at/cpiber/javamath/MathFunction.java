package at.cpiber.javamath;

import java.io.InvalidObjectException;
import java.util.Deque;

public interface MathFunction extends MathElement {
  MathSymbol get(final Deque<MathSymbol> stack) throws InvalidObjectException;

  int getArgCount();

  public static MathSymbol pop(final Deque<MathSymbol> stack) throws InvalidObjectException {
    final MathSymbol op = stack.pop();
    if (op.getClass() != MathSymbol.class)
      throw new InvalidObjectException("internal error - expected symbol (number)");
    return op;
  }
}
