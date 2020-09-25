package at.cpiber.javamath;

import java.util.HashMap;
import java.util.Map;

public enum MathOperator implements MathElement {
  NEG     ("_"),
  PLUS    ("+"),
  MINUS   ("-"),
  MULT    (1, "*"),
  DIV     (1, "/"),
  MOD     (1, "%"),
  IDIV    (1, "\\"),
  EXP     (2, "^", false),
  LBRACE  (3, "("),
  RBRACE  (3, ")");

  private static final Map<Character, MathOperator> ops = new HashMap<>();
  static {
    ops.put('+', PLUS);
    ops.put('-', MINUS);
    ops.put('*', MULT);
    ops.put('/', DIV);
    ops.put('%', MOD);
    ops.put('\\', IDIV);
    ops.put('^', EXP);
    ops.put('(', LBRACE);
    ops.put('[', LBRACE);
    ops.put('{', LBRACE);
    ops.put(')', RBRACE);
    ops.put(']', RBRACE);
    ops.put('}', RBRACE);
  }

  private final int precedence; // operator precedence
  private final boolean lassociative;
  private final String sym; // operator symbol

  MathOperator(String sym) {
    this.precedence = 0;
    this.sym = sym;
    this.lassociative = true;
  }

  MathOperator(int precedence, String sym) {
    this.precedence = precedence;
    this.sym = sym;
    this.lassociative = true;
  }

  MathOperator(int precedence, String sym, boolean lassociative) {
    this.precedence = precedence;
    this.sym = sym;
    this.lassociative = lassociative;
  }

  public boolean lt(MathOperator other) {
    return this.precedence < other.precedence ||
      (this.precedence == other.precedence && this.lassociative);
  }

  public static MathOperator get(char chr) {
    return ops.get(chr);
  }

  public static boolean is(char chr) {
    return ops.containsKey(chr);
  }

  public String toString() {
    return this.sym;
  }

  public Type getType() {
    return Type.OP;
  }
}