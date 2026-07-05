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

It implements a preprocessor, a parser, a compiler and a x64 code generation backend.

> [!IMPORTANT]
> Not standard complient

> [!NOTE]
> Worth noting that it doesn't produce an executable (yet), rather it runs the program in the same process as the compiler.

# Building

> [!NOTE]
> The project is Windows only and can be built using both Clang and MSVC.

There are two ways to build the project:
1. [Using an auto-generated batch script](#building-using-batch-script) (Clang only)
2. [Using a build tool](#building-using-the-build-tool)

The build process produces multiple executables:
- `bin/c.exe` - the compiler
- `bin/test_runner.exe` - a test runner
- `bin/tester.exe` - an exe that actually runs the tests. **Not meant for manual use**. It is only lauched by `test_runner.exe` and it's main purpose is to isolate the tests so that in case of a crash the `test_runner` can keep on running other tests.
- `bin/gen.exe` - source code generator, currently only used to generate `code_gen/src/code_gen/instr.gen.c`.

### Build using batch script

Just run the following script to compile everything using `clang`:

```
scripts/build_all.bat
```

### Build using the build tool

To build the project using the build tool, first you need to compile the build tool by running the next script:

```
scripts/build_bb.bat clang
```

or using `MSVC`:

```
scripts/build_bb.bat cl
```

The build tool uses `clang` by default and exepects it to be in the `PATH`. To compile using `clang` just run:
```
bin/bb.exe build
```

If you want to use `MSVC` as the compiler, then you can do this from the `Developer PowerShell` or `Developer Command Prompt` by running the following command:
```
bin/bb.exe build --cc=cl
```

By default the build tool looks for the selected compiler in the `PATH`, however it is also possible to override the search path, by specifying a `;` separated list of paths with the `--compiler-paths=<search-paths>` option.

### Asan

When building with `MSVC` it is possible to compile with address sanitization, by passing `--asan` flag to the build tool.

> [!NOTE]
> `Asan` is `MSVC` only, since `clang`'s implementation is rather buggy.

### Profiling

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
    --show-ir              print generated IR instructions
    --x64-debug-log        log results of intermediate operations for debugging
    --x64-show-instr-loc   print which storage locations were assigned to each instruction
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
1. `#include`, `#error`, `#undef` directives
1. `#pragma once`
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
2. Works only with the `main` function. **Calling other defined functions is not supported**
3. Calling of external functions. These are provided as function pointers by the compiler.
4. Integer and pointer arithmetics, dereferencing and assignment
5. Conditional branches and comparison operators: `==`, `!=`, `<`, `<=`, `>`, `>=`
6. String constants
7. Array indexing and element assignment
