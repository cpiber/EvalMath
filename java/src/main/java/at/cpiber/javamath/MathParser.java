package at.cpiber.javamath;

import java.io.InvalidObjectException;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.function.Predicate;

public class MathParser {
  private JavaMath math;

  private final Deque<MathOperator> ostack = new ArrayDeque<>();
  private int index;
  private boolean expectOp;
  private boolean allowAssign;

  private String lastError = null;

  MathParser(final JavaMath math) {
    this.math = math;
  }

  /**
   * Parse and evaluate string
   */
  public Double parse(final String input, final int start) {
    List<MathElement> output;
    if ((output = pfx(input, start)) == null)
      return null;
    return exec(output);
  }

  public Double parse(final String input) {
    return parse(input, 0);
  }

  /**
   * Error message
   */
  public String getError() {
    return lastError;
  }

  /**
   * Parse infix string to postfix list
   * 
   * @param input infix string
   * @return postfix list
   */
  private List<MathElement> pfx(String input, int start) {
    final List<MathElement> output = new ArrayList<>();
    ostack.clear();
    lastError = null;

    input = input.trim();
    final int len = input.length();
    expectOp = false;
    allowAssign = true;
    char chr;

    // empty string evaluates to 0
    if (len == 0) {
      output.add(new MathSymbol(0.0d));
      return output;
    } else if (start >= len) {
      lastError = "internal error - input too short";
      return null;
    }

    for (index = start; index < len; index++) {
      chr = input.charAt(index);

      // handle character
      // parses numbers, resolves operator precedence
      try {
        handleChar(input, output, chr, len);
      } catch (final InvalidObjectException exception) {
        lastError = exception.getMessage();
        return null;
      }
    }

    // check valid operation
    if (!expectOp) { // operator at end
      lastError = "cannot end in operator";
      return null;
    }

    // pop remaining operators
    while (!ostack.isEmpty()) {
      final MathOperator op = ostack.pop();
      if (op == MathOperator.LBRACE) {
        lastError = "mismatched parenthesis";
        return null;
      }
      output.add(op);
    }

    return output;
  }

  /**
   * Evaluate postfix list
   */
  private Double exec(final List<MathElement> list) {
    final Deque<MathSymbol> stack = new ArrayDeque<>();

    try {
      for (final MathElement e : list) {
        if (e.getClass() == MathOperator.class) {
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
   * Check character in input string and parse
   * 
   * Updates expectOp
   */
  private void handleChar(final String input, final List<MathElement> output, final char chr, final int len)
      throws InvalidObjectException {
    // check current symbol
    if (isNumber(chr)) {
      if (expectOp)
        handleOp(MathOperator.MULT, output); // implicit multiplication

      output.add(new MathSymbol(consume(input, len, this::isNumber)));
      expectOp = true;

    } else if (isAlpha(chr)) {
      if (expectOp)
        handleOp(MathOperator.MULT, output); // implicit multiplication

      String varname = consume(input, len, this::isVar);
      MathSymbol var;

      if ((var = math.getVar(varname)) != null) {
        output.add(var);
        expectOp = true;

      } else if (allowAssign) {
        output.add(handleAssign(input, len, varname));
        index = len;
        expectOp = true;

      } else {
        throw new InvalidObjectException(String.format("unknown variable '%s'", varname));
      }

    } else if (MathOperator.is(chr)) {
      handleOp(chr, output);

    } else if (chr != ' ') { // ignore whitespace, rest is invalid
      throw new InvalidObjectException(String.format("invalid character '%c'", chr));
    }
    allowAssign = allowAssign && chr == ' '; // false if anything other than ' ' came before
  }

  /**
   * Handle operator (precedence)
   * 
   * Updates expectOp
   */
  private void handleOp(final MathOperator op, final List<MathElement> output) throws InvalidObjectException {
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
        throw new InvalidObjectException("mismatched parenthesis");
      }
    }
  }

  private void handleOp(final char chr, final List<MathElement> output) throws InvalidObjectException {
    handleOp(MathOperator.get(chr), output);
  }

  /**
   * Handle variable assignment
   * 
   * Updates expectOp
   */
  private MathSymbol handleAssign(final String input, final int len, final String varname)
      throws InvalidObjectException {
    consume(input, len, chr -> chr == ' ', false); // remove whitespace
    if (++index >= len || input.charAt(index) != '=')
      throw new InvalidObjectException(String.format("unknown variable '%s', expected assignment (=)", varname));
    return math.setVar(varname, new MathSymbol(new MathParser(math).parse(input, index + 1)));
  }

  /**
   * Check if character is a number
   */
  private boolean isNumber(final char chr) {
    return chr >= '0' && chr <= '9' || chr == '.';
  }

  /**
   * Check if character is a letter
   */
  private boolean isAlpha(final char chr) {
    return (chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z');
  }

  /**
   * Check if character is a variable/function name
   */
  private boolean isVar(final char chr) {
    return isNumber(chr) || isAlpha(chr) || chr == '_'; // Note: Allows . in var names
  }

  /**
   * Consume characters that fit predicate and return consumed characters
   */
  private String consume(final String input, final int len, final Predicate<Character> check, final boolean substring) {
    final int start = index;
    while (++index < len && check.test(input.charAt(index)))
      ; // go to end of chunk
    index--;
    if (!substring)
      return null;
    return input.substring(start, index + 1);
  }

  private String consume(final String input, final int len, final Predicate<Character> check) {
    return consume(input, len, check, true);
  }
}
