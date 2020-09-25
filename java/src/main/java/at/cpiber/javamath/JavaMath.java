package at.cpiber.javamath;

import java.io.InvalidObjectException;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;
import java.util.NoSuchElementException;

public class JavaMath {
  private final List<MathElement> output = new ArrayList<>();
  private final Deque<MathOperator> ostack = new ArrayDeque<>();

  private int start;
  private int index;

  private String lastError = null;

  public Double eval(final String input) {
    if (!pfx(input)) return null;
    return exec();
  }

  public String getError() {
    return lastError;
  }

  /**
   * Parse infix string to postfix list
   * 
   * @param input infix string
   * @return postfix list
   */
  private boolean pfx(String input) {
    output.clear();
    ostack.clear();
    lastError = null;

    input = input.trim();
    final int len = input.length();
    start = 0;
    boolean expectOp = false;
    char chr;
    char last = ' ';

    for (index = 0; index < len; index++) {
      chr = input.charAt(index);

      // handle character
      // parses numbers, resolves operator precedence
      try {
        expectOp = handleChar(input, chr, last, expectOp);
      } catch (final InvalidObjectException exception) {
        lastError = exception.getMessage();
        return false;
      }

      last = chr;
    }

    // finish last number
    addNum(input);

    // check valid operation
    if (!isNumber(last) && !expectOp) { // operator at end
      lastError = "cannot end in operator";
      return false;
    }

    // pop remaining operators
    while (!ostack.isEmpty()) {
      final MathOperator op = ostack.pop();
      if (op == MathOperator.LBRACE) {
        lastError = "mismatched parenthesis";
        return false;
      }
      output.add(op);
    }

    return true;
  }

  /**
   * Evaluate postfix list
   */
  private Double exec() {
    final Deque<MathSymbol> stack = new ArrayDeque<>();

    try {
      for (final MathElement e : output) {
        if (e.getType() == MathElement.Type.OP) {
          stack.push(((MathOperator) e).exec(stack));
        } else {
          stack.push((MathSymbol) e);
        }
      }
    } catch (final Exception exception) {
      lastError = exception.getMessage();
      return null;
    }

    if (stack.size() != 1) {
      lastError = "internal error - stack invalid";
      return null;
    }

    return stack.pop().get();
  }

  /**
   * Check if character is a number
   */
  private boolean isNumber(final char chr) {
    return chr >= '0' && chr <= '9' || chr == '.';
  }

  /**
   * Parse previous character(s) as number and add to output
   */
  private void addNum(final String str) {
    if (start < index) { // actually started new number?
      output.add(new MathSymbol(str.substring(start, index)));
      start = index + 1;
    } else {
      ++start;
    }
  }

  /**
   * Check character in input string and parse Updates expectOp
   */
  private boolean handleChar(final String input, final char chr, final char last, boolean expectOp)
      throws InvalidObjectException {
    // check current symbol
    if (isNumber(chr)) {
      if (expectOp)
        expectOp = handleOp(MathOperator.MULT, expectOp); // implicit multiplication

    } else if (chr == ' ') {
      addNum(input); // finish number / advance counter
      // just finished a number, operator expected next
      if (isNumber(last))
        expectOp = true;

    } else if (MathOperator.is(chr)) {
      expectOp = handleOp(input, chr, last, expectOp);

    } else {
      throw new InvalidObjectException("invalid character");
    }

    return expectOp;
  }

  /**
   * Operator handling wrapper Updates expectOp
   */
  private boolean handleOp(final String input, final char chr, final char last, boolean expectOp)
      throws InvalidObjectException {
    final MathOperator op = MathOperator.get(chr);
    addNum(input); // finish number / advance counter

    // just finished a number, operator expected next (can also be set after ' ')
    if (isNumber(last))
      expectOp = true;

    return handleOp(op, expectOp);
  }

  /**
   * Handle operator (precedence) Updates expectOp
   */
  private boolean handleOp(final MathOperator op, boolean expectOp) throws InvalidObjectException {
    // no two operators allowed except negation
    if (!expectOp) {
      // convert to unary negation
      // e.x. (-digit)
      if (op == MathOperator.MINUS) {
        ostack.push(MathOperator.NEG);
        return false;
      } else if (op != MathOperator.LBRACE) {
        throw new InvalidObjectException("unexpected operator");
      }

    } else if (op == MathOperator.LBRACE) {
      ostack.push(MathOperator.MULT); // implicit multiplication
    }

    // pop operators with same / less precedence (handles left-association)
    while (!ostack.isEmpty() && ostack.getFirst() != MathOperator.LBRACE
        && (op == MathOperator.RBRACE || op.lt(ostack.getFirst())))
      output.add(ostack.pop());

    expectOp = op == MathOperator.RBRACE; // need operator after ), nowhere else

    if (!expectOp) {
      ostack.push(op);
    } else {
      try {
        ostack.pop(); // pop (
      } catch (final NoSuchElementException exception) {
        throw new InvalidObjectException("mismatched parentheses");
      }
    }

    return expectOp;
  }
}
