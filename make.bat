@echo off
if _%VCVARS% == _ ( 
	set VCVARS=1
	call vcvarsall x64
)
if not exist obj mkdir obj

SET CFLAGS=/nologo /W4 /wd4505 /wd4706 /D_CRT_SECURE_NO_WARNINGS /I SDL2/include /I box2d SDL2/lib/x64/SDL2main.lib SDL2/lib/x64/SDL2.lib opengl32.lib box2d.lib /MD
rc /nologo boxcatapult2d.rc
if _%1 == _ (
	cl main.cpp /DDEBUG /DEBUG /Zi %CFLAGS% /Fo:obj/urbs /Fe:boxcatapult2d boxcatapult2d.res
	rem cl sim.cpp /DDEBUG /DEBUG /LD %CFLAGS% /Fo:obj/sim /Fe:obj/sim
	rem echo > obj\sim.dll_changed
)
if _%1 == _release cl main.cpp /O2 %CFLAGS% /Fe:boxcatapult2d boxcatapult2d.res
