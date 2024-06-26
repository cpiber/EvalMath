package at.cpiber.javamath;

import java.io.InvalidObjectException;
import java.util.Deque;
import java.util.HashMap;
import java.util.Map;

public enum MathOperator implements MathFunction {
  NEG("_"), PLUS("+"), MINUS("-"), MULT(1, "*"), DIV(1, "/"), MOD(1, "%"), IDIV(1, "\\"), EXP(2, "^", false),
  LBRACE(3, "("), RBRACE(3, ")");

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

  MathOperator(final String sym) {
    this.precedence = 0;
    this.sym = sym;
    this.lassociative = true;
  }

  MathOperator(final int precedence, final String sym) {
    this.precedence = precedence;
    this.sym = sym;
    this.lassociative = true;
  }

  MathOperator(final int precedence, final String sym, final boolean lassociative) {
    this.precedence = precedence;
    this.sym = sym;
    this.lassociative = lassociative;
  }

  /**
   * Less precedence? Considers left/right association
   */
  public boolean lt(final MathOperator other) {
    return this.precedence < other.precedence || (this.precedence == other.precedence && this.lassociative);
  }

  /**
   * Get result
   */
  public MathSymbol get(final Deque<MathSymbol> stack) throws InvalidObjectException {
    // unary
    if (this == NEG)
      return new MathSymbol(-1 * MathFunction.pop(stack).get());

    // binary
    final double op2 = MathFunction.pop(stack).get();
    final double op1 = MathFunction.pop(stack).get();
    if (this == MINUS) {
      return new MathSymbol(op1 - op2);
    } else if (this == PLUS) {
      return new MathSymbol(op1 + op2);
    } else if (this == MULT) {
      return new MathSymbol(op1 * op2);
    } else if (this == DIV) {
      return new MathSymbol(op1 / op2);
    } else if (this == MOD) {
      return new MathSymbol(op1 % op2);
    } else if (this == IDIV) {
      return new MathSymbol((int) op1 / (int) op2);
    } else if (this == EXP) {
      return new MathSymbol(Math.pow(op1, op2));
    } else {
      throw new InvalidObjectException("internal error - evaluating braces not allowed");
    }
  }

  /**
   * Convert char to enum
   */
  public static MathOperator get(final char chr) {
    return ops.get(chr);
  }

  /**
   * Stub
   */
  public int getArgCount() {
    return this == NEG ? 1 : 2;
  }

  /**
   * Check if char is enum
   */
  public static boolean is(final char chr) {
    return ops.containsKey(chr);
  }

  public String toString() {
    return this.sym;
  }
}