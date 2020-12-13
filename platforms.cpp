#define PLATFORM_RADIUS_MIN 0.5f
#define PLATFORM_RADIUS_MAX 2.0f
#define PLATFORM_MOVE_SPEED_MIN 0.1f
#define PLATFORM_MOVE_SPEED_MAX 5.0f
#define PLATFORM_ROTATE_SPEED_MIN -3.0f
#define PLATFORM_ROTATE_SPEED_MAX +3.0f

// additional cost for each additional meter of platform radius
#define PLATFORM_RADIUS_COST 1.0f
#define PLATFORM_MOVE_SPEED_COST 1.0f
#define PLATFORM_ROTATE_SPEED_COST 1.0f

// some horrible code to figure out the rightmost x coordinate or the topmost y coordinate a platform can reach
static float platform_highest_coordinate(Platform const *platform, bool y) {
	float angle;
	if (platform->rotates)
		angle = y ? HALF_PIf : 0;
	else
		angle = platform->start_angle;
	float past_center = platform->radius * fabsf((y ? sinf : cosf)(angle)); // width/height of platform to the right of the center
	v2 center;
	if (platform->moves) {
		v2 p1 = platform->move_p1, p2 = platform->move_p2;
		// pick the point with the higher x coordinate
		if ((y ? p1.y : p1.x) > (y ? p2.y : p2.x))
			center = p1;
		else
			center = p2;
	} else {
		center = platform->center;
	}
	return (y ? center.y : center.x) + past_center;
}

static float platform_lowest_coordinate(Platform const *platform, bool y) {
	float angle;
	if (platform->rotates)
		angle = y ? HALF_PIf : 0;
	else
		angle = platform->start_angle;
	float before_center = platform->radius * -fabsf((y ? sinf : cosf)(angle));
	v2 center;
	if (platform->moves) {
		v2 p1 = platform->move_p1, p2 = platform->move_p2;
		if ((y ? p1.y : p1.x) < (y ? p2.y : p2.x))
			center = p1;
		else
			center = p2;
	} else {
		center = platform->center;
	}
	return (y ? center.y : center.x) + before_center;
}

static Rect platform_bounding_box(Platform const *platform) {
	float x1 = platform_lowest_coordinate(platform, false);
	float y1 = platform_lowest_coordinate(platform, true);
	float x2 = platform_highest_coordinate(platform, false);
	float y2 = platform_highest_coordinate(platform, true);
	return rect4(x1, y1, x2, y2);
}

static float platform_rightmost_x(Platform const *platform) {
	return platform_highest_coordinate(platform, false);
}

// where the ball's distance traveled should be measured from
static float platforms_starting_line(Platform const *platforms, u32 nplatforms) {
	float rightmost_x = BALL_STARTING_X; // the starting line can't be to the left of the ball
	for (u32 i = 0; i < nplatforms; ++i) {
		float x = platform_rightmost_x(&platforms[i]);
		if (x > rightmost_x)
			rightmost_x = x;
	}
	return rightmost_x;
}

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
		float radius = platform->radius;
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
		glBegin(GL_LINES);
		for (Platform *platform = platforms, *end = platform + nplatforms; platform != end; ++platform) {
			gl_rgbacolor(platform->color);
			if (platform->rotates) {
				float speed = platform->rotate_speed;
				float angle = speed * 0.5f;
				if (angle) {
					// draw arc-shaped arrow to show rotation
					float theta1 = HALF_PIf;
					float theta2 = theta1 + angle;
					float dtheta = 0.03f * sgnf(angle);
					float radius = platform->radius;
					v2 last_point = {};
					for (float theta = theta1; angle > 0 ? (theta < theta2) : (theta > theta2); theta += dtheta) {
						v2 point = b2_to_gl(state, v2_add(platform->center, v2_polar(radius, theta)));
						if (theta != theta1) {
							v2_gl_vertex(last_point);
							v2_gl_vertex(point);
						}
						last_point = point;
					}

					v2 p1 = b2_to_gl(state, v2_add(platform->center, v2_polar(radius-0.2f, theta2-0.1f * sgnf(angle))));
					v2 p2 = b2_to_gl(state, v2_add(platform->center, v2_polar(radius, theta2)));
					v2 p3 = b2_to_gl(state, v2_add(platform->center, v2_polar(radius+0.2f, theta2-0.1f * sgnf(angle))));

					v2_gl_vertex(p1); v2_gl_vertex(p2);
					v2_gl_vertex(p2); v2_gl_vertex(p3);
				}
			}

			if (platform->moves) {
				// draw double-headed arrow to show back & forth motion
				v2 p1 = platform->move_p1;
				v2 p2 = platform->move_p2;
				v2 p2_to_p1 = v2_scale(v2_normalize(v2_sub(p1, p2)), platform->move_speed * 0.5f);
				v2 p1_to_p2 = v2_scale(p2_to_p1, -1);
				v2 arrowhead_a1 = v2_add(p1, v2_rotate(p1_to_p2, +0.5f));
				v2 arrowhead_b1 = v2_add(p1, v2_rotate(p1_to_p2, -0.5f));
				v2 arrowhead_a2 = v2_add(p2, v2_rotate(p2_to_p1, +0.5f));
				v2 arrowhead_b2 = v2_add(p2, v2_rotate(p2_to_p1, -0.5f));
				
				v2 p1_gl = b2_to_gl(state, p1);
				v2 p2_gl = b2_to_gl(state, p2);
				v2 aa1_gl = b2_to_gl(state, arrowhead_a1);
				v2 ab1_gl = b2_to_gl(state, arrowhead_b1);
				v2 aa2_gl = b2_to_gl(state, arrowhead_a2);
				v2 ab2_gl = b2_to_gl(state, arrowhead_b2);

				v2_gl_vertex(p1_gl);
				v2_gl_vertex(p2_gl);

				v2_gl_vertex(aa1_gl);
				v2_gl_vertex(p1_gl);
				v2_gl_vertex(p1_gl);
				v2_gl_vertex(ab1_gl);

				v2_gl_vertex(aa2_gl);
				v2_gl_vertex(p2_gl);
				v2_gl_vertex(p2_gl);
				v2_gl_vertex(ab2_gl);
			}
		}
		glEnd();
	}
}

// sets platform->body to a new Box2D body.
static void platform_make_body(State *state, Platform *platform, u32 index) {
	b2World *world = state->world;
	assert(!platform->body);

	float radius = platform->radius;
	
	if (platform->moves)
		platform->center = platform->move_p1;

	v2 center = platform->center;

	b2BodyDef body_def;
	body_def.type = b2_kinematicBody;
	body_def.position.Set(center.x, center.y);
	body_def.angle = platform->angle;
	b2Body *body = world->CreateBody(&body_def);

	b2PolygonShape shape;
	shape.SetAsBox(radius, state->platform_thickness);

	b2FixtureDef fixture;
	fixture.shape = &shape;
	fixture.friction = 0.5f;
	fixture.userData.pointer = USER_DATA_PLATFORM | index;

	body->CreateFixture(&fixture);
	
	// velocity and rotate speed will be set when setup_reset is called
	
	platform->body = body;
}

class PlatformQueryCallback : public b2QueryCallback {
public:
	PlatformQueryCallback(State *state_) {
		this->state = state_;
	}
	bool ReportFixture(b2Fixture *fixture) {
		uintptr_t userdata = fixture->GetUserData().pointer;
		if (userdata & USER_DATA_PLATFORM) {
			u32 index = (u32)(userdata & ~USER_DATA_PLATFORM);
			assert(index < state->nplatforms);
			platform = &state->platforms[index];
			return false; // we can stop now
		} else {
			return true; // keep going
		}
	}
	Platform *platform = NULL;
	State *state = NULL;
};

static Platform *platform_at_mouse_pos(State *state) {
	v2 mouse_pos = state->mouse_pos;
	v2 mouse_radius = V2(0.1f, 0.1f); // you don't have to hover exactly over a platform to select it. this is the tolerance.
	v2 a = v2_sub(mouse_pos, mouse_radius);
	v2 b = v2_add(mouse_pos, mouse_radius);
	PlatformQueryCallback callback(state);
	b2AABB aabb;
	aabb.lowerBound = v2_to_b2(a);
	aabb.upperBound = v2_to_b2(b);
	state->world->QueryAABB(&callback, aabb);
	return callback.platform;
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
		platforms[index].body->GetFixtureList()->GetUserData().pointer = index | USER_DATA_PLATFORM;
	} else {
		// platform is at end of array; don't need to do anything special
		memset(&platforms[index], 0, sizeof(Platform));
	}
	--state->nplatforms;

}

static float platform_cost(Platform const *platform) {
	float cost = platform->radius * PLATFORM_RADIUS_COST;
	if (platform->moves)
		cost += platform->move_speed * PLATFORM_MOVE_SPEED_COST;
	if (platform->rotates)
		cost += fabsf(platform->rotate_speed) * PLATFORM_ROTATE_SPEED_COST;
	return cost;
}

static float platforms_cost(Platform const *platforms, u32 nplatforms) {
	float total_cost = 0;
	for (u32 i = 0; i < nplatforms; ++i) {
		total_cost += platform_cost(&platforms[i]);
	}
	return total_cost;
}

static void platform_write_to_file(Platform const *p, FILE *fp) {
	fwrite_float(fp, p->radius);
	fwrite_float(fp, p->start_angle);
	fwrite_u32(fp, p->color);

	u8 flags = (u8)(p->moves * 1) | (u8)(p->rotates * 2);
	fwrite_u8(fp, flags);
	if (p->moves) {
		fwrite_float(fp, p->move_speed);
		fwrite_v2(fp, p->move_p1);
		fwrite_v2(fp, p->move_p2);
	} else {
		fwrite_v2(fp, p->center);
	}
	if (p->rotates) {
		fwrite_float(fp, p->rotate_speed);
	}
}

static void platform_read_from_file(Platform *p, FILE *fp) {
	p->radius = fread_float(fp);
	p->start_angle = fread_float(fp);
	p->color = fread_u32(fp);

	u8 flags = fread_u8(fp);
	p->moves = (flags & 1) != 0;
	p->rotates = (flags & 2) != 0;

	if (p->moves) {
		p->move_speed = fread_float(fp);
		p->move_p1 = fread_v2(fp);
		p->move_p2 = fread_v2(fp);
	} else {
		p->center = fread_v2(fp);
	}

	if (p->rotates) {
		p->rotate_speed = fread_float(fp);
	}
}

static v2 setup_rand_point(void);

#define PLATFORM_MOVE_CHANCE 0.5f // chance that the platform will be a moving one
#define PLATFORM_ROTATE_CHANCE 0.5f // chance that the platform will be a rotating one (platforms can be moving and rotating)

static void platform_random(Platform *platform) {
	do {
		platform->color = rand_u32() | 0xFF;
	} while (rgba_brightness(platform->color) < 0.5f); // ensure color is visible
	platform->radius = rand_uniform(PLATFORM_RADIUS_MIN, PLATFORM_RADIUS_MAX);
	platform->center = setup_rand_point();
	platform->start_angle = rand_uniform(0, PIf);

	if (randf() < PLATFORM_MOVE_CHANCE) {
		platform->moves = true;
		platform->move_speed = rand_uniform(PLATFORM_MOVE_SPEED_MIN, PLATFORM_MOVE_SPEED_MAX);
		platform->move_p1 = platform->center;
		platform->move_p2 = v2_add(platform->move_p1, v2_scale(v2_rand_unit(), 2 * rand_gauss()));
	}

	if (randf() < PLATFORM_ROTATE_CHANCE) {
		platform->rotates = true;
		platform->rotate_speed = rand_uniform(0.1f, PLATFORM_ROTATE_SPEED_MAX);
		if (rand() % 2)
			platform->rotate_speed = -platform->rotate_speed; // clockwise
	}
}

static void mutate_position(v2 *p) {
	if (randf() < 0.2f)
		*p = setup_rand_point(); // randomize completely
	else
		*p = v2_add(*p, v2_scale(V2(rand_gauss(), rand_gauss()), 0.1f));
}

static void platform_mutate(State *state, Setup *setup, Platform *platform) {
	Platform original = *platform;
	int i;
	int max_attempts = 100;
	for (i = 0; i < max_attempts; ++i) { // make at most max_attempts attempts to mutate the platform
		*platform = original;

		if (randf() < 0.2f) {
			// completely randomize platform
			platform_random(platform);
		} else {
			// partially randomize platform
#define FEATURE_MUTATE_RATE 0.3f
			if (randf() < FEATURE_MUTATE_RATE) {
				platform->start_angle += 0.3f * rand_gauss(); // mutate angle
				platform->start_angle = fmodf(platform->start_angle, TAUf);
			}
			if (platform->moves) {
				if (randf() < FEATURE_MUTATE_RATE) {
					platform->move_speed += 0.1f * rand_gauss(); // mutate move speed
					platform->move_speed = clampf(platform->move_speed, PLATFORM_MOVE_SPEED_MIN, PLATFORM_MOVE_SPEED_MAX);
				}
				if (randf() < FEATURE_MUTATE_RATE)
					mutate_position(&platform->move_p1); // mutate p1
				if (randf() < FEATURE_MUTATE_RATE)
					mutate_position(&platform->move_p2); // mutate p2
			} else if (randf() < FEATURE_MUTATE_RATE) {
				// mutate position
				mutate_position(&platform->center);
			}
			if (platform->rotates) {
				if (randf() < FEATURE_MUTATE_RATE) {
					// mutate rotate speed
					platform->rotate_speed += 0.3f * rand_gauss();
					platform->rotate_speed = clampf(platform->rotate_speed, PLATFORM_ROTATE_SPEED_MIN, PLATFORM_ROTATE_SPEED_MAX);
				}
			}
			if (randf() < FEATURE_MUTATE_RATE) {
				// mutate radius
				platform->radius += 0.1f * rand_gauss();
				platform->radius = clampf(platform->radius, PLATFORM_RADIUS_MIN, PLATFORM_RADIUS_MAX);
			}
		}
		
		Rect bbox = platform_bounding_box(platform);
		bool intersects_any = false;
		for (Platform *platform2 = setup->platforms, *end = platform2 + setup->nplatforms;
			platform2 != end; ++platform2) {
			if (platform != platform2) {
				if (rects_intersect(bbox, platform_bounding_box(platform2))) {
					intersects_any = true;
					break;
				}
			}
		}
		if (!intersects_any && bbox.pos.x > state->left_x) break; // doesn't intersect anything; we're good.
	}
	if (i == max_attempts) {
		// we tried so many times but we couldn't mutate the platform ):
		*platform = original; // give up on mutation
	}
}
