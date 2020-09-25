package at.cpiber.javamath;

public class MathSymbol implements MathElement {
  private final int sym;

  MathSymbol(int sym) {
    this.sym = sym;
  }
  MathSymbol(String sym) {
    this.sym = Integer.parseInt(sym);
  }

  public String toString() {
    return Integer.toString(sym);
  }

  public Type getType() {
    return Type.SYM;
  }
}
