@echo off
del boxcatapult2d-windows.zip
rd /s/q boxcatapult2d
mkdir boxcatapult2d
mkdir boxcatapult2d\assets
copy assets\*.* boxcatapult2d\assets
copy LICENSE boxcatapult2d
copy SDL2.dll boxcatapult2d
copy README.md boxcatapult2d
del boxcatapult2d.exe

call make release
copy boxcatapult2d.exe boxcatapult2d
7z a boxcatapult2d-windows.zip boxcatapult2d
