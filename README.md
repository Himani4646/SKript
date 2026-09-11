# SKript

SKript is a small Python-inspired programming language interpreter written in modern C++. It is designed as a portfolio project that demonstrates the complete language-toolchain path: lexing, parsing, AST construction, bytecode compilation, and stack-based virtual-machine execution.

## Current status

Milestone 6 is complete: the runtime includes `len()`, `type()`, and `range()` built-ins alongside the `print(...)` statement. The VM is the path that will become the final language runtime.

## Syntax preview

```skript
x = 10
values = [x, 20, "thirty"]

def square(n):
    return n * n

if x > 5:
    print(square(x))
```

## Planned architecture

```text
Source -> Lexer -> Parser -> AST -> Bytecode Compiler -> Virtual Machine
                                              |              |
                                           Runtime values and environments
```

The lexer produces `Token` values containing a token type, its source text (or decoded string value), and a one-based line and column. It also emits `NEWLINE`, `INDENT`, and `DEDENT` tokens so the parser can construct Python-style blocks. The recursive-descent parser constructs AST nodes for literals, variables, unary and binary expressions, calls, assignment, print, `if`/`else`, `while`, functions, and `return`. The compiler translates those nodes to bytecode instructions such as `LOAD_CONSTANT`, `LOAD_VARIABLE`, `STORE_VARIABLE`, arithmetic operations, `COMPARE`, jumps, `CALL`, `RETURN`, and `PRINT`. The VM executes them with a value stack, global variables, and call frames.

Heap-backed functions and lists use `std::shared_ptr` reference counting; see [the memory-management design](docs/memory-management.md) for ownership details and current constraints.

## Standard library

`print(value)` writes a value followed by a newline. `len(value)` accepts strings and lists. `type(value)` returns names such as `int`, `str`, and `list`. `range(stop)`, `range(start, stop)`, and `range(start, stop, step)` create integer lists.

## Build and test

Requirements: CMake 3.20+, a C++17-capable compiler, and Git (CMake downloads GoogleTest on the first test-enabled configure).

GitHub Actions runs this build and test suite on both Windows and Linux for every push and pull request.

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Run a program with the compiled VM:

```sh
./build/skript examples/factorial.skr
```

On Windows, the executable is `build\\skript.exe`.

Run it with no filename to open the interactive prompt:

```sh
./build/skript
```

Use `:quit` to exit. Expressions print their result in the prompt; for `if`, `while`, and `def` blocks, enter a blank line after the final indented line to run the whole block.

## Repository layout

```text
src/lexer/       Milestone 1 tokenizer
src/parser/      Recursive-descent parser (planned)
src/ast/         AST nodes (planned)
src/compiler/    Bytecode compiler (planned)
src/vm/          Stack VM (planned)
src/runtime/     Values, environments, and objects (planned)
tests/           GoogleTest test suite
examples/        Example .skr programs
```

## Future improvements

Upcoming milestones add indentation-aware blocks, recursive-descent parsing, dynamic runtime values and functions, bytecode, a VM, lists, and a memory-management strategy.
