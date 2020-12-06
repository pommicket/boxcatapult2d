WARNINGS=-Wall -Wextra -Wshadow -Wconversion -Wpedantic -pedantic -std=gnu11 -Wno-unused-function -Wno-fixed-enum-extension -Wimplicit-fallthrough
LIBS=-lSDL2 -lGL -ldl
DEBUG_CFLAGS=$(CFLAGS) $(WARNINGS) $(LIBS) -DDEBUG -O0 -g
obj/debug: physics obj/sim.so
	touch obj/debug
physics: main.c gui.h sim.c time.c
	$(CC) main.c -o $@ $(DEBUG_CFLAGS)
obj/sim.so: *.[ch]
	$(CC) sim.c -fPIC -shared -o $@ $(DEBUG_CFLAGS)
	touch obj/sim.so_changed
