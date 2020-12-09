WARNINGS=-Wall -Wextra -Wshadow -Wconversion -Wpedantic -pedantic -std=gnu++11 -Wno-unused-function -Wimplicit-fallthrough
LIBS=-ldl `pkg-config --libs --cflags sdl2 gl` -l:libbox2d.a
DEBUG_CFLAGS=$(CFLAGS) $(WARNINGS) $(LIBS) -DDEBUG -O0 -g
obj/debug: physics obj/sim.so obj
	touch obj/debug
physics: main.cpp gui.hpp sim.cpp time.cpp
	$(CXX) main.cpp -o $@ $(DEBUG_CFLAGS)
obj/sim.so: *.[ch]* obj
	$(CXX) sim.cpp -fPIC -shared -o $@ $(DEBUG_CFLAGS)
	touch obj/sim.so_changed
obj:
	mkdir -p obj
