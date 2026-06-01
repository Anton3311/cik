clang ^
	core\src\core\core.c ^
	builder\src\builder\builder_main.c ^
	builder\src\builder\builder_core.c ^
	-g -lDbghelp.lib -lShlwapi.lib -lPathcch.lib -lAdvapi32.lib ^
	-m64 -Wall -o bin\bb.exe -Icore\src\ -Ibuilder\src\
