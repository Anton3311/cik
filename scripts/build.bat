clang ^
	driver\src\driver\main.c ^
	core\src\core\core.c ^
	core\src\core\profiler.c ^
	parser\src\parser\tokenizer.c ^
	parser\src\parser\preprocessor.c ^
	parser\src\parser\source_info.c ^
	parser\src\parser\diagnostics.c ^
	parser\src\parser\parsed_ast.c ^
	parser\src\parser\parser.c ^
	parser\src\parser\parse_tools.c ^
	compiler\src\compiler\compiler.c ^
	code_gen\src\code_gen\instr.c ^
	code_gen\src\code_gen\instr.gen.c ^
	code_gen\src\code_gen\backends\x64.c ^
	-g -lDbghelp.lib -lShlwapi.lib -lPathcch.lib -lAdvapi32.lib ^
	-m64 -Wall -o bin\c.exe -Icore\src\ -Iparser\src\ -Icompiler\src -Icode_gen\src

clang ^
	core\src\core\core.c ^
	core\src\core\profiler.c ^
	tester\src\tester\tester_main.c ^
	tester\src\tester\tests.c ^
	tester\src\tester\tests_x64.c ^
	tester\src\tester\tests_compiler.c ^
	parser\src\parser\tokenizer.c ^
	parser\src\parser\preprocessor.c ^
	parser\src\parser\source_info.c ^
	parser\src\parser\diagnostics.c ^
	parser\src\parser\parser.c ^
	parser\src\parser\parse_tools.c ^
	parser\src\parser\parsed_ast.c ^
	compiler\src\compiler\compiler.c ^
	code_gen\src\code_gen\instr.c ^
	code_gen\src\code_gen\instr.gen.c ^
	code_gen\src\code_gen\backends\x64.c ^
	-g -lDbghelp.lib -lShlwapi.lib -lPathcch.lib -lAdvapi32.lib ^
	-m64 -Wall -o bin\tester.exe -Icore\src\ -Itester\src\ -Iparser\src -Icompiler\src -Icode_gen\src

clang ^
	core\src\core\core.c ^
	core\src\core\profiler.c ^
	tester\src\tester\test_runner.c ^
	-g -lDbghelp.lib -lShlwapi.lib -lPathcch.lib -lAdvapi32.lib ^
	-m64 -Wall -o bin\test_runner.exe -Icore\src\ -Itester\src\ -Icompiler\src -Icode_gen\src

clang ^
	core\src\core\core.c ^
	builder\src\builder\builder_main.c ^
	builder\src\builder\builder_core.c ^
	-g -lDbghelp.lib -lShlwapi.lib -lPathcch.lib -lAdvapi32.lib ^
	-m64 -Wall -o bin\bb.exe -Icore\src\ -Ibuilder\src\
