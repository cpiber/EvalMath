# EvalMath

Math string evaluation.

Repo includes implementations in various languages (java and counting).

Created for learning purposes.



## Usage

Pass a mathematical expression as argument(s). All arguments are concatenated into one string.

If no arguments are passed, interactive mode is started. Enter your equations when prompted. Exit by passing an empty string or pressing CTRL+C.

In the first mode of operation, errors are hidden, and only null is printed. In the second mode of operation more information is printed.

Note that EvalMath supports `()`, `[]` and `{}` for brackets but does not check that the matching bracket is the same type. I.e. `(expr]` is just as valid as `(expr)`.

### Supported operations

- `-` subtraction / negation
- `+` addition
- `*` multiplication
- `/` division
- `%` modulo
- `\` integer division
- `^` exponentiation

### Language specific notes

#### Java

EvalMath does not explicitly check for division by zero. By language design this returns `Infinity` except when using integer division `\`, in which case an error is returned. Modulo `%` returns `NaN`.



## Building JVM-based implementations

Build all languages: `./gradlew build` or `gradlew.bat build` (windows)

Building specific language: `./gradlew :<lang>:build` where `<lang>` is the language folder (currently `java`).

Running specific language: `./gradlew :<lang>:run --console=plain`. To pass arguments, add `--args "<arguments>"`.

For more information visit [the docs](https://docs.gradle.org/current/userguide/userguide.html)
