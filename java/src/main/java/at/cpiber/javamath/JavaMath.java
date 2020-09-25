package at.cpiber.javamath;

import java.io.InvalidObjectException;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.function.Predicate;

public class JavaMath {
  private final List<MathElement> output = new ArrayList<>();
  private final Deque<MathOperator> ostack = new ArrayDeque<>();

  private int index;
  private boolean expectOp;

  private String lastError = null;

  public Double eval(final String input) {
    if (!pfx(input))
      return null;
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
    expectOp = false;
    char chr;

    for (index = 0; index < len; index++) {
      chr = input.charAt(index);

      // handle character
      // parses numbers, resolves operator precedence
      try {
        expectOp = handleChar(input, chr, len);
      } catch (final InvalidObjectException exception) {
        lastError = exception.getMessage();
        return false;
      }
    }

    // check valid operation
    if (!expectOp) { // operator at end
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

    System.out.println(output);

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
   * Check character in input string and parse Updates expectOp
   */
  private boolean handleChar(final String input, final char chr, final int len) throws InvalidObjectException {
    // check current symbol
    if (isNumber(chr)) {
      if (expectOp)
        handleOp(MathOperator.MULT); // implicit multiplication

      output.add(new MathSymbol(consume(input, len, this::isNumber)));
      expectOp = true;

    } else if (MathOperator.is(chr)) {
      handleOp(chr);

    } else if (chr != ' ') { // ignore whitespace, rest is invalid
      throw new InvalidObjectException("invalid character");
    }

    return expectOp;
  }

  /**
   * Consume characters that fit predicate and return consumed characters
   */
  private String consume(String input, int len, Predicate<Character> check) {
    int start = index;
    while (++index < len && check.test(input.charAt(index)))
      ; // go to end of number
    index--;
    return input.substring(start, index + 1);
  }

  /**
   * Operator handling wrapper Updates expectOp
   */
  private void handleOp(final char chr) throws InvalidObjectException {
    handleOp(MathOperator.get(chr));
  }

  /**
   * Handle operator (precedence) Updates expectOp
   */
  private void handleOp(final MathOperator op) throws InvalidObjectException {
    // no two operators allowed except negation
    if (!expectOp) {
      // convert to unary negation
      // e.x. (-digit)
      if (op == MathOperator.MINUS) {
        ostack.push(MathOperator.NEG);
        return;
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
  }
}
