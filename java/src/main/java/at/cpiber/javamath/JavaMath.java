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
  
  public int eval(final String input) {
    final List<MathElement> math = pfx(input);
    System.out.println(math);
    return 0;
  }

  /**
   * Parse string to postfix list
   * @param input infix string
   * @return postfix list
   */
  private List<MathElement> pfx(String input) {
    output.clear();
    ostack.clear();

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
      } catch (InvalidObjectException exception) {
        return null;
      }

      last = chr;
    }

    // finish last number
    addNum(input);

    // check valid operation
    if (!isNumber(last) && !expectOp) // operator at end
      return null;

    // pop remaining operators
    while (!ostack.isEmpty()) {
      MathOperator op = ostack.pop();
      if (op == MathOperator.LBRACE) return null;
      output.add(op);
    }

    return output;
  }

  /**
   * Check if character is a number
   */
  private boolean isNumber(final char chr) {
    return chr >= '0' && chr <= '9';
  }

  /**
   * Parse previous character(s) as number and add to output
   */
  private void addNum(String str) {
    if (start < index) { // actually started new number?
      output.add(new MathSymbol(str.substring(start, index)));
      start = index + 1;
    } else {
      ++start;
    }
  }

  /**
   * Check character in input string and parse
   * Updates expectOp
   */
  private boolean handleChar(String input, char chr, char last, boolean expectOp) throws InvalidObjectException {
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
   * Operator handling wrapper
   * Updates expectOp
   */
  private boolean handleOp(String input, char chr, char last, boolean expectOp) throws InvalidObjectException {
    MathOperator op = MathOperator.get(chr);
    addNum(input); // finish number / advance counter

    // just finished a number, operator expected next (can also be set after ' ')
    if (isNumber(last))
      expectOp = true;

    return handleOp(op, expectOp);
  }

  /**
   * Handle operator (precedence)
   * Updates expectOp
   */
  private boolean handleOp(MathOperator op, boolean expectOp) throws InvalidObjectException {
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
      } catch (NoSuchElementException exception) {
        throw new InvalidObjectException("mismatched parentheses");
      }
    }

    return expectOp;
  }
}
