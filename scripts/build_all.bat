clang.exe -c driver/src/driver/main.c -m64 -g -Wall -o bin/obj/main.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
clang.exe -c core/src/core/core.c -m64 -g -Wall -o bin/obj/core.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
clang.exe -c core/src/core/profiler.c -m64 -g -Wall -o bin/obj/profiler.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
llvm-lib.exe /OUT:bin/core.lib bin/obj/core.o bin/obj/profiler.o
clang.exe -c parser/src/parser/diagnostics.c -m64 -g -Wall -o bin/obj/diagnostics.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
clang.exe -c parser/src/parser/parsed_ast.c -m64 -g -Wall -o bin/obj/parsed_ast.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
clang.exe -c parser/src/parser/parser.c -m64 -g -Wall -o bin/obj/parser.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
clang.exe -c parser/src/parser/parse_tools.c -m64 -g -Wall -o bin/obj/parse_tools.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
clang.exe -c parser/src/parser/preprocessor.c -m64 -g -Wall -o bin/obj/preprocessor.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
clang.exe -c parser/src/parser/source_info.c -m64 -g -Wall -o bin/obj/source_info.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
clang.exe -c parser/src/parser/tokenizer.c -m64 -g -Wall -o bin/obj/tokenizer.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
llvm-lib.exe /OUT:bin/parser.lib bin/obj/diagnostics.o bin/obj/parsed_ast.o bin/obj/parser.o bin/obj/parse_tools.o bin/obj/preprocessor.o bin/obj/source_info.o bin/obj/tokenizer.o bin/core.lib
clang.exe -c compiler/src/compiler/compiler.c -m64 -g -Wall -o bin/obj/compiler.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
llvm-lib.exe /OUT:bin/compiler.lib bin/obj/compiler.o
clang.exe -c code_gen/src/code_gen/code_gen.c -m64 -g -Wall -o bin/obj/code_gen.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
clang.exe -c code_gen/src/code_gen/instr.c -m64 -g -Wall -o bin/obj/instr.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
clang.exe -c code_gen/src/code_gen/instr.gen.c -m64 -g -Wall -o bin/obj/instr.gen.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
clang.exe -c code_gen/src/code_gen/backends/x64.c -m64 -g -Wall -o bin/obj/x64.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
clang.exe -c code_gen/src/code_gen/backends/x64_encoding.c -m64 -g -Wall -o bin/obj/x64_encoding.o "-Icore/src" "-Iparser/src" "-Icompiler/src" "-Icode_gen/src"
llvm-lib.exe /OUT:bin/code_gen.lib bin/obj/code_gen.o bin/obj/instr.o bin/obj/instr.gen.o bin/obj/x64.o bin/obj/x64_encoding.o bin/core.lib
clang.exe -m64 -g -o bin/c.exe bin/obj/main.o -lbin/core.lib -lbin/parser.lib -lbin/compiler.lib -lbin/code_gen.lib -lDbghelp.lib -lShlwapi.lib -lPathcch.lib -lAdvapi32.lib
clang.exe -c gen/src/gen/gen_main.c -m64 -g -Wall -o bin/obj/gen_main.o "-Icore/src" "-Iparser/src"
clang.exe -m64 -g -o bin/gen.exe bin/obj/gen_main.o -lbin/core.lib -lbin/parser.lib -lDbghelp.lib -lShlwapi.lib -lPathcch.lib -lAdvapi32.lib
clang.exe -c tester/src/tester/tester_main.c -m64 -g -Wall -o bin/obj/tester_main.o "-Icore/src/" "-Itester/src/" "-Iparser/src/" "-Icompiler/src/" "-Icode_gen/src/"
clang.exe -c tester/src/tester/tests.c -m64 -g -Wall -o bin/obj/tests.o "-Icore/src/" "-Itester/src/" "-Iparser/src/" "-Icompiler/src/" "-Icode_gen/src/"
clang.exe -c tester/src/tester/tests_x64.c -m64 -g -Wall -o bin/obj/tests_x64.o "-Icore/src/" "-Itester/src/" "-Iparser/src/" "-Icompiler/src/" "-Icode_gen/src/"
clang.exe -c tester/src/tester/tests_compiler.c -m64 -g -Wall -o bin/obj/tests_compiler.o "-Icore/src/" "-Itester/src/" "-Iparser/src/" "-Icompiler/src/" "-Icode_gen/src/"
clang.exe -m64 -g -o bin/tester.exe bin/obj/tester_main.o bin/obj/tests.o bin/obj/tests_x64.o bin/obj/tests_compiler.o -lbin/core.lib -lbin/parser.lib -lbin/compiler.lib -lbin/code_gen.lib -lDbghelp.lib -lShlwapi.lib -lPathcch.lib -lAdvapi32.lib
clang.exe -c tester/src/tester/test_runner.c -m64 -g -Wall -o bin/obj/test_runner.o "-Icore/src/" "-Itester/src/"
clang.exe -m64 -g -o bin/test_runner.exe bin/obj/test_runner.o -lbin/core.lib -lDbghelp.lib -lShlwapi.lib -lPathcch.lib -lAdvapi32.lib
