// @TODO: test read/write setups
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

#define BALL_STARTING_X 3.0f

static b2Vec2 v2_to_b2(v2 v) {
	return b2Vec2(v.x, v.y);
}

static v2 b2_to_v2(b2Vec2 v) {
	return V2(v.x, v.y);
}

// converts Box2D coordinates to GL coordinates
static v2 b2_to_gl(State const *state, v2 box2d_coordinates) {
	v3 v = m4_mul_v3(state->transform, v3_from_v2(box2d_coordinates));
	return V2(v.x, v.y);
}

#include "shaders.cpp"
#include "platforms.cpp"

static void simulate_time(State *state, float dt) {
	Ball *ball = &state->ball;
	if (!ball->body) return; // we're done simulating
	float time_step = 0.01f; // fixed time step
	dt += state->time_residue;
	while (dt >= time_step) {
		b2World *world = state->world;

		world->Step(time_step, 8, 3); // step using recommended parameters

		{ // update ball
			state->stuck_time += time_step;
			state->total_time += time_step;
			b2Vec2 ball_pos = ball->body->GetPosition();
			
			bool reached_bottom = ball_pos.y - ball->radius < state->bottom_y; // ball reached bottom line
			float max_stuck_time = 10;
			bool stuck = state->stuck_time > max_stuck_time; // ball hasn't gotten any further in a while. it's over
			if (reached_bottom || stuck) {
				world->DestroyBody(ball->body);
				ball->body = NULL;
				if (reached_bottom) {
					// place ball on ground
					ball->pos.y = state->bottom_y + ball->radius;
				}
				if (stuck)
					state->stuck_time = max_stuck_time;
				return; // done simulating
			} else {
				ball->pos = b2_to_v2(ball_pos);
				float rounded_pos = 0.01f * roundf(ball->pos.x * 100);
				float rounded_record = 0.01f * roundf(state->furthest_ball_x_pos * 100);
				if (rounded_pos > rounded_record) { // only update record if centimeter reading will change
					state->furthest_ball_x_pos = ball->pos.x;
					state->stuck_time = 0;
				}
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

#include "setup.cpp"

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
	i32 width = frame->width, height = frame->height;
	Input *input = &frame->input;
	maybe_unused u8 *keys_pressed = input->keys_pressed;
	maybe_unused bool *keys_down = input->keys_down;
	State *state = (State *)frame->memory;
#if DEBUG
	if (state->magic_number != MAGIC_NUMBER || keys_pressed[KEY_F5]) {
		memset(state, 0, sizeof *state);
	}
#endif
	Ball *ball = &state->ball;
	GL *gl = &state->gl;
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
	state->aspect_ratio = state->win_width / state->win_height;
	state->dt = (float)frame->dt;
	
	if (width == 0 || height == 0) return;

	// set up GL
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, width, height);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
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

		make_directory("setups");

		shaders_load(state);
		
		state->platform_thickness = 0.05f;
		state->bottom_y = 0.1f;
		state->left_x   = 0;

		text_font_load(state, &state->font, "assets/font.ttf", 36.0f);
		text_font_load(state, &state->small_font, "assets/font.ttf", 18.0f);

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

		for (size_t i = 0; i < arr_count(state->setups); ++i) {
			// randomize initial setups
			Setup *setup = &state->setups[i];
			setup_random(state, setup);
			setup_score(state, setup);
		}

		qsort(state->setups, arr_count(state->setups), sizeof(Setup), setup_compare_scores);

		for (size_t i = 0; i < TOP_KEPT; ++i) {
			printf("%zu. %f\n", i, state->setups[i].score);
		}

		for (int g = 0; g < 100; ++g) {
			printf("---Generation %d---\n",g);
			for (size_t i = 0; i < GENERATION_SIZE; ++i) {
				// create new generation from TOP_KEPT
				Setup *setup = &state->setups[i + TOP_KEPT];
				*setup = state->setups[rand() % TOP_KEPT]; // select one of the top setups to mutate from
				setup->mutated = true;
				switch (i / 20) {
				case 0: setup_mutate(state, setup, 0.05f); break; // 5% mutation rate group
				case 1: setup_mutate(state, setup, 0.10f); break; // 10% mutation rate group
				case 2: setup_mutate(state, setup, 0.20f); break; // 20% mutation rate group
				case 3: setup_mutate(state, setup, 0.30f); break; // 30% mutation rate group
				case 4: memset(setup, 0, sizeof *setup); setup_random(state, setup); break; // completely random group
				}
				setup_score(state, setup);
			}

			qsort(state->setups, arr_count(state->setups), sizeof(Setup), setup_compare_scores);

			for (size_t i = 0; i < TOP_KEPT; ++i) {
				Setup *setup = &state->setups[i];
				char filename[64] = {0};
				snprintf(filename, sizeof filename - 1, "setups/%03zu.b2s", i);
				printf("%zu. %f%s\n", i, setup->score, setup->mutated ? " (mutated)" : "");
				setup_write_to_file(setup, filename);
			}

			setup_use(state, &state->setups[0]);
		}

		#if 0
		Setup *best_setup = &state->generation[0];
		for (size_t i = 0; i < GENERATION_SIZE; ++i) {
			Setup *setup = &state->generation[i];
			setup_random(state, setup);
			char filename[64] = {0};
			snprintf(filename, sizeof filename-1, "setups/%03zu.b2s", i);
			setup_write_to_file(setup, filename);
			setup_score(state, setup);
			printf("Setup %zu: %.2f m\n", i, setup->score);
			if (setup->score > best_setup->score) {
				best_setup = setup;
			}
		}
		printf("Best: setup %ld\n", (long)(best_setup - state->generation));
		setup_use(state, best_setup);
		assert(state->nplatforms == best_setup->nplatforms);
		#endif

		{
			Platform *b = &state->platform_building;
			b->radius = 1.5f;
			b->color = 0xFF00FF7F;
		}

		setup_reset(state);

		state->pan = ball->pos;
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


	Font *font = &state->font;
	Font *small_font = &state->small_font;
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
			setup_reset(state);
		}
	}


	{
		float half_height = 10.0f;
		float half_width = half_height * state->win_width / state->win_height;
		float view_x = ball->pos.x, view_y = ball->pos.y;
		if (state->building) {
			// pan = center of view
			view_x = state->pan.x;
			view_y = state->pan.y;
		}
		// center view around (view_x, view_y)
		state->transform = m4_ortho(view_x - half_width, view_x + half_width, view_y - half_height, view_y + half_height, -1, +1);
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

	if (state->building) {
		Platform *platform_building = &state->platform_building;

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

		if (keys_pressed[KEY_M]) {
			// toggle moving platform
			bool moves = platform_building->moves = !platform_building->moves;
			if (moves) {
				platform_building->move_p1 = platform_building->center;
				platform_building->move_speed = 1.0f;
				state->setting_move_p2 = true;
			} else {
				state->setting_move_p2 = false;
			}
		}

		if (keys_pressed[KEY_B]) {
			// pan back to ball
			state->pan = ball->pos;
		}

		if (state->setting_move_p2) {
			platform_building->move_p2 = state->mouse_pos;
		} else {
			platform_building->center = state->mouse_pos;
			platform_building->move_p1 = platform_building->center;
		}
		float dt = state->dt;
		float rotate_amount = 2.0f * dt;
		float radius_change_amount = 2.0f * dt;
		// rotate platform using left/right
		if (keys_down[KEY_LEFT]) {
			platform_building->angle += rotate_amount;
		}
		if (keys_down[KEY_RIGHT]) {
			platform_building->angle -= rotate_amount;
		}

		// change size of platform using up/down
		if (keys_down[KEY_UP])
			platform_building->radius += radius_change_amount;
		if (keys_down[KEY_DOWN])
			platform_building->radius -= radius_change_amount;

		// pan
		float pan_amount = 5.0f * dt;
		v2 *pan = &state->pan;
		if (keys_down[KEY_A])
			pan->x -= pan_amount;
		if (keys_down[KEY_D])
			pan->x += pan_amount;
		if (keys_down[KEY_S])
			pan->y -= pan_amount;
		if (keys_down[KEY_W])
			pan->y += pan_amount;


		// change rotation speed
		float rotate_change_amount = 2.0f * dt;
		if (keys_down[KEY_Q]) {
			if (!platform_building->rotates) {
				platform_building->rotates = true;
				platform_building->rotate_speed = 0;
			}
			platform_building->rotate_speed += rotate_change_amount;
		} 
		if (keys_down[KEY_E]) {
			if (!platform_building->rotates) {
				platform_building->rotates = true;
				platform_building->rotate_speed = 0;
			}
			platform_building->rotate_speed -= rotate_change_amount;
		}

		if (platform_building->moves) {
			// change move speed
			float speed_change_amount = 2.0f * dt;
			if (keys_down[KEY_Z])
				platform_building->move_speed -= speed_change_amount;
			if (keys_down[KEY_X])
				platform_building->move_speed += speed_change_amount;
		}

		platform_building->rotate_speed = clampf(platform_building->rotate_speed,
			PLATFORM_ROTATE_SPEED_MIN, PLATFORM_ROTATE_SPEED_MAX);
		platform_building->move_speed = clampf(platform_building->move_speed,
			PLATFORM_MOVE_SPEED_MIN, PLATFORM_MOVE_SPEED_MAX);
		platform_building->radius = clampf(platform_building->radius,
			PLATFORM_RADIUS_MIN, PLATFORM_RADIUS_MAX);
		platform_building->angle = fmodf(platform_building->angle, TAUf);

		for (u32 i = 0; i < input->nmouse_presses; ++i) {
			MousePress *press = &input->mouse_presses[i];
			u8 button = press->button;

			if (button == MOUSE_LEFT) {
				if (mouse_platform) {
					// edit platform
					*platform_building = *mouse_platform;
					platform_building->body = NULL;
					platform_delete(state, mouse_platform);
					platform_building->color = (platform_building->color & 0xFFFFFF00) | 0x7F;
				} else {
					// left-click to build platform
					if (state->nplatforms < MAX_PLATFORMS) {
						platform_building->start_angle = platform_building->angle;
						Platform *p = &state->platforms[state->nplatforms++];
						*p = *platform_building;
						p->color |= 0xFF; // set alpha to 255
						platform_make_body(state, p, state->nplatforms - 1);
						state->setting_move_p2 = false;
						platform_building->moves = false;
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
			setup_reset(state);
		}
	}


	u32 prev_mouse_platform_color = mouse_platform ? mouse_platform->color : 0;
	if (state->building) {
		// turn platform under mouse blue
		if (mouse_platform) mouse_platform->color = 0x007FFFFF;

	#if 0
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixf(state->transform.e);	

		glBegin(GL_QUADS);
		glColor4f(1, 1, 1, 0.5f);
		rect_render(platform_bounding_box(&state->platform_building));
		for (u32 i = 0; i < state->nplatforms; ++i) {
			rect_render(platform_bounding_box(&state->platforms[i]));
		}
		glEnd();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
	#endif
	}
	platforms_render(state, state->platforms, state->nplatforms);
	if (state->building) {
		if (mouse_platform) {
			mouse_platform->color = prev_mouse_platform_color;
		} else {
			platforms_render(state, &state->platform_building, 1);
		#if 1
			{ // show rightmost x coordinate of platform
				glBegin(GL_LINES);
				v2 line_pos = V2(platform_rightmost_x(&state->platform_building), 0);
				float x = b2_to_gl(state, line_pos).x;
				glVertex2f(x, -1);
				glVertex2f(x, +1);
				glEnd();
			}
		#endif
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

		if (state->simulating) { // starting line & distance traveled
			float starting_line = platforms_starting_line(state->platforms, state->nplatforms);
			float starting_line_gl = b2_to_gl(state, V2(starting_line, 0)).x;
			glBegin(GL_LINES);
			glColor3f(1,1,0);
			glVertex2f(starting_line_gl, bottom_y);
			glVertex2f(starting_line_gl, +1);
			glEnd();

			char dist_text[64] = {0};
			if (ball->body)
				glColor4f(0.8f,0.8f,0.8f,0.8f); // still going
			else
				glColor4f(0.5f,1,0.5f,1.0f); // done
			snprintf(dist_text, sizeof dist_text - 1, "Distance: %.2f m", ball->pos.x - starting_line);
			v2 dist_size = text_get_size(state, font, dist_text);

			char best_text[64] = {0};
			snprintf(best_text, sizeof best_text - 1, "Best distance: %.2f m", state->furthest_ball_x_pos - starting_line);
			v2 best_size = text_get_size(state, font, best_text);


			v2 pos = V2(0.98f - maxf(dist_size.x, best_size.x), 0.98f);
			pos.y -= dist_size.y;
			text_render(state, font, dist_text, pos);
			pos.y -= best_size.y;
			text_render(state, font, best_text, pos);

		}

	}

#if 0
	{ // cost
		char text[64];
		snprintf(text, sizeof text - 1, "Cost: %.1f", platforms_cost(state->platforms, state->nplatforms));
		glColor3f(1, 1, 0.5f);
		text_render(state, font, text, V2(-0.95f, -0.95f));
	}
#endif

	{ // details in the bottom right
		char text[64] = {0};
		glColor3f(0.5f,0.5f,0.5f);
		// position of ball
		snprintf(text, sizeof text - 1, "(%.2f, %.2f)", ball->pos.x, ball->pos.y);
		v2 size = text_get_size(state, small_font, text);
		v2 pos = V2(1 - size.x, -1 + size.y);
		text_render(state, small_font, text, pos);
		// stuck time
		snprintf(text, sizeof text - 1, "Last record: %.1fs ago", state->stuck_time);
		size = text_get_size(state, small_font, text);
		pos.x = 1 - size.x;
		pos.y += size.y * 1.5f;
		text_render(state, small_font, text, pos);
		// total time
		snprintf(text, sizeof text - 1, "Total time: %.1fs", state->total_time);
		size = text_get_size(state, small_font, text);
		pos.x = 1 - size.x;
		pos.y += size.y * 1.5f;
		text_render(state, small_font, text, pos);
	}

	#if DEBUG
	GLuint error = glGetError();
	if (error) {
		printf("!!! GL ERROR: %u\n", error);
	}
	#endif
}
