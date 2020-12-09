#include "gui.hpp"
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
#include "math.cpp"
#include "sim.hpp"
#include "time.cpp"
#include "util.cpp"
#include "base.cpp"
#include "text.cpp"

static b2Vec2 v2_to_b2(v2 v) {
	return b2Vec2(v.x, v.y);
}

static v2 b2_to_v2(b2Vec2 v) {
	return V2(v.x, v.y);
}

// compile a vertex or fragment shader
static GLuint shader_compile_from_file(GL *gl, char const *filename, GLenum shader_type) {
	FILE *fp = fopen(filename, "rb");
	if (fp) {
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
		logln("File does not exist: %s.", filename);
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
	ShaderBase *base = &shader->base;
	shader_load(gl, base, "assets/platform_v.glsl", "assets/platform_f.glsl");
	shader->vertex_p1 = shader_attrib_location(gl, base, "vertex_p1");
	shader->vertex_p2 = shader_attrib_location(gl, base, "vertex_p2");
	shader->uniform_thickness = shader_uniform_location(gl, base, "thickness");
	shader->uniform_transform = shader_uniform_location(gl, base, "transform");
}

static void shader_ball_load(GL *gl, ShaderBall *shader) {
	ShaderBase *base = &shader->base;
	shader_load(gl, base, "assets/ball_v.glsl", "assets/ball_f.glsl");
	shader->uniform_transform = shader_uniform_location(gl, base, "transform");
	shader->uniform_center = shader_uniform_location(gl, base, "center");
	shader->uniform_radius = shader_uniform_location(gl, base, "radius");
}

static void shaders_load(State *state) {
	GL *gl = &state->gl;
	shader_platform_load(gl, &state->shader_platform);
	shader_ball_load(gl, &state->shader_ball);
}

#if DEBUG
static void shaders_reload_if_necessary(State *state) {
	GL *gl = &state->gl;
	if (shader_needs_reloading(&state->shader_platform.base))
		shader_platform_load(gl, &state->shader_platform);
	if (shader_needs_reloading(&state->shader_ball.base))
		shader_ball_load(gl, &state->shader_ball);
}
#endif

// render the given platforms
static void platforms_render(State *state, Platform *platforms, u32 nplatforms) {
	GL *gl = &state->gl;
	ShaderPlatform *shader = &state->shader_platform;
	float platform_render_thickness = state->platform_thickness;

	shader_start_using(gl, &shader->base);
	
	gl->Uniform1f(shader->uniform_thickness, platform_render_thickness);
	gl->UniformMatrix4fv(shader->uniform_transform, 1, GL_FALSE, state->transform.e);

	glBegin(GL_QUADS);
	glColor3f(1,0,1);
	for (Platform *platform = platforms, *end = platform + nplatforms; platform != end; ++platform) {
		float radius = platform->size * 0.5f;
		v2 center = platform->center;
		v2 thickness_r = v2_polar(platform_render_thickness, platform->angle - HALF_PIf);
		v2 platform_r = v2_polar(radius, platform->angle);
		v2 endpoint1 = v2_add(center, platform_r);
		v2 endpoint2 = v2_sub(center, platform_r);

	#if 1
		gl->VertexAttrib2f(shader->vertex_p1, endpoint1.x, endpoint1.y);
		gl->VertexAttrib2f(shader->vertex_p2, endpoint2.x, endpoint2.y);
		v2_gl_vertex(v2_sub(endpoint1, thickness_r));
		v2_gl_vertex(v2_sub(endpoint2, thickness_r));
		v2_gl_vertex(v2_add(endpoint2, thickness_r));
		v2_gl_vertex(v2_add(endpoint1, thickness_r));
	#else
		v2_gl_vertex(endpoint1);
		v2_gl_vertex(endpoint2);
	#endif
	}
	glEnd();
	shader_stop_using(gl);
}

// render the ball
static void ball_render(State *state) {
	GL *gl = &state->gl;
	Ball *ball = &state->ball;
	float ball_x = ball->pos.x, ball_y = ball->pos.y;
	float ball_r = ball->radius;
	ShaderBall *shader = &state->shader_ball;
	
	shader_start_using(gl, &shader->base);
	
	gl->UniformMatrix4fv(shader->uniform_transform, 1, GL_FALSE, state->transform.e);
	gl->Uniform2f(shader->uniform_center, ball_x, ball_y);
	gl->Uniform1f(shader->uniform_radius, ball_r);

	glBegin(GL_QUADS);
	glColor3f(1,1,1);
	glVertex2f(ball_x-ball_r, ball_y-ball_r);
	glVertex2f(ball_x-ball_r, ball_y+ball_r);
	glVertex2f(ball_x+ball_r, ball_y+ball_r);
	glVertex2f(ball_x+ball_r, ball_y-ball_r);
	glEnd();
	shader_stop_using(gl);
}


// sets platform->body to a new Box2D body.
static void platform_make_body(State *state, Platform *platform) {
	b2World *world = state->world;

	float half_size = platform->size * 0.5f;
	
	if (platform->moves)
		platform->center = platform->move_p1;

	v2 center = platform->center;

	b2BodyDef body_def;
	body_def.type = b2_kinematicBody;
	body_def.position.Set(center.x, center.y);
	body_def.angle = platform->angle;
	b2Body *body = world->CreateBody(&body_def);

	b2PolygonShape shape;
	shape.SetAsBox(half_size, state->platform_thickness);

	b2FixtureDef fixture;
	fixture.shape = &shape;
	fixture.friction = 0.8f;

	body->CreateFixture(&fixture);
	if (platform->moves) {
		float speed = platform->move_speed;
		v2 p1 = platform->move_p1, p2 = platform->move_p2;
		v2 direction = v2_normalize(v2_sub(p2, p1));
		v2 velocity = v2_scale(direction, speed);
		body->SetLinearVelocity(v2_to_b2(velocity));
	} 
	body->SetAngularVelocity(platform->rotate_speed);

	platform->body = body;
}

static void simulate_time(State *state, float dt) {
	float time_step = 0.01f; // fixed time step
	dt += state->time_residue;
	while (dt >= time_step) {
		Ball *ball = &state->ball;
		b2World *world = state->world;

		world->Step(time_step, 8, 3); // step using recommended parameters

		if (ball->body) {
			b2Vec2 ball_pos = ball->body->GetPosition();

			if (ball_pos.y - ball->radius < state->bottom_y) {
				// oh no! ball reached bottom line
				world->DestroyBody(ball->body);
				ball->body = NULL;
				ball->pos.y = state->bottom_y + ball->radius;
			} else {
				ball->pos = b2_to_v2(ball_pos);
			}
		}

		dt -= time_step;
	}
	state->time_residue = dt;
}

#ifdef __cplusplus
extern "C"
#endif
#ifdef _WIN32
__declspec(dllexport)
#endif
void sim_frame(Frame *frame) {
	if (frame->memory_size < sizeof(State)) {
		printf("Not enough memory (got %lu, require %lu).\n", (ulong)frame->memory_size, (ulong)sizeof(State));
		frame->close = true;
		return;
	}
	State *state = (State *)frame->memory;
	Ball *ball = &state->ball;
	i32 width = frame->width, height = frame->height;
	Input *input = &frame->input;
	GL *gl = &state->gl;
	maybe_unused u8 *keys_pressed = input->keys_pressed;
	maybe_unused bool *keys_down = input->keys_down;

	state->win_width  = width;
	state->win_height = height;
	state->dt = (float)frame->dt;
	

	// set up GL
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, width, height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	
#if DEBUG
	if (state->magic_number != MAGIC_NUMBER || keys_pressed[KEY_F5]) {
		memset(state, 0, sizeof *state);
	}
#endif

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
		
		state->platform_thickness = 0.05f;
		state->bottom_y = 0.1f;

		text_font_load(state, &state->font, "assets/font.ttf", 36.0f);

		ball->radius = 0.3f;
		ball->pos = V2(0, 10.0f);

		b2Vec2 gravity(0, -9.81f);
		b2World *world = state->world = new b2World(gravity);
			
		// create ground
		b2BodyDef ground_body_def;
		ground_body_def.position.Set(0.0f, -1000.0f);
		b2Body *ground_body = world->CreateBody(&ground_body_def);

		b2PolygonShape ground_shape;
		ground_shape.SetAsBox(50.0f, 10.0f);
		ground_body->CreateFixture(&ground_shape, 0.0f);

		// create ball
		b2BodyDef ball_def;
		ball_def.type = b2_dynamicBody;
		ball_def.position.Set(ball->pos.x, ball->pos.y);
		b2Body *ball_body = ball->body = world->CreateBody(&ball_def);
		
		b2CircleShape ball_shape;
		ball_shape.m_radius = ball->radius;

		b2FixtureDef ball_fixture;
		ball_fixture.shape = &ball_shape;
		ball_fixture.density = 1.0f;
		ball_fixture.friction = 0.3f;
		ball_fixture.restitution = 0.9f; // bounciness

		ball_body->CreateFixture(&ball_fixture);
		
		{ // initialize platforms
			state->nplatforms = 2;
			Platform *p = &state->platforms[0];
			p->center = V2(0, 0.8f);
			p->angle  = 0;
			p->size   = 9.0f;
			p->rotate_speed = 1.0f;
			platform_make_body(state, p);
			++p;
			p->moves = true;
			p->move_p1 = V2(0, 0.5f);
			p->move_p2 = V2(0, 5.5f);
			p->move_speed = 1.0f;
			p->angle  = PIf * 0.54f;
			p->size   = 6.0f;
			platform_make_body(state, p);
		}

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

	maybe_unused b2World *world = state->world;
	Font *font = &state->font;

	// simulate physics
	{
		float dt = state->dt;
		if (dt > 100) dt = 100; // prevent floating-point problems for very large dt's
		simulate_time(state, dt);
		for (Platform *platform = state->platforms, *end = platform + state->nplatforms; platform != end; ++platform) {
			b2Vec2 platform_pos = platform->body->GetPosition();
			platform->center = b2_to_v2(platform_pos);
			platform->angle = platform->body->GetAngle();
		}
	}

	{
		float half_height = 10.0f;
		float half_width = half_height * (float)state->win_width / (float)state->win_height;
		float ball_x = ball->pos.x, ball_y = ball->pos.y;
		// center view around ball
		state->transform = m4_ortho(ball_x - half_width, ball_x + half_width, ball_y - half_height, ball_y + half_height, -1, +1);
	}

	platforms_render(state, state->platforms, state->nplatforms);
	ball_render(state);

	{
		v3 line_pos = V3(0, state->bottom_y, 0);
		line_pos = m4_mul_v3(state->transform, line_pos);
		glBegin(GL_LINES);
		glColor3f(1,0,0);
		glVertex2f(-1, line_pos.y);
		glVertex2f(+1, line_pos.y);
		glEnd();
		glBegin(GL_QUADS);
		glColor4f(1,0,0,0.1f);
		glVertex2f(-1, line_pos.y);
		glVertex2f(-1, -1);
		glVertex2f(+1, -1);
		glVertex2f(+1, line_pos.y);
		glEnd();
	}

	{
		char x_text[64] = {0}, y_text[64] = {0};
		glColor3f(0.8f,0.5f,1);
		snprintf(x_text, sizeof x_text - 1, "x: %.2f m", ball->pos.x);
		snprintf(y_text, sizeof y_text - 1, "y: %.2f m", ball->pos.y);
		v2 x_size = text_get_size(state, font, x_text);
		v2 y_size = text_get_size(state, font, y_text);
		v2 pos = V2(0.95f, 0.95f);
		text_render(state, font, x_text, v2_sub(pos, x_size));
		pos.y -= x_size.y;
		text_render(state, font, y_text, v2_sub(pos, y_size));
		pos.y -= y_size.y;
	}

	#if DEBUG
	GLuint error = glGetError();
	if (error) {
		printf("!!! GL ERROR: %u\n", error);
	}
	#endif
}
