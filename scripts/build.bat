clang ^
	driver\src\driver\main.c ^
	core\src\core\core.c ^
	parser\src\parser\tokenizer.c ^
	parser\src\parser\preprocessor.c ^
	parser\src\parser\source_info.c ^
	parser\src\parser\diagnostics.c ^
	parser\src\parser\parsed_ast.c ^
	parser\src\parser\parser.c ^
	parser\src\parser\parse_tools.c ^
	-g -lDbghelp.lib -lShlwapi.lib -lPathcch.lib -lAdvapi32.lib ^
	-m64 -Wall -o bin\c.exe -Icore\src\ -Iparser\src\

clang ^
	core\src\core\core.c ^
	tester\src\tester\tester_main.c ^
	tester\src\tester\tests.c ^
	parser\src\parser\tokenizer.c ^
	parser\src\parser\preprocessor.c ^
	parser\src\parser\source_info.c ^
	parser\src\parser\diagnostics.c ^
	parser\src\parser\parser.c ^
	parser\src\parser\parse_tools.c ^
	parser\src\parser\parsed_ast.c ^
	-g -lDbghelp.lib -lShlwapi.lib -lPathcch.lib -lAdvapi32.lib ^
	-m64 -Wall -o bin\tester.exe -Icore\src\ -Itester\src\ -Iparser\src

clang ^
	core\src\core\core.c ^
	tester\src\tester\test_runner.c ^
	-g -lDbghelp.lib -lShlwapi.lib -lPathcch.lib -lAdvapi32.lib ^
	-m64 -Wall -o bin\test_runner.exe -Icore\src\ -Itester\src\
