package at.cpiber.javamath;

public abstract class MathJavaFunction<F> implements MathFunction {
  protected final F op;
  private final String sym;

  MathJavaFunction(final F op) {
    this.op = op;
    this.sym = op.toString();
  }

  MathJavaFunction(final F op, final String sym) {
    this.op = op;
    this.sym = sym;
  }

  public String toString() {
    return sym;
  }
}
