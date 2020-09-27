package at.cpiber.javamath;

import java.io.InvalidObjectException;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.function.Predicate;

public class JavaMath {
  private final Deque<MathOperator> ostack = new ArrayDeque<>();
  private int index;
  private boolean expectOp;

  private String lastError = null;

  private static final Map<String, MathSymbol> dvars = new HashMap<>();
  private final Map<String, MathSymbol> uvars = new HashMap<>(); // user vars
  static {
    MathSymbol pi = new MathSymbol(Math.PI);
    dvars.put("pi", pi);
    dvars.put("Pi", pi);
    dvars.put("PI", pi);
    MathSymbol e = new MathSymbol(Math.E);
    dvars.put("e", e);
    dvars.put("E", e);
  }

  /**
   * Evaluate infix string
   * 
   * @param input
   * @return (last) result
   */
  public Double eval(final String input) {
    List<MathElement> output;
    Double ret = 0.0d;
    String[] inputs = input.split(";");

    for (String i : inputs) {
      if ((output = pfx(i)) == null) {
        ret = null;
        continue;
      }
      ret = exec(output);
    }

    return ret;
  }

  /**
   * Error message
   * 
   * @return
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
  private List<MathElement> pfx(String input) {
    final List<MathElement> output = new ArrayList<>();
    ostack.clear();
    lastError = null;

    input = input.trim();
    final int len = input.length();
    expectOp = false;
    char chr;

    // empty string evaluates to 0
    if (len == 0) {
      output.add(new MathSymbol(0));
      return output;
    }

    for (index = 0; index < len; index++) {
      chr = input.charAt(index);

      // handle character
      // parses numbers, resolves operator precedence
      try {
        expectOp = handleChar(input, output, chr, len);
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
   * Check character in input string and parse Updates expectOp
   */
  private boolean handleChar(final String input, final List<MathElement> output, final char chr, final int len)
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
      if (dvars.containsKey(varname)) {
        var = dvars.get(varname);
      } else if (uvars.containsKey(varname)) {
        var = uvars.get(varname);
      } else {
        throw new InvalidObjectException(String.format("unknown variable '%s'", varname));
      }

      output.add(var);
      expectOp = true;
    
    } else if (MathOperator.is(chr)) {
      handleOp(chr, output);

    } else if (chr != ' ') { // ignore whitespace, rest is invalid
      throw new InvalidObjectException(String.format("invalid character '%c'", chr));
    }

    return expectOp;
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
  private String consume(final String input, final int len, final Predicate<Character> check) {
    final int start = index;
    while (++index < len && check.test(input.charAt(index))); // go to end of chunk
    return input.substring(start, index--);
  }

  /**
   * Operator handling wrapper Updates expectOp
   */
  private void handleOp(final char chr, final List<MathElement> output) throws InvalidObjectException {
    handleOp(MathOperator.get(chr), output);
  }

  /**
   * Handle operator (precedence) Updates expectOp
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
        throw new InvalidObjectException("mismatched parentheses");
      }
    }
  }
}
