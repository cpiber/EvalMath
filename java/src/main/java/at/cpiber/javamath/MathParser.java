package at.cpiber.javamath;

import java.io.InvalidObjectException;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.function.Predicate;

public class MathParser {
  private final JavaMath math;

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
  public Double parse(final String input, final int start, final String until) {
    List<MathElement> output;
    if ((output = pfx(input, start, until)) == null)
      return null;
    return exec(output);
  }

  public Double parse(final String input, final int start) {
    return parse(input, start, "");
  }

  public Double parse(final String input) {
    return parse(input, 0, "");
  }

  /**
   * Error message
   */
  public String getError() {
    return lastError;
  }

  /**
   * Index
   */
  public int getIndex() {
    return index;
  }

  /**
   * Parse infix string to postfix list
   * 
   * @param input infix string
   * @return postfix list
   */
  public List<MathElement> pfx(String input, final int start, final String until) {
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
        if (until.indexOf(chr) != -1 || handleChar(input, output, chr, len, until))
          break;
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
  public Double exec(final List<MathElement> list) {
    final Deque<MathSymbol> stack = new ArrayDeque<>();

    try {
      for (final MathElement e : list) {
        if (e.getClass() == MathVar.class) {
          stack.push(((MathVar) e).get());
        } else if (e.getClass() != MathSymbol.class) {
          stack.push(((MathFunction) e).get(stack));
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
  private boolean handleChar(final String input, final List<MathElement> output, final char chr, final int len,
      final String until) throws InvalidObjectException {
    // check current symbol
    if (isNumber(chr)) {
      if (expectOp)
        handleOp(MathOperator.MULT, output); // implicit multiplication

      String num = null;
      try {
        num = consume(input, len, this::isNumber);
        output.add(new MathSymbol(num));
      } catch (final NumberFormatException exception) {
        throw new InvalidObjectException(String.format("Invalid number '%s'", num));
      }
      expectOp = true;

    } else if (isAlpha(chr)) {
      if (expectOp)
        handleOp(MathOperator.MULT, output); // implicit multiplication

      if (handleVar(input, output, len, until))
        return true;

    } else if (MathOperator.is(chr)) {
      handleOp(chr, output);

    } else if (chr == '=') {
      throw new InvalidObjectException("unexpected assignment");

    } else if (chr != ' ') { // ignore whitespace, rest is invalid
      throw new InvalidObjectException(String.format("invalid character '%c'", chr));
    }
    allowAssign = allowAssign && chr == ' '; // false if anything other than ' ' came before
    return false;
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
   * Handle call & assignment of variables + functions
   * 
   * Updates expectOp
   * 
   * @return true if break
   */
  private boolean handleVar(final String input, final List<MathElement> output, final int len, final String until)
      throws InvalidObjectException {
    final String varname = consume(input, len, this::isVar);
    MathSymbol var;
    MathFunction func;

    if ((var = math.getVar(varname)) != null) {
      output.add(var);
      expectOp = true;

    } else if ((func = math.getFunc(varname)) != null) {
      handleArgs(input, output, len, func);
      output.add(func);
      expectOp = true;

    } else if (allowAssign) {
      output.add(handleAssign(input, len, varname, until));
      expectOp = true;
      return true;

    } else {
      throw new InvalidObjectException(String.format("unknown variable/function '%s'", varname));
    }
    return false;
  }

  /**
   * Handle variable assignment
   * 
   * Updates expectOp
   */
  private MathVar handleAssign(final String input, final int len, final String varname, final String until)
      throws InvalidObjectException {
    consume(input, len, chr -> chr == ' ', false); // remove whitespace
    if (++index >= len || input.charAt(index) != '=')
      throw new InvalidObjectException(String.format("unknown variable '%s', expected assignment (=)", varname));

    final MathParser parser = new MathParser(math);
    final List<MathElement> output = parser.pfx(input, index + 1, until);
    if (output == null)
      throw new InvalidObjectException(parser.getError());
    index = parser.getIndex();

    return new MathVar(parser, varname, output);
  }

  /**
   * Handle arguments for function call
   */
  private void handleArgs(final String input, final List<MathElement> output, final int len, final MathFunction op)
      throws InvalidObjectException {
    final int c = op.getArgCount();

    consume(input, len, chr -> chr == ' ', false); // remove whitespace
    if (++index >= len || input.charAt(index) != '(')
      throw new InvalidObjectException("expected '(' for function call");

    for (int i = 1; i <= c; i++) {
      final MathParser parser = new MathParser(math);
      final List<MathElement> arg = parser.pfx(input, ++index, ",)");

      if (arg == null)
        throw new InvalidObjectException(parser.getError());

      final char expected = i != c ? ',' : ')';
      if ((index = parser.getIndex()) >= len || input.charAt(index) != expected)
        throw new InvalidObjectException(String.format("expected '%s'", expected));

      output.addAll(arg);
    }
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

  /**
   * Delegate setting variables
   */
  public MathSymbol setVar(final String varname, final MathSymbol var) {
    return math.setVar(varname, var);
  }
}
