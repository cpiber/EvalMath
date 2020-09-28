package at.cpiber.javamath;

import java.util.HashMap;
import java.util.Map;

public class JavaMath {
  private String lastError = null;

  private static final Map<String, MathSymbol> dvars = new HashMap<>();
  private final Map<String, MathSymbol> uvars = new HashMap<>(); // user vars
  static {
    final MathSymbol pi = new MathSymbol(Math.PI);
    dvars.put("pi", pi);
    dvars.put("Pi", pi);
    dvars.put("PI", pi);
    final MathSymbol e = new MathSymbol(Math.E);
    dvars.put("e", e);
    dvars.put("E", e);
  }

  private static final Map<String, MathJavaFunction<?>> dfunc = new HashMap<>();
  private final Map<String, MathFunction> ufunc = new HashMap<>();
  static {
    final MathJava1Function sin = new MathJava1Function(Math::sin, "sin");
    dfunc.put("sin", sin);
    dfunc.put("SIN", sin);
    final MathJava1Function cos = new MathJava1Function(Math::cos, "cos");
    dfunc.put("cos", cos);
    dfunc.put("COS", cos);
    final MathJava1Function tan = new MathJava1Function(Math::tan, "tan");
    dfunc.put("tan", tan);
    dfunc.put("TAN", tan);
    final MathJava1Function asin = new MathJava1Function(Math::asin, "asin");
    dfunc.put("asin", asin);
    dfunc.put("ASIN", asin);
    dfunc.put("arcsin", asin);
    dfunc.put("ARCSIN", asin);
    final MathJava1Function acos = new MathJava1Function(Math::acos, "acos");
    dfunc.put("acos", acos);
    dfunc.put("ACOS", acos);
    dfunc.put("arccos", acos);
    dfunc.put("ARCCOS", acos);
    final MathJava1Function atan = new MathJava1Function(Math::atan, "atan");
    dfunc.put("atan", atan);
    dfunc.put("ATAN", atan);
    dfunc.put("arctan", atan);
    dfunc.put("ARCTAN", atan);
    final MathJava1Function sqrt = new MathJava1Function(Math::sqrt, "sqrt");
    dfunc.put("sqrt", sqrt);
    dfunc.put("SQRT", sqrt);
    final MathJava1Function ln = new MathJava1Function(Math::log, "ln");
    dfunc.put("ln", ln);
    dfunc.put("LN", ln);
    final MathJava2Function log = new MathJava2Function((x, a) -> Math.log(x) / Math.log(a), "log");
    dfunc.put("log", log);
    dfunc.put("LOG", log);
  }

  /**
   * Evaluate infix string
   * 
   * @param input
   * @return (last) result
   */
  public Double eval(final String input) {
    Double ret = 0.0d;
    final String[] inputs = input.trim().split(";");

    for (final String i : inputs) {
      final MathParser parser = new MathParser(this);
      ret = parser.parse(i);
      if (ret == null && parser.getError() != null)
        lastError = parser.getError();
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
   * 
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

  /**
   * Get function
   * 
   * @param varname
   */
  public MathFunction getFunc(final String funcname) {
    if (dfunc.containsKey(funcname)) {
      return dfunc.get(funcname);
    } else if (ufunc.containsKey(funcname)) {
      return ufunc.get(funcname);
    }
    return null;
  }

  /**
   * Set function
   */
  public MathFunction setFunc(final String funcname, final MathFunction func) {
    ufunc.put(funcname, func);
    return func;
  }
}
