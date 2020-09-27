package at.cpiber.javamath;

public class MathSymbol implements MathElement {
  private final double sym;

  MathSymbol(final double sym) {
    this.sym = sym;
  }

  MathSymbol(final String sym) {
    this.sym = Double.parseDouble(sym);
  }

  public double get() {
    return sym;
  }

  public String toString() {
    return Double.toString(sym);
  }
}
