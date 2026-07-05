@echo off
if [%1] == [clang] (
	@echo on
	clang ^
		core\src\core\core.c ^
		builder\src\builder\builder_main.c ^
		builder\src\builder\builder_core.c ^
		-g -lDbghelp.lib -lShlwapi.lib -lPathcch.lib -lAdvapi32.lib ^
		-m64 -Wall -o bin\bb.exe -Icore\src\ -Ibuilder\src\
	@echo off
) else if [%1] == [cl] (
	@echo on
	cl ^
		/nologo core\src\core\core.c /std:c11 ^
		builder\src\builder\builder_main.c ^
		builder\src\builder\builder_core.c ^
		/Icore/src /Ibuilder/src ^
		/DEBUG /Z7 /Fe"bin/bb.exe" Dbghelp.lib Shlwapi.lib Pathcch.lib Advapi32.lib

	rm builder_main.obj
	rm builder_core.obj
	rm core.obj

	@echo off
) else (
	echo No compiler specified. Pass `clang` or `cl` as the first argument
)
@echo on
