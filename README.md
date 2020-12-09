To install box2d:
```bash
git clone https://github.com/erincatto/box2d/
cd box2d
mkdir -p build
cd build
cmake -DBOX2D_BUILD_TESTBED=False ..
sudo make -j8 install
```
