package at.cpiber.javamath;

import java.io.InvalidObjectException;
import java.util.List;

public class MathVar implements MathElement {
  private final MathParser parser;
  private final String varname;
  private final List<MathElement> output;

  MathVar(final MathParser parser, final String varname, final List<MathElement> output) {
    this.parser = parser;
    this.varname = varname;
    this.output = output;
  }

  public MathSymbol get() throws InvalidObjectException {
    final Double d = parser.exec(output);
    if (d == null)
      throw new InvalidObjectException(parser.getError());
    return parser.setVar(varname, new MathSymbol(d));
  }
}
