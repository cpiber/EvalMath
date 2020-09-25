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

      System.out.print("Result: ");
      out(input);
    }
  }

  public static void out(String input) {
    Double res = math.eval(input);
    if (res == null) {
      System.out.print("Error: ");
      System.out.println(math.getError());
    }
    System.out.println(res);
  }
}
