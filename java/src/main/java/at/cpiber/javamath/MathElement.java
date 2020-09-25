package at.cpiber.javamath;

public interface MathElement {
  public enum Type {
    OP, SYM
  }

  public Type getType();
}
