To install box2d on Linux/OS X:
```bash
git clone https://github.com/erincatto/box2d/
cd box2d
mkdir -p build
cd build
cmake -DBOX2D_BUILD_TESTBED=False ..
sudo make -j8 install
```
On Windows (you need `vcvarsall.bat` and `git` in your PATH):
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
You will also need SDL2.
