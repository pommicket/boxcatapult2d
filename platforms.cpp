// how far right could any part of this platform possibly go?
static float platform_rightmost_x(Platform const *platform) {
	float angle;
	if (platform->rotates)
		angle = 0; // for rotating platforms, the maximum x coordinate is achieved when the platform has an angle of 0
	else
		angle = platform->angle;
	float x_past_center = platform->radius * fabsf(cosf(angle)); // width of platform to the right of the center
	v2 center;
	if (platform->moves) {
		v2 p1 = platform->move_p1, p2 = platform->move_p2;
		// pick the point with the higher x coordinate
		if (p1.x > p2.x)
			center = p1;
		else
			center = p2;
	} else {
		center = platform->center;
	}
	return center.x + x_past_center;
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
					v2 last_point;
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
		platforms[index].body->GetFixtureList()->GetUserData().pointer = platform_to_user_data(state, &platforms[index]);
	} else {
		// platform is at end of array; don't need to do anything special
		memset(&platforms[index], 0, sizeof(Platform));
	}
	--state->nplatforms;

}

static float platform_cost(Platform const *platform) {
	float cost = platform->radius;
	if (platform->moves)
		cost += platform->move_speed;
	if (platform->rotates)
		cost += platform->rotate_speed;
	return cost;
}

static float platforms_cost(Platform const *platforms, u32 nplatforms) {
	float total_cost = 0;
	for (u32 i = 0; i < nplatforms; ++i) {
		total_cost += platform_cost(&platforms[i]);
	}
	return total_cost;
}
