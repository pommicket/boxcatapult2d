# Boxcatapult2D

Computer-generated catapults.

Created for [PROCJAM 2020](https://itch.io/jam/procjam).

Somewhat inspired by [boxcar2d](http://boxcar2d.com).

Press F11 to toggle fullscreen and Ctrl+Q (or just close the window) to quit.

## How it works

The algorithm begins by generating 110 random catapults, then throwing out all but the top 10 (in terms of
the distance the ball traveled).
Then, for each generation, 100 new catapults are generated from the top 10 from the last generation.
Each new catapult is a copy of one of the old ones, with a chance of having some "mutations".
These mutations slightly modify the setup, by changing the positions, sizes, etc. of platforms.
Each generation has catapults with different mutations rates (20 with a 5% mutation rate per platform, 20 with a 10%
mutation rate, 20 with a 20% mutation rate, 20 with a 30% mutation rate, and 20 completely random new setups).

## Editor controls
Left mouse - build / edit platform  
Right mouse - delete platform  
W/A/S/D - pan  
Up/Down - change platform size  
Left/Right - change platform angle  
R - toggle rotating platform  
Q - decrease rotation speed  
E - increase rotation speed  
M - toggle moving platform  
Z - decrease move speed  
X - increase move speed  
space - simulate catapult  

# Compiling it yourself
You will need SDL2 and Box2D.

## Linux/OS X/etc.
First, install SDL2. On Debian/Ubuntu, you can do this with
```bash
sudo apt install libsdl2-dev
```

You need the latest version of Box2D. The versions in Debian stable/testing aren't new enough.
You can install it with:
```bash
git clone https://github.com/erincatto/box2d/
cd box2d
mkdir -p build
cd build
cmake -DBOX2D_BUILD_TESTBED=False ..
sudo make -j8 install
```

Now, just run `make release`, and you will get the executable `boxcatapult2d`.

## Windows
First, you will need MSVC and `vcvarsall.bat` in your PATH.  
Then, download <a href="https://www.libsdl.org/download-2.0.php" target="_blank">SDL2 (Visual C++ 32/64-bit)</a>.  
Copy the contents of the folder `SDL2-something\lib\x64` into the same directory as Boxcatapult2D,  
and copy the folder `SDL2-something\include` there too, renaming it to `SDL2`.  

Next, install `Box2D` (you will need `git` in your PATH):
```bash
vcvarsall x64
git clone https://github.com/erincatto/box2d/
cd box2d
xcopy /s /i include\box2d ..\box2d
mkdir build
cd build
cmake ..
cmake --build . --config Release
copy bin\Release\box2d.lib ..\..
```

Now, you should be able to run `make.bat release` and you will get `boxcatapult2d.exe`.

## Report a bug

If you find a bug, you can report it to pommicket at gmail.com if you'd like.

## Credits
Box2D: A physics library  
See: https://box2d.org  

Roboto font (assets/font.ttf):  
Courtesy of Google Fonts.  
Apache 2.0 license (see: assets/apache-2.0.txt)  

STB truetype (lib/stb\_truetype.h):  
See: https://github.com/nothings/stb  
