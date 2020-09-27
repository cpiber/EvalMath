package at.cpiber.javamath;

import java.util.HashMap;
import java.util.Map;

public class JavaMath {
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
    Double ret = 0.0d;
    String[] inputs = input.trim().split(";");

    for (String i : inputs) {
      MathParser parser = new MathParser(this);
      ret = parser.parse(i);
      if (ret == null && parser.getError() != null) lastError = parser.getError();
    }

    return ret;
  }

  /**
   * Error message
   */
  public String getError() {
    return lastError;
  }
  
  /**
   * Get variable content
   * @param varname
   */
  public MathSymbol getVar(final String varname) {
    if (dvars.containsKey(varname)) {
      return dvars.get(varname);
    } else if (uvars.containsKey(varname)) {
      return uvars.get(varname);
    }
    return null;
  }

  /**
   * Set variable content
   */
  public MathSymbol setVar(final String varname, final MathSymbol var) {
    uvars.put(varname, var);
    return var;
  }
}
