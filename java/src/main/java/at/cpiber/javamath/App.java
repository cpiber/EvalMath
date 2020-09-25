package at.cpiber.javamath;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;

public class App {
  private static JavaMath math = new JavaMath();

  private App() {
  }

  public static void main(String[] args) throws IOException {
    String input = String.join(" ", args);

    // parse arg
    if (input != null && !input.equals("")) {
      out(input);
      return;
    }

    // else enter interactive mode
    BufferedReader in = new BufferedReader(new InputStreamReader(System.in));

    while (true) {
      // get equation from stdin
      System.out.print("Enter equation: ");
      input = in.readLine().trim();
      if (input.equals(""))
        break;

      out(input, true);
    }
  }

  /**
   * Evalute and print result
   * @param input infix string
   * @param print whether to print errors + "Result: "
   */
  public static void out(String input, boolean print) {
    Double res = math.eval(input);
    if (res == null && print) {
      System.out.print("Error: ");
      System.out.println(math.getError());
    }
    if (print) 
      System.out.print("Result: ");
    System.out.println(res);
  }

  public static void out(String input) {
    out(input, false);
  }
}
