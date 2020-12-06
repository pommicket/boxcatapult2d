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

// compile a vertex or fragment shader
static GLuint shader_compile_from_file(GL *gl, char const *filename, GLenum shader_type) {
	FILE *fp = fopen(filename, "rb");
	if (filename) {
		char code[16384] = {0};
		char log[4096] = {0};
		char const *const_code = code;
		fread(code, 1, sizeof code, fp);
		GLuint shader = gl->CreateShader(shader_type);
		if (shader == 0) {
			logln("Couldn't create shader: %u",glGetError());
		}
		gl->ShaderSource(shader, 1, &const_code, NULL);
		gl->CompileShader(shader);
		gl->GetShaderInfoLog(shader, sizeof log - 1, NULL, log);
		if (*log) {
			logln("Error compiling shader:\n%s", log);
			gl->DeleteShader(shader);
			return 0;
		}
		return shader;
	} else {
		return 0;
	}
}

#if DEBUG
static struct timespec shader_get_last_modified(ShaderBase *shader) {
	// the last modified time of the shader is whichever of the vertex/fragment shaders was modified last
	return timespec_max(
		time_last_modified(shader->vertex_filename),
		time_last_modified(shader->fragment_filename)
	);
}
#endif

static GLuint shader_attrib_location(GL *gl, ShaderBase *shader, char const *attrib) {
	GLint loc = gl->GetAttribLocation(shader->program, attrib);
	if (loc == -1) {
		printf("Couldn't find vertex attribute %s.\n", attrib);
		return 0;
	}
	return (GLuint)loc;
}

static GLint shader_uniform_location(GL *gl, ShaderBase *shader, char const *uniform) {
	GLint loc = gl->GetUniformLocation(shader->program, uniform);
	if (loc == -1) {
		printf("Couldn't find uniform: %s.\n", uniform);
		return -1;
	}
	return loc;
}

// compile a full shader program
static void shader_load(GL *gl, ShaderBase *shader, char const *vertex_filename, char const *fragment_filename) {
#if DEBUG
	str_cpy(shader->vertex_filename, sizeof shader->vertex_filename, vertex_filename);
	str_cpy(shader->fragment_filename, sizeof shader->fragment_filename, fragment_filename);
	shader->last_modified = shader_get_last_modified(shader);

	if (shader->program) gl->DeleteProgram(shader->program);
#endif

	
	GLuint vertex_shader = shader_compile_from_file(gl, vertex_filename, GL_VERTEX_SHADER);
	GLuint fragment_shader = shader_compile_from_file(gl, fragment_filename, GL_FRAGMENT_SHADER);
	GLuint program = gl->CreateProgram();
	gl->AttachShader(program, vertex_shader);
	gl->AttachShader(program, fragment_shader);
	gl->LinkProgram(program);
	char log[4096] = {0};
	gl->GetProgramInfoLog(program, sizeof log - 1, NULL, log);
	if (*log) {
		logln("Error linking shader:\n%s", log);
		gl->DeleteProgram(program);
		program = 0;
	}
	gl->DeleteShader(vertex_shader);
	gl->DeleteShader(fragment_shader);
	shader->program = program;

	GLenum err = glGetError();
	if (err) {
		logln("Error loading shader %s/%s: %u.", vertex_filename, fragment_filename, err);
	} else {
		logln("Loaded shader %s/%s as %u.", vertex_filename, fragment_filename, program);
	}
}

static void shader_start_using(GL *gl, ShaderBase *shader) {
	gl->UseProgram(shader->program);
}

static void shader_stop_using(GL *gl) {
	gl->UseProgram(0);
}

#if DEBUG
static bool shader_needs_reloading(ShaderBase *shader) {
	return !timespec_eq(shader->last_modified, shader_get_last_modified(shader));
}
#endif

static void shader_platform_load(GL *gl, ShaderPlatform *shader) {
	shader_load(gl, &shader->base, "assets/platform_v.glsl", "assets/platform_f.glsl");
	shader->vertex_p1 = shader_attrib_location(gl, &shader->base, "vertex_p1");
	shader->vertex_p2 = shader_attrib_location(gl, &shader->base, "vertex_p2");
	shader->uniform_thickness = shader_uniform_location(gl, &shader->base, "thickness");
	// @TODO: transform matrix
}

static void shaders_load(State *state) {
	GL *gl = &state->gl;
	shader_platform_load(gl, &state->shader_platform);
}

#if DEBUG
static void shaders_reload_if_necessary(State *state) {
	GL *gl = &state->gl;
	if (shader_needs_reloading(&state->shader_platform.base))
		shader_platform_load(gl, &state->shader_platform);
}
#endif


static void platforms_render(State *state, Platform *platforms, u32 nplatforms) {
	GL *gl = &state->gl;
	ShaderPlatform *shader = &state->shader_platform;
	glColor3f(1,0,1);
	float thickness = 0.01f;
	shader_start_using(gl, &shader->base);
	gl->Uniform1f(shader->uniform_thickness, thickness);
	glBegin(GL_QUADS);
	for (Platform *platform = platforms, *end = platform + nplatforms; platform != end; ++platform) {
		// calculate endpoints of platform
		maybe_unused float radius = platform->size * 0.5f;
		v2 endpoint1 = v2_add(platform->center, v2_polar(radius, platform->angle));
		v2 endpoint2 = v2_sub(platform->center, v2_polar(radius, platform->angle));

	#if 1
		v2 r = v2_polar(thickness, platform->angle - HALF_PIf);
		gl->VertexAttrib2f(shader->vertex_p1, endpoint1.x, endpoint1.y);
		gl->VertexAttrib2f(shader->vertex_p2, endpoint2.x, endpoint2.y);
		v2_gl_vertex(v2_sub(endpoint1, r));
		v2_gl_vertex(v2_sub(endpoint2, r));
		v2_gl_vertex(v2_add(endpoint2, r));
		v2_gl_vertex(v2_add(endpoint1, r));
	#else
		v2_gl_vertex(endpoint1);
		v2_gl_vertex(endpoint2);
	#endif
	}
	glEnd();
	shader_stop_using(gl);
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
	GL *gl = &state->gl;
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

		void (*(*get_gl_proc)(char const *))(void) = frame->get_gl_proc;

		// get GL functions
	#define required_gl_proc(name) if (!(gl->name = (GL ## name)get_gl_proc("gl" #name))) { printf("Couldn't get GL proc: %s.\n", #name); exit(-1); }
	#define optional_gl_proc(name) gl->name = (GL ## name)get_gl_proc("gl" #name)
		required_gl_proc(AttachShader);
		required_gl_proc(CompileShader);
		required_gl_proc(CreateProgram);
		required_gl_proc(CreateShader);
		required_gl_proc(DeleteProgram);
		required_gl_proc(DeleteShader);
		required_gl_proc(GetAttribLocation);
		required_gl_proc(GetProgramInfoLog);
		required_gl_proc(GetProgramiv);
		required_gl_proc(GetShaderInfoLog);
		required_gl_proc(GetShaderiv);
		required_gl_proc(GetUniformLocation);
		required_gl_proc(LinkProgram);
		required_gl_proc(ShaderSource);
		required_gl_proc(Uniform1f);
		required_gl_proc(Uniform2f);
		required_gl_proc(Uniform3f);
		required_gl_proc(Uniform4f);
		required_gl_proc(Uniform1i);
		required_gl_proc(Uniform2i);
		required_gl_proc(Uniform3i);
		required_gl_proc(Uniform4i);
		required_gl_proc(UniformMatrix4fv);
		required_gl_proc(UseProgram);
		required_gl_proc(VertexAttrib1f);
		required_gl_proc(VertexAttrib2f);
		required_gl_proc(VertexAttrib3f);
		required_gl_proc(VertexAttrib4f);
	#undef optional_gl_proc
	#undef required_gl_proc

		shaders_load(state);
		

		state->nplatforms = 1;
		Platform *p = &state->platforms[0];
		p->center = V2(0.5f, 0.5f);
		p->angle  = 0;//PIf * 0.3f;
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

#if DEBUG
	shaders_reload_if_necessary(state);
#endif

	platforms_render(state, state->platforms, state->nplatforms);

	#if DEBUG
	GLuint error = glGetError();
	if (error) {
		printf("!!! GL ERROR: %u\n", error);
	}
	#endif
}
