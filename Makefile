WARNINGS=-Wall -Wextra -Wshadow -Wconversion -Wpedantic -pedantic -std=gnu++11 -Wno-unused-function -Wimplicit-fallthrough
LIBS=-ldl `pkg-config --libs --cflags sdl2 gl` -l:libbox2d.a
DEBUG_CFLAGS=$(CFLAGS) $(WARNINGS) $(LIBS) -DDEBUG -O0 -g3
RELEASE_CFLAGS=$(CFLAGS) $(WARNINGS) $(LIBS) -O3 -s
boxcatapult2d: *.[ch]*
	$(CXX) main.cpp -o $@ $(DEBUG_CFLAGS)
release: *.[ch]* obj
	$(CXX) main.cpp -o boxcatapult2d $(RELEASE_CFLAGS)
# obj/sim.so: *.[ch]* obj
# 	$(CXX) sim.cpp -fPIC -shared -o $@ $(DEBUG_CFLAGS)
#	touch obj/sim.so_changed
obj:
	mkdir -p obj
clean:
	rm boxcatapult2d
