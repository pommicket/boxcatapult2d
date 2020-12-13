# Boxcatapult2D

Computer-generated catapults.

Somewhat inspired by [boxcar2d](https://boxcar2d.com).

## Editor controls
@TODO

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
Then, download SDL2 (Visual C++ 32/64-bit): https://www.libsdl.org/download-2.0.php  
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
