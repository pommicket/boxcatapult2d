@echo off
if _%VCVARS% == _ ( 
	set VCVARS=1
	call vcvarsall x64
)
if not exist obj mkdir obj

SET CFLAGS=/nologo /W3 /D_CRT_SECURE_NO_WARNINGS /I SDL2/include SDL2/lib/x64/SDL2main.lib SDL2/lib/x64/SDL2.lib opengl32.lib
if _%1 == _ (
	cl main.c /DDEBUG /DEBUG /Zi %CFLAGS% /Fo:obj/urbs /Fe:urbs
	cl sim.c /DDEBUG /DEBUG /LD %CFLAGS% /Fo:obj/sim /Fe:obj/sim
	echo > obj\sim.dll_changed
)
if _%1 == _release cl main.c /O2 %CFLAGS% /Fe:urbs
if _%1 == _profile cl main.c /O2 /DPROFILE %CFLAGS% /Fe:urbs
