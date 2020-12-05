#include "gui.h"
#ifdef _WIN32
#include <windows.h>
#include "lib/glcorearb.h"
#endif
#include <GL/gl.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <ctype.h>

#define MATH_GL
#include "math.c"
#include "sim.h"
#include "time.c"
#include "util.c"
#include "base.c"


static void platforms_render(Platform *platforms, u32 nplatforms) {
static float t = 0.0f;
	glColor3f(1,0,1);
	glBegin(GL_QUADS);
	for (Platform *platform = platforms, *end = platform + nplatforms; platform != end; ++platform) {
		platform->angle = t += 0.01f;
		// calculate endpoints of platform
		float radius = platform->size * 0.5f;
		v2 endpoint1 = v2_add(platform->center, v2_polar(radius, platform->angle));
		v2 endpoint2 = v2_sub(platform->center, v2_polar(radius, platform->angle));

		v2 thickness = v2_polar(0.005f, platform->angle - HALF_PIf);
		v2_gl_vertex(v2_sub(endpoint1, thickness));
		v2_gl_vertex(v2_sub(endpoint2, thickness));
		v2_gl_vertex(v2_add(endpoint2, thickness));
		v2_gl_vertex(v2_add(endpoint1, thickness));
	}
	glEnd();
}

#ifdef _WIN32
__declspec(dllexport)
#endif
#ifdef __cplusplus
extern "C"
#endif
void sim_frame(Frame *frame) {
	if (frame->memory_size < sizeof(State)) {
		printf("Not enough memory (got %lu, require %lu).\n", (ulong)frame->memory_size, (ulong)sizeof(State));
		frame->close = true;
		return;
	}
	State *state = (State *)frame->memory;
	i32 width = frame->width, height = frame->height;
	Input *input = &frame->input;
	maybe_unused u8 *keys_pressed = input->keys_pressed;
	maybe_unused bool *keys_down = input->keys_down;

	state->win_width  = width;
	state->win_height = height;
	
	// set up GL
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, width, height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	//glOrtho(, -1, +1);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);


	if (!state->initialized) {
		logln("Initializing...");
		strcpy(frame->title, "physics");
			

		state->nplatforms = 1;
		Platform *p = &state->platforms[0];
		p->center = V2(0.5f, 0.5f);
		p->angle  = PIf * 0.3f;
		p->size   = 0.2f;
		
		state->initialized = true;
	#if DEBUG
		state->magic_number = MAGIC_NUMBER;
	#endif
	}
	if (input->keys_pressed[KEY_ESCAPE]) {
		frame->close = true;
		return;
	}

	platforms_render(state->platforms, state->nplatforms);

	#if DEBUG
	GLuint error = glGetError();
	if (error) {
		printf("!!! GL ERROR: %u\n", error);
	}
	#endif
}
