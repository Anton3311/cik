# Table of Contents

1. [About](#about)
1. [Project Structure](#project-structure)
1. [How to build](#how-to-build)
1. [Running the compiler](#running-the-compiler)
1. [Running tests](#running-tests)
1. [About the Preprocessor](docs/preprocessor.md)
1. [About the Compiler](docs/compiler.md)

# About

This is a compiler for a subset of C99, written fully in C.

It implements a preprocessor, a parser, a compiler and a x64 code generation backend.

> [!IMPORTANT]
> Not standard complient

> [!NOTE]
> Doesn't produce an executable (yet), rather it runs the program in the same process as the compiler.

Although the compiler doesn't yet support a lot of C features, it is already capable of compiling not just simple programs like `printf("hello world");`, but also some algorithms with loops, like bubble sort (an example can be found in the test suite, [here](tests/compiler/test_for_loop_bubble_sort.c))

Features and limitations:

1. `x64` machine code generation
2. Partially supported `cdecl` calling convention. **Only implemented for trivial register sized types like ints or pointers. Bigger types are not supported**.
3. Works only with the `main` function. **Calling other defined functions is not supported**
4. Calling of external functions. These are provided inernally as function pointers by the compiler.
5. `char`, `int`, `short`, `long`, `long long` and their signed/unsigned variats with support for all binary and unary operators.
6. Integer and pointer arithmetics.
7. Pointer dereferencing and assignment.
8. Array indexing and element assignment
9. Conditional branches
10. Comparison operators: `==`, `!=`, `<`, `<=`, `>`, `>=` and unary not `!`.
11. `while`, `for` and `do while` loops
10. String constants
11. Macros: regular and function-like
12. Conditional preprocessor directives
13. `#include`, `#error`, `#undef`, `#pragma once`
14. Builtin macros: `__LINE__`, `__FILE__` and `__STDC__`
15. Macros with variable number of arguments (`__VA_ARGS__`)

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
10. `tester/src` - test runner and tests
11. `tests/src` - preprocessor and compiler tests. Here every test is defined in it's own file. This directory is scanned by the `test_runner`, and adding new tests doesn't require recompiling the whole project.

There are more detailed explanations for parts of the project. These are located in `docs/`.

# How to build

The project is Windows only and can be built using both Clang and MSVC.

The build process produces multiple executables:
- `bin/c.exe` - the compiler
- `bin/test_runner.exe` - a test runner
- `bin/tester.exe` - an exe that actually runs the tests. **Not meant for manual use**. It is only lauched by `test_runner.exe` and its main purpose is to isolate the tests so that in case of a crash the `test_runner` can keep on running other tests.
- `bin/gen.exe` - source code generator, currently only used to generate `code_gen/src/code_gen/instr.gen.c`.

By default the build tool looks for the selected compiler in the `PATH`, however it is also possible to override the search path, by specifying a `;` separated list of paths with the `--compiler-paths=<search-paths>` option.

## Building using clang

To build the project first you need to compile the build tool:

```
.\scripts\build_bb.bat clang
```

Then run the build tool:

```
.\bin\bb.exe build
```

## Building using MSVC

Open the `Developer PowerShell` or `Developer Command Prompt` and run the following command, to compile the build tool:

```
.\scripts\build_bb.bat cl
```

Then run the build tool:

```
.\bin\bb.exe build --cc=cl
```

## Asan

When building with `MSVC` it is possible to compile with address sanitization, by passing `--asan` flag to the build tool.

> [!NOTE]
> `Asan` is `MSVC` only, since `clang`'s implementation is rather buggy.

## Profiling

`Cik` uses `Tracy 0.10.0` as the profiler.

Building with the profiler support requires only passing the `--profiler` flag to the build tool.

> [!NOTE]
> All the required `Tracy` client code is already included in the repository, so it doesn't require any extra steps.
> 
> Compiling with profiler support will work even if you don't have `Tracy` installed, however you will need it to view the profiling results.
> 
> Tracy 0.10.0 release can be found here: https://github.com/wolfpld/tracy/releases#release-v0.10

# Running the compiler

```
.\bin\c.exe <path-to-your-c-file>
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
    --show-ir              print generated IR instructions
    --x64-debug-log        log results of intermediate operations for debugging
    --x64-show-instr-loc   print which storage locations were assigned to each instruction
```

# Running tests

Use `.\bin\test_runner.exe` to run all the tests:

```
.\bin\test_runner.exe
```

It is also possible to run only a specific test:
```
.\bin\test_runner.exe run-test <test-name>
```

The test runner also integrates with RAD Debugger. RAD Debugger is not required for the compiler or the test runner to function. However, it unlocks extra features, that mostly come in handy during development.

Other options:

1. `--stop-on-fail` - stop after the first failing test
2. `--debug-failed` - automatically add failing tests as targets to RAD Debugger.
3. `--raddbg-path=<path>` - explicitely specify the path to the `raddbg.exe`. By default test runner will look for `raddbg.exe` in the `PATH`.
