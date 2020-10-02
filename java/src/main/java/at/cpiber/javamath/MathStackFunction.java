package at.cpiber.javamath;

import java.io.InvalidObjectException;
import java.util.Deque;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class MathStackFunction implements MathFunction {
  private final MathParser math;

  private final List<String> args;
  private final Map<String, Var> vars = new HashMap<>();
  private List<MathElement> pfx = null;

  private final String sym;

  MathStackFunction(final MathParser math, final List<String> args, final String sym) {
    this.math = math;
    this.args = args;
    this.sym = sym;

    for (String arg : args) {
      vars.put(arg, new Var(arg));
    }
  }

  public void setPfx(final List<MathElement> output) {
    this.pfx = output;
  }

  public MathSymbol get(Deque<MathSymbol> stack) throws InvalidObjectException {
    for (int i = args.size() - 1; i >= 0; i--) {
      vars.get(args.get(i)).set(MathFunction.pop(stack));
    }
    return new MathSymbol(this.math.exec(this.pfx));
  }

  public int getArgCount() {
    return args.size();
  }
  
  public String toString() {
    return sym;
  }

  public boolean hasVar(final String varname) {
    return args.contains(varname);
  }

  public Var getVar(final String varname) {
    return vars.get(varname);
  }


  public class Var extends MathSymbol {
    private MathSymbol val = null;
    private String sym;

    Var(final String sym) {
      super(0);
      this.sym = sym;
    }

    public MathSymbol set(final MathSymbol val) {
      this.val = val;
      return val;
    }

    @Override
    public double get() {
      return this.val.get();
    }

    @Override
    public String toString() {
      return this.sym;
    }
  }
}
