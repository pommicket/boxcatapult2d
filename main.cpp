#if DEBUG
#define AUTO_RELOAD_CODE 0
#endif

#include "gui.hpp"
#if AUTO_RELOAD_CODE
typedef void (*SimFrameFn)(Frame *);
#include "time.cpp"
#else
#include "sim.cpp"
#endif

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if __unix__
#include <dlfcn.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#elif defined _WIN32
#else
#error "Unsupported operating system."
#endif

#if DEBUG
#if __unix__
#define debug_log printf
#else // __unix__
static void debug_log(char const *fmt, ...) {
	char buf[256];
	va_list args;
	va_start(args, fmt);
	vsprintf_s(buf, sizeof buf, fmt, args);
	va_end(args);
	OutputDebugStringA(buf);
	OutputDebugStringA("\n");
}
#endif // __unix__
#else // DEBUG
#define debug_log(...)
#endif

static void die(char const *fmt, ...) {
	char buf[256] = {0};
	
	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof buf - 1, fmt, args);
	va_end(args);

	// show a message box, and if that fails, print to stderr
	if (SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error", buf, NULL) < 0) {
		fprintf(stderr, "%s\n", buf);
	}

	exit(EXIT_FAILURE);
}

// returns true on success
static bool copy_file(char const *filename_from, char const *filename_to) {
	bool success = false;
	FILE *from = fopen(filename_from, "rb");
	if (from) {
		FILE *to = fopen(filename_to, "wb");
		if (to) {
			char buf[4096];
			success = true;
			while (fread(buf, 1, sizeof buf, from) == sizeof buf) {
				if (fwrite(buf, 1, sizeof buf, to) != sizeof buf) {
					success = false;
				}
			}
			fwrite(buf, 1, sizeof buf, to);
			if (ferror(from) || ferror(to)) {
				success = false;
			}
			fclose(to);
		}
		fclose(from);
	}
	return success;
}

static Key keycode_to_key(SDL_Keycode sym) {
	switch (sym) {
	case SDLK_LEFT: return KEY_LEFT;
	case SDLK_RIGHT: return KEY_RIGHT;
	case SDLK_UP: return KEY_UP;
	case SDLK_DOWN: return KEY_DOWN;
	case SDLK_SPACE: return KEY_SPACE;
	case SDLK_LCTRL: return KEY_LCTRL;
	case SDLK_RCTRL: return KEY_RCTRL;
	case SDLK_LSHIFT: return KEY_LSHIFT;
	case SDLK_RSHIFT: return KEY_RSHIFT;
	case SDLK_RETURN: return KEY_ENTER;
	case SDLK_BACKSPACE: return KEY_BACKSPACE;
	case SDLK_ESCAPE: return KEY_ESCAPE;
	case SDLK_PAGEDOWN: return KEY_PAGEDOWN;
	case SDLK_PAGEUP: return KEY_PAGEUP;
	case SDLK_EXCLAIM: return KEY_EXCLAMATION_MARK;
	case SDLK_AT: return KEY_AT;
	case SDLK_HASH: return KEY_HASH;
	case SDLK_DOLLAR: return KEY_DOLLAR;
	case SDLK_PERCENT: return KEY_PERCENT;
	case SDLK_CARET: return KEY_CARET;
	case SDLK_AMPERSAND: return KEY_AMPERSAND;
	case SDLK_ASTERISK: return KEY_ASTERISK;
	case SDLK_LEFTPAREN: return KEY_LPAREN;
	case SDLK_RIGHTPAREN: return KEY_RPAREN;
	case SDLK_KP_0: return KEY_0;
	case SDLK_PLUS: return KEY_PLUS;
	case SDLK_MINUS: return KEY_MINUS;
	case SDLK_EQUALS: return KEY_EQUALS;
	case SDLK_UNDERSCORE: return KEY_UNDERSCORE;
	default:
		if (sym >= SDLK_a && sym <= SDLK_z) {
			return (u16)(sym - SDLK_a + KEY_A);
		} else if (sym >= SDLK_0 && sym <= SDLK_9) {
			return (u16)(sym - SDLK_0 + KEY_0);
		} else if (sym >= SDLK_KP_1 && sym <= SDLK_KP_9) {
			return (u16)(sym - SDLK_KP_1 + KEY_1);
		} else if (sym >= SDLK_F1 && sym <= SDLK_F12) {
			return (u16)(sym - SDLK_F1 + KEY_F1);
		}
	}
	return KEY_UNKNOWN;
}

static SDL_Scancode key_to_scancode(Key key, bool *shift) {
	*shift = false;
	switch (key) {
	case KEY_LEFT: return SDL_SCANCODE_LEFT;
	case KEY_RIGHT: return SDL_SCANCODE_RIGHT;
	case KEY_UP: return SDL_SCANCODE_UP;
	case KEY_DOWN: return SDL_SCANCODE_DOWN;
	case KEY_SPACE: return SDL_SCANCODE_SPACE;
	case KEY_LCTRL: return SDL_SCANCODE_LCTRL;
	case KEY_RCTRL: return SDL_SCANCODE_RCTRL;
	case KEY_LSHIFT: return SDL_SCANCODE_LSHIFT;
	case KEY_RSHIFT: return SDL_SCANCODE_RSHIFT;
	case KEY_ENTER: return SDL_SCANCODE_RETURN;
	case KEY_BACKSPACE: return SDL_SCANCODE_BACKSPACE;
	case KEY_ESCAPE: return SDL_SCANCODE_ESCAPE;
	case KEY_PAGEDOWN: return SDL_SCANCODE_PAGEDOWN;
	case KEY_PAGEUP: return SDL_SCANCODE_PAGEUP;
	case KEY_MINUS: return SDL_SCANCODE_MINUS;
	case KEY_PLUS: *shift = true; return SDL_SCANCODE_MINUS;
	case KEY_EQUALS: return SDL_SCANCODE_EQUALS;
	case KEY_UNDERSCORE: *shift = true; return SDL_SCANCODE_EQUALS;

	case KEY_EXCLAMATION_MARK: *shift = true; return SDL_SCANCODE_1;
	case KEY_AT: *shift = true; return SDL_SCANCODE_2;
	case KEY_HASH: *shift = true; return SDL_SCANCODE_3;
	case KEY_DOLLAR: *shift = true; return SDL_SCANCODE_4;
	case KEY_PERCENT: *shift = true; return SDL_SCANCODE_5;
	case KEY_CARET: *shift = true; return SDL_SCANCODE_6;
	case KEY_AMPERSAND: *shift = true; return SDL_SCANCODE_7;
	case KEY_ASTERISK: *shift = true; return SDL_SCANCODE_8;
	case KEY_LPAREN: *shift = true; return SDL_SCANCODE_9;
	case KEY_RPAREN: *shift = true; return SDL_SCANCODE_0;

	case KEY_0: return SDL_SCANCODE_0; // SDL_SCANCODE_0 != SDL_SCANCODE_1 - 1 
	default:
		if (key >= KEY_A && key <= KEY_Z) {
			return (SDL_Scancode)(SDL_SCANCODE_A + (key - KEY_A));
		} else if (key >= KEY_1 && key <= KEY_9) {
			return (SDL_Scancode)(SDL_SCANCODE_1 + (key - KEY_1));
		} else if (key >= KEY_F1 && key <= KEY_F12) {
			return (SDL_Scancode)(SDL_SCANCODE_F1 + (key - KEY_F1));
		}
		break;
	}
	return (SDL_Scancode)0;
}

#ifdef _WIN32
int WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR     lpCmdLine,
  int       nShowCmd
) {
	(void)hInstance;
	(void)hPrevInstance;
	(void)lpCmdLine;
	(void)nShowCmd;
#else
int main(void) {
#endif
	Frame frame = {};
	Input *input = &frame.input;
#if AUTO_RELOAD_CODE
	struct timespec dynlib_last_modified = {};
	SimFrameFn sim_frame = NULL;
	#if __unix__
	void *dynlib = NULL;
	#else
	HMODULE dynlib = NULL;
	#endif
#endif

	srand((unsigned)time(NULL));

	SDL_SetHint(SDL_HINT_NO_SIGNAL_HANDLERS, "1");
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		die("%s", SDL_GetError());
	}

	SDL_Window *window = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		1280, 720, SDL_WINDOW_SHOWN|SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	if (!window) {
		die("%s", SDL_GetError());
	}

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GLContext glctx = SDL_GL_CreateContext(window);
	if (!glctx) {
		die("%s", SDL_GetError());
	}

	SDL_GL_SetSwapInterval(1); // vsync

	frame.memory_size = (size_t)16 << 20;

#if DEBUG
	{
		void *address = (void *)(13ULL << 28);
		#if __unix__
		frame.memory = mmap(address, frame.memory_size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		if (frame.memory == MAP_FAILED) {
			frame.memory = NULL;
		}
		#else
		frame.memory = VirtualAlloc(address, frame.memory_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		#endif
	}
#else
	frame.memory = calloc(1, frame.memory_size);
#endif

	if (!frame.memory) {
		die("Couldn't allocate memory (%lu bytes).", (ulong)frame.memory_size);
	}

	frame.get_gl_proc = (void (*(*)(char const *))(void))SDL_GL_GetProcAddress;

	Uint32 last_frame_ticks = SDL_GetTicks();

	while (1) {
		SDL_Event event;
		memset(input, 0, sizeof *input);
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN: {
				Key key = keycode_to_key(event.key.keysym.sym);
				++input->keys_pressed[key];
				if (input->nkey_presses < MAX_KEY_PRESSES_PER_FRAME)
					++input->nkey_presses;
			} break;
			case SDL_KEYUP: {
				Key key = keycode_to_key(event.key.keysym.sym);
				++input->keys_released[key];
			} break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP: {
				int x = event.button.x, y = event.button.y;
				u8 button = 0;
				switch (event.button.button) {
				case SDL_BUTTON_LEFT:
					button = MOUSE_LEFT;
					break;
				case SDL_BUTTON_MIDDLE:
					button = MOUSE_MIDDLE;
					break;
				case SDL_BUTTON_RIGHT:
					button = MOUSE_RIGHT;
					break;
				}
				MousePress *press = NULL;
				if (event.type == SDL_MOUSEBUTTONDOWN) {
					if (input->nmouse_presses < MAX_MOUSE_PRESSES_PER_FRAME)
						press = &input->mouse_presses[input->nmouse_presses++];
				} else {
					if (input->nmouse_releases < MAX_MOUSE_RELEASES_PER_FRAME)
						press = &input->mouse_releases[input->nmouse_releases++];
				}
				if (press) {
					press->button = button;
					press->x = x;
					press->y = y;
				}
			} break;
			case SDL_QUIT:
				input->closed = true;
				break;
			}
		}

		frame.close = input->closed;

	#if AUTO_RELOAD_CODE
	#if __unix__
	#define DYNLIB_EXT "so"
	#else
	#define DYNLIB_EXT "dll"
	#endif
		struct timespec new_dynlib_last_modified = time_last_modified("obj/sim." DYNLIB_EXT "_changed");
		if (timespec_cmp(dynlib_last_modified, new_dynlib_last_modified) != 0) {
			// reload dynlib
			char new_filename[256] = {0};
			snprintf(new_filename, sizeof new_filename-1, "obj/sim%08x%08x." DYNLIB_EXT, rand(), rand());
			copy_file("obj/sim." DYNLIB_EXT, new_filename);

			#if __unix__
			chmod(new_filename, 0755);
			void *new_dynlib = dlopen(new_filename, RTLD_NOW);

			if (new_dynlib) {
				SimFrameFn new_sim_frame = (SimFrameFn)dlsym(new_dynlib, "sim_frame");
				if (new_sim_frame) {
					// function loaded from dynamic library successfully
					if (dynlib) dlclose(dynlib);
					debug_log("Successfully loaded %s\n", new_filename);
					sim_frame = new_sim_frame;
					dynlib = new_dynlib;
					// delete other .so files
					{
						DIR *obj = opendir("obj");
						if (obj) {
							struct dirent *ent;
							while ((ent = readdir(obj))) {
								if (ent->d_type == DT_REG) {
									char *name = ent->d_name;
									char *ext = strrchr(name, '.');
									if (ext && strcmp(ext, ".so") == 0 && strcmp(name, "sim.so") != 0 && strcmp(name, new_filename + 4) != 0) {
										char path[256] = {0};
										snprintf(path, sizeof path - 1, "obj/%s", name);
										debug_log("Deleting old file %s.\n", path);
										unlink(path);
									}
								}
							}
							closedir(obj);
						}
					}
				} else {
					debug_log("Couldn't get sim_frame from dynamic library: %s\n", dlerror());
				}
			} else {
				debug_log("Error opening dynamic library %s: %s\n", new_filename, dlerror());
			}
			
			#else
			HMODULE new_dynlib = LoadLibraryA(new_filename);
			if (new_dynlib) {
				SimFrameFn new_sim_frame = (SimFrameFn)GetProcAddress(new_dynlib, "sim_frame");
				if (new_sim_frame) {
					// function loaded from dynamic librayr successfully
					if (dynlib) FreeLibrary(dynlib);
					debug_log("Successfully loaded %s.\n", new_filename);
					sim_frame = new_sim_frame;
					dynlib = new_dynlib;
					// delete other .dll files
					{
						WIN32_FIND_DATA find_data;
						HANDLE find = FindFirstFileA("obj\\*.dll", &find_data);
						if (find) {
							do {
								char const *filename = find_data.cFileName;
								if (strcmp(filename, "sim.dll") != 0 && strcmp(filename, "sim.dll_changed") != 0 && strcmp(filename, new_filename) != 0) {
									char path[256] = {0};
									snprintf(path, sizeof path-1, "obj/%s", filename);
									debug_log("Deleting old dll: %s\n", path);
									DeleteFileA(path);
								}
							} while (FindNextFile(find, &find_data) != 0);
							FindClose(find);
						} else {
							debug_log("Error finding dll files: %ld\n", (long)GetLastError());
						}
					}
				} else {
					FreeLibrary(new_dynlib);
					debug_log("Couldn't get sim_frame from dynamic library: %ld\n", (long)GetLastError());
				}
			} else {
				debug_log("Error opening dynamic library %s: %ld\n", new_filename, (long)GetLastError());
			}
			#endif
			dynlib_last_modified = new_dynlib_last_modified;
		}
	#endif

		{
			Uint8 const *kbd = SDL_GetKeyboardState(NULL);
			bool *keys_down = input->keys_down;
			bool shift = input->shift = keys_down[KEY_LSHIFT] || keys_down[KEY_RSHIFT];
			input->ctrl = keys_down[KEY_LCTRL] || keys_down[KEY_RCTRL];
			for (Key i = 0; i < NKEYS; ++i) {
				bool needs_shift;
				SDL_Scancode scan = key_to_scancode(i, &needs_shift);
				keys_down[i] = kbd[scan];
				if (needs_shift) {
					keys_down[i] &= shift;
				}
			}
		}

		{
			int w = 0, h = 0;
			SDL_GetWindowSize(window, &w, &h);
			frame.width = w;
			frame.height = h;

			int x = 0, y = 0;
			SDL_GetMouseState(&x, &y);
			input->mouse_x = x;
			input->mouse_y = y;
		}
		
		{
			Uint32 this_frame_ticks = SDL_GetTicks();
			Uint32 frame_ms = 0;
			if (this_frame_ticks > last_frame_ticks) // SDL_GetTicks wraps after 49 days
				frame_ms = this_frame_ticks - last_frame_ticks;
			frame.dt = 0.001 * (double)frame_ms;
			last_frame_ticks = this_frame_ticks;
		}

		sim_frame(&frame);

		SDL_SetWindowTitle(window, frame.title);

		if (frame.close) break;

		SDL_GL_SwapWindow(window);
	}
	return 0;
}
