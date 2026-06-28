# Table of Contents

1. [About](#about)
1. [Building](#building)
1. [Running a compiler](#running-a-compiler)
1. [Running tests](#running-tests)
1. [Project Structure](#project-structure)
1. [Features](#features)
1. [About the Preprocessor](docs/preprocessor.md)
1. [Preprocessor features](#preprocessor-features)
1. [Parser features](#parser-features)
1. [About the Compiler](docs/compiler.md)
1. [Compiler features](#compiler-features)

# About

This is a compiler for a subset of C99, written fully in C.

It preprocesses, parses and compiles C source into x64 machine code.

> [!IMPORTANT]
> Not standard complient

> [!NOTE]
> Worth noting that it doesn't produce an executable (yet), rather it runs the program in the same process as the compiler.

# Building

> [!NOTE]
> The project is Windows only and can only be built using Clang.

There are two ways to build the project:
1. [Using an auto-generated batch script](#building-using-batch-script)
2. [Using a build tool](#building-using-the-build-tool)

The build process produces multiple executables:
- `bin/c.exe` - the compiler
- `bin/test_runner.exe` - a test runner
- `bin/tester.exe` - an exe that actually runs the tests. **Not meant for manual use**. It is only lauched by `test_runner.exe` and it's main purpose is to isolate the tests so that in case of a crash the `test_runner` can keep on running other tests.
- `bin/gen.exe` - source code generator, currently only used to generate `code_gen/src/code_gen/instr.gen.c`.

### Build using batch script

Just run the following script to compile everything using clang:

```
scripts/build_all.bat
```

### Build using the build tool

To build the project using the build tool, first you need to compile the build tool by running the next script:

```
scripts/build_bb.bat
```

After successful compilation run:
```
bin/bb.exe build
```

# Running a compiler

```
bin/c.exe <path-to-your-c-file>
```

```
Usage:
  c.exe <path-to-c-file>

Compiler flags:
  --no-win-sdk           don't add Win SDK to include path
  -I<include-path>       specify an include path
  --show-ast             print AST after parsing

Backend flags:
  --keep-dead-instr      don't eliminate dead instructions
  --show-ir              print IR instructions before lowering to machine code
```

# Running tests

Use `bin/test_runner.exe` to run the tests:

```
bin/test_runner.exe
```

# Project structure

1. `builder/src` - a build tool for compiling the compiler
2. `code_gen/src` - intermediate representation
3. `code_gen/src/code_gen/backends/x64` - x64 backend
4. `compiler/src` - compiler implementation
5. `core/src` - common code: arenas, allocators, strings and OS abstractions
6. `driver/src` - the entry point of the whole compiler
7. `gen/src` - source code generators
8. `parser/src` - parser and preprocessor
9. `stdx/src` - some simplified versions of standard library headers, that are sometimes used to work around the limitations of the preprocessor.
10. `tester/src` - test runner and unit tests
11. `tests/src` - preprocessor and compiler tests (every test is defined in it's own file. This directory is scanned by the `test_runner`, and adding new tests doesn't require compilation, like for the unit tests)

There are more detailed explanations for parts of the project. These are located in `docs/`.

# Features

## Preprocessor features

1. Preprocessor macros: regular and function-like
1. Conditional preprocessor directives
1. #include, #error, #undef directives
1. #pragma once
1. Builtin macros: `__LINE__`, `__FILE__` and `__STDC__`
1. Macros with variable number of arguments (`__VA_ARGS__`)

## Parser features

1. Binary and unary expressions
2. Functions (forward declarations and definitions)
3. `struct`, `union`, `enum` and `typedef` (both forward declarations and definitions)
4. If/else statements
5. Returns statements

## Compiler features

1. `x64` machine code generation
2. Partially supported `cdecl` calling convetion. (Only implemented for trivial register sized types like ints or pointers. Bigger types are not supported).
2. Works only with the `main` function. **Doesn't support calling other defined functions.**
3. Calling of external functions. These are provided as function pointers to the compiler.
4. Integer and pointer arithmetics
5. Conditional branches and comparison operators: `==`, `!=`, `<`, `<=`, `>`, `>=`
