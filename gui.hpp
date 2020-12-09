#ifndef PLATFORM_H_
#define PLATFORM_H_

#ifdef DEBUG
#undef DEBUG
#define DEBUG 1
#else
#define DEBUG 0
#define NDEBUG 1
#endif

#include "types.hpp"

enum {
	KEY_UNKNOWN,
	KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_DOWN,
	KEY_PAGEDOWN, KEY_PAGEUP,
	KEY_SPACE,
	KEY_BACKSPACE,
	KEY_ESCAPE,
	KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M,
	KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z,
	KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,
	KEY_EXCLAMATION_MARK, KEY_AT, KEY_HASH, KEY_DOLLAR, KEY_PERCENT,
	KEY_CARET, KEY_AMPERSAND, KEY_ASTERISK, KEY_LPAREN, KEY_RPAREN,
	KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
	KEY_LCTRL, KEY_RCTRL,
	KEY_LSHIFT, KEY_RSHIFT,
	KEY_ENTER,
	KEY_MINUS, KEY_UNDERSCORE,
	KEY_EQUALS, KEY_PLUS,
	NKEYS
};
typedef u16 Key;

#define MOUSE_OTHER 0
#define MOUSE_LEFT 1
#define MOUSE_MIDDLE 2
#define MOUSE_RIGHT 3

typedef struct {
	u8 button;
	i32 x, y;
} MousePress;
typedef MousePress MouseRelease;

typedef struct {
	bool closed; // was the window closed?
	u8 keys_pressed[NKEYS];  // [i] = how many times was key #i pressed  this frame?
	u8 keys_released[NKEYS]; // [i] = how many times was key #i released this frame?
	bool keys_down[NKEYS]; // [i] = is key #i down?
	u16 nmouse_presses;
#define MAX_MOUSE_PRESSES_PER_FRAME 256
	MousePress mouse_presses[MAX_MOUSE_PRESSES_PER_FRAME];
	u16 nmouse_releases;
#define MAX_MOUSE_RELEASES_PER_FRAME MAX_MOUSE_PRESSES_PER_FRAME 
	MousePress mouse_releases[MAX_MOUSE_RELEASES_PER_FRAME];

	i32 mouse_x, mouse_y; // (+y = down)
	bool shift, ctrl;
} Input;

typedef struct {
	Input input;
	i32 width, height; // window width and height in pixels
	bool close; // should the window be closed? default: input.closed
	void *memory;
	size_t memory_size;
	double dt; // time in seconds between this frame and the last one
	void (*(*get_gl_proc)(char const *))(void); // get a GL function
	char title[64]; // window title
} Frame;

#endif // PLATFORM_H_
