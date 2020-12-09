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
	m4 transform = state->transform;

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

		gl_rgbacolor(platform->color);

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

	if (state->building) {
		// show arrows for platforms
		for (Platform *platform = platforms, *end = platform + nplatforms; platform != end; ++platform) {
			gl_rgbacolor(platform->color);
			if (platform->rotates) {
				float speed = platform->rotate_speed;
				float angle = speed * 0.5f;
				if (angle) {
					float theta1 = HALF_PIf;
					float theta2 = theta1 + angle;
					float dtheta = 0.03f * sgnf(angle);
					float radius = platform->size;
					glBegin(GL_LINE_STRIP);
					for (float theta = theta1; angle > 0 ? (theta < theta2) : (theta > theta2); theta += dtheta) {
						v3 point = v3_from_v2(v2_add(platform->center, v2_polar(radius, theta)));
						point = m4_mul_v3(transform, point);
						v3_gl_vertex(point);
					}
					glEnd();

					v3 p1 = v3_from_v2(v2_add(platform->center, v2_polar(radius-0.2f, theta2-0.1f * sgnf(angle))));
					v3 p2 = v3_from_v2(v2_add(platform->center, v2_polar(radius, theta2)));
					v3 p3 = v3_from_v2(v2_add(platform->center, v2_polar(radius+0.2f, theta2-0.1f * sgnf(angle))));

					p1 = m4_mul_v3(transform, p1);
					p2 = m4_mul_v3(transform, p2);
					p3 = m4_mul_v3(transform, p3);
					glBegin(GL_LINE_STRIP);
					v3_gl_vertex(p1);
					v3_gl_vertex(p2);
					v3_gl_vertex(p3);
					glEnd();
				}
			}
		}
	}
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

static uintptr_t platform_to_user_data(State *state, Platform *platform) {
	return USER_DATA_PLATFORM | (uintptr_t)(platform - state->platforms);
}

static Platform *platform_from_user_data(State *state, uintptr_t user_data) {
	if ((user_data & USER_DATA_TYPE) == USER_DATA_PLATFORM) {
		uintptr_t index = user_data & USER_DATA_INDEX;
		return &state->platforms[index];
	} else {
		return NULL;
	}
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
	fixture.userData.pointer = platform_to_user_data(state, platform);

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

class PlatformQueryCallback : public b2QueryCallback {
public:
	PlatformQueryCallback(State *state_) {
		this->state = state_;
	}
	bool ReportFixture(b2Fixture *fixture) {
		platform = platform_from_user_data(state, fixture->GetUserData().pointer);
		return !platform; // if we haven't found a platform, keep going
	}
	Platform *platform = NULL;
	State *state = NULL;
};

static Platform *platform_at_mouse_pos(State *state) {
	v2 mouse_pos = state->mouse_pos;
	v2 mouse_radius = V2(0.3f, 0.3f); // you don't have to hover exactly over a platform to select it. this is the tolerance.
	v2 a = v2_sub(mouse_pos, mouse_radius);
	v2 b = v2_add(mouse_pos, mouse_radius);
	PlatformQueryCallback callback(state);
	b2AABB aabb;
	aabb.lowerBound = v2_to_b2(a);
	aabb.upperBound = v2_to_b2(b);
	state->world->QueryAABB(&callback, aabb);
	return callback.platform;
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

		for (Platform *platform = state->platforms, *end = platform + state->nplatforms; platform != end; ++platform) {
			platform->angle = platform->body->GetAngle();
			v2 pos = b2_to_v2(platform->body->GetPosition());
			platform->center = pos;

			if (platform->moves) {
				// check if the platform has reached the other endpoint; if so, set it going in the other direction
				v2 p1 = platform->move_p1, p2 = platform->move_p2;
				v2 vel = b2_to_v2(platform->body->GetLinearVelocity());
				bool switch_direction = false;

				if (vel.x > 0) {
					if (pos.x > maxf(p1.x, p2.x))
						switch_direction = true;
				} else if (vel.x < 0) {
					if (pos.x < minf(p1.x, p2.x))
						switch_direction = true;
				}
				if (vel.y > 0) {
					if (pos.y > maxf(p1.y, p2.y))
						switch_direction = true;
				} else if (vel.y < 0) {
					if (pos.y < minf(p1.y, p2.y))
						switch_direction = true;
				}

				if (switch_direction) {
					v2 new_vel = v2_scale(vel, -1); // flip the velocity
					platform->body->SetLinearVelocity(v2_to_b2(new_vel));
				}
				
			}
		}

		dt -= time_step;
	}
	state->time_residue = dt;
}

static void platform_delete(State *state, Platform *platform) {
	Platform *platforms = state->platforms;
	u32 nplatforms = state->nplatforms;
	u32 index = (u32)(platform - platforms);
	
	state->world->DestroyBody(platforms[index].body);

	if (index+1 < nplatforms) {
		// set this platform to last platform
		platforms[index] = platforms[nplatforms-1];
		memset(&platforms[nplatforms-1], 0, sizeof(Platform));
		platforms[index].body->GetFixtureList()->GetUserData().pointer = platform_to_user_data(state, &platforms[index]);
	} else {
		// platform is at end of array; don't need to do anything special
		memset(&platforms[index], 0, sizeof(Platform));
	}
	--state->nplatforms;

}

static void correct_mouse_button(State *state, u8 *button) {
	if (*button == MOUSE_LEFT) {
		if (state->shift) {
			// shift+left mouse = right mouse
			*button = MOUSE_RIGHT;
		} else if (state->ctrl) {
			// ctrl+left mouse = middle mouse
			*button = MOUSE_MIDDLE;
		}
	}
}

static void ball_reset(State *state) {
	Ball *ball = &state->ball;
	b2World *world = state->world;
	if (ball->body)
		world->DestroyBody(ball->body);


	ball->radius = 0.3f;
	ball->pos = V2(0, 10.0f);

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
	ball_fixture.restitution = 0.3f; // bounciness

	ball_body->CreateFixture(&ball_fixture);
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
	state->ctrl = input->keys_down[KEY_LCTRL] || input->keys_down[KEY_RCTRL];
	state->shift = input->keys_down[KEY_LSHIFT] || input->keys_down[KEY_RSHIFT];

	for (u32 i = 0; i < input->nmouse_presses; ++i) {
		MousePress *p = &input->mouse_presses[i];
		correct_mouse_button(state, &p->button);
	}
	for (u32 i = 0; i < input->nmouse_releases; ++i) {
		MouseRelease *r = &input->mouse_releases[i];
		correct_mouse_button(state, &r->button);
	}

	state->win_width  = (float)width;
	state->win_height = (float)height;
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
		state->left_x   = -3.0f;

		text_font_load(state, &state->font, "assets/font.ttf", 36.0f);

		b2Vec2 gravity(0, -9.81f);
		b2World *world = state->world = new b2World(gravity);
			
		// create ground
		b2BodyDef ground_body_def;
		ground_body_def.position.Set(0.0f, -1000.0f);
		b2Body *ground_body = world->CreateBody(&ground_body_def);

		b2PolygonShape ground_shape;
		ground_shape.SetAsBox(50.0f, 10.0f);
		ground_body->CreateFixture(&ground_shape, 0.0f);

		// create left wall
		b2BodyDef left_wall_def;
		left_wall_def.position.Set(state->left_x - 0.5f, 0);
		b2Body *left_wall_body = world->CreateBody(&left_wall_def);
		b2PolygonShape left_wall_shape;
		left_wall_shape.SetAsBox(0.5f, 1000);
		left_wall_body->CreateFixture(&left_wall_shape, 0);

		ball_reset(state);
		
		{ // initialize platforms
			Platform *p = &state->platforms[0];
			p->center = V2(-1.0f, 5.0f);
			p->angle  = 0;
			p->size   = 1.0f;
			p->color = 0xFF00FFFF;
			platform_make_body(state, p);
			state->nplatforms = (u32)(p - state->platforms + 1);

			Platform *b = &state->platform_building;
			b->size = 3.0f;
			b->color = 0xFF00FF7F;
		}
		
		state->building = true;
			
		state->initialized = true;
	#if DEBUG
		state->magic_number = MAGIC_NUMBER;
	#endif
	}
	if (keys_pressed[KEY_ESCAPE]) {
		frame->close = true;
		return;
	}

#if DEBUG
	shaders_reload_if_necessary(state);
#endif

	{
		float half_height = 10.0f;
		float half_width = half_height * state->win_width / state->win_height;
		float ball_x = ball->pos.x, ball_y = ball->pos.y;
		// center view around ball
		state->transform = m4_ortho(ball_x - half_width, ball_x + half_width, ball_y - half_height, ball_y + half_height, -1, +1);
		state->inv_transform = m4_inv(state->transform);
	}

	{ // calculate mouse position in Box2D coordinates
		v3 mouse_gl_coords = V3(
			(float)input->mouse_x / state->win_width * 2 - 1,
			(1 - (float)input->mouse_y / state->win_height) * 2 - 1,
			0
		);

		state->mouse_pos = v3_xy(m4_mul_v3(state->inv_transform, mouse_gl_coords));
	}

	Font *font = &state->font;
	Platform *mouse_platform = platform_at_mouse_pos(state);

	if (state->simulating) {
		// simulate physics
		float dt = state->dt;
		if (dt > 100) dt = 100; // prevent floating-point problems for very large dt's
		simulate_time(state, dt);
		if (keys_pressed[KEY_SPACE]) {
			state->building = true;
			state->simulating = false;
			keys_pressed[KEY_SPACE] = 0;
			ball_reset(state);
		}
	}

	if (state->building) {
		Platform *platform_building = &state->platform_building;
		platform_building->center = state->mouse_pos;
		float dt = state->dt;
		float rotate_amount = 2.0f * dt;
		float size_change_amount = 4.0f * dt;
		// rotate platform using left/right
		if (keys_down[KEY_LEFT])
			platform_building->angle += rotate_amount;
		if (keys_down[KEY_RIGHT])
			platform_building->angle -= rotate_amount;
		
		// change size of platform using up/down
		if (keys_down[KEY_UP])
			platform_building->size += size_change_amount;
		if (keys_down[KEY_DOWN])
			platform_building->size -= size_change_amount;

		if (platform_building->rotates) {
			// change rotation speed
			float rotate_change_amount = 2.0f * dt;
			if (keys_down[KEY_Q])
				platform_building->rotate_speed += rotate_change_amount;
			if (keys_down[KEY_E])
				platform_building->rotate_speed -= rotate_change_amount;
		}

		platform_building->rotate_speed = clampf(platform_building->rotate_speed, -3, +3);
		platform_building->size = clampf(platform_building->size, 0.3f, 10.0f);
		platform_building->angle = fmodf(platform_building->angle, TAUf);

		if (keys_pressed[KEY_R]) {
			// toggle rotating platform
			bool rotates = platform_building->rotates = !platform_building->rotates;
			if (rotates) {
				if (platform_building->rotate_speed == 0) {
					platform_building->rotate_speed = 1;
				}
			} else {
				platform_building->rotate_speed = 0;
			}
		}

		for (u32 i = 0; i < input->nmouse_presses; ++i) {
			MousePress *press = &input->mouse_presses[i];
			u8 button = press->button;

			if (button == MOUSE_LEFT) {
				if (mouse_platform) {
					*platform_building = *mouse_platform;
					platform_delete(state, mouse_platform);
					platform_building->color = (platform_building->color & 0xFFFFFF00) | 0x7F;
				} else {
					// left-click to build platform
					if (state->nplatforms < MAX_PLATFORMS) {
						Platform *p = &state->platforms[state->nplatforms++];
						*p = *platform_building;
						p->color |= 0xFF; // set alpha to 255
						platform_make_body(state, p);
					}
				}
			} else if (button == MOUSE_RIGHT) {
				if (mouse_platform) {
					// right-click to delete platform
					platform_delete(state, mouse_platform);
				}
			}
		}

		if (keys_pressed[KEY_SPACE]) {
			state->building = false;
			state->simulating = true;
		}
	}


	u32 prev_mouse_platform_color = mouse_platform ? mouse_platform->color : 0;
	if (state->building) {
		// turn platform under mouse blue
		if (mouse_platform) mouse_platform->color = 0x007FFFFF;
	}
	platforms_render(state, state->platforms, state->nplatforms);
	if (state->building) {
		if (mouse_platform) {
			mouse_platform->color = prev_mouse_platform_color;
		} else {
			platforms_render(state, &state->platform_building, 1);
		}
	}
	ball_render(state);

	{
		float bottom_y = m4_mul_v3(state->transform, V3(0, state->bottom_y, 0)).y;
		float left_x = m4_mul_v3(state->transform, V3(state->left_x, 0, 0)).x;

		glBegin(GL_LINES);
		glColor3f(1,0,0);
		// render floor line
		glVertex2f(-1, bottom_y);
		glVertex2f(+1, bottom_y);

		// render left wall line
		glColor3f(0.5f,0.5f,0.5f);
		glVertex2f(left_x, bottom_y);
		glVertex2f(left_x, +1);
		glEnd();

		glBegin(GL_QUADS);
		// floor area
		glColor4f(1,0,0,0.2f);
		glVertex2f(-1, bottom_y);
		glVertex2f(-1, -1);
		glVertex2f(+1, -1);
		glVertex2f(+1, bottom_y);
		// left wall area
		glColor4f(0.5f, 0.5f, 0.5f, 0.2f);
		glVertex2f(-1, +1);
		glVertex2f(left_x, +1);
		glVertex2f(left_x, bottom_y);
		glVertex2f(-1, bottom_y);
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
