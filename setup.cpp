// these only apply to computer-generated setups
#define SETUP_MIN_X 1.0f
#define SETUP_MAX_X 10.0f
#define SETUP_MIN_Y 1.0f
#define SETUP_MAX_Y 10.0f

static v2 setup_rand_point(void) {
	return V2(
		rand_uniform(SETUP_MIN_X, SETUP_MAX_X),
		rand_uniform(SETUP_MIN_Y, SETUP_MAX_Y)
	);
}

#define PLATFORM_MOVE_CHANCE 0.5f // chance that the platform will be a moving one
#define PLATFORM_ROTATE_CHANCE 0.5f // chance that the platform will be a rotating one (platforms can be moving and rotating)
static void setup_random(Setup *setup, float cost_allowed) {
	// how much "money" we have to spend
	float cost_left = cost_allowed;
	
	while (cost_left > PLATFORM_RADIUS_MIN * PLATFORM_RADIUS_COST && setup->nplatforms < MAX_PLATFORMS) {
		Platform *platform = &setup->platforms[setup->nplatforms++];

		platform->color = rand_u32() | 0xFF;
		float max_radius_allowed = cost_left / PLATFORM_RADIUS_COST;
		if (max_radius_allowed > PLATFORM_RADIUS_MAX)
			max_radius_allowed = PLATFORM_RADIUS_MAX;
		platform->radius = rand_uniform(PLATFORM_RADIUS_MIN, max_radius_allowed);
		platform->center = setup_rand_point();
		platform->start_angle = rand_uniform(0, PIf);
		cost_left -= platform->radius * PLATFORM_RADIUS_COST;

		if (cost_left > PLATFORM_MOVE_SPEED_COST * PLATFORM_MOVE_SPEED_MIN) {
			if (randf() < PLATFORM_MOVE_CHANCE) {
				platform->moves = true;
				float max_speed_allowed = cost_left / PLATFORM_MOVE_SPEED_COST;
				if (max_speed_allowed > PLATFORM_MOVE_SPEED_MAX) 
					max_speed_allowed = PLATFORM_MOVE_SPEED_MAX;
				platform->move_speed = rand_uniform(PLATFORM_MOVE_SPEED_MIN, max_speed_allowed);
				platform->move_p1 = platform->center;
				platform->move_p2 = setup_rand_point();
				cost_left -= platform->move_speed * PLATFORM_MOVE_SPEED_COST;
			}
		}

		if (cost_left > PLATFORM_ROTATE_SPEED_COST * 0.1f) {
			if (randf() < PLATFORM_ROTATE_CHANCE) {
				float max_rotate_speed_allowed = cost_left / PLATFORM_ROTATE_SPEED_COST;
				if (max_rotate_speed_allowed > PLATFORM_ROTATE_SPEED_MAX)
					max_rotate_speed_allowed = PLATFORM_ROTATE_SPEED_MAX;
				platform->rotates = true;
				platform->rotate_speed = rand_uniform(0.1f, max_rotate_speed_allowed);
				if (rand() % 2)
					platform->rotate_speed = -platform->rotate_speed; // clockwise
				cost_left -= fabsf(platform->rotate_speed) * PLATFORM_ROTATE_SPEED_COST;
			}
		}
	}
	assert(cost_left >= 0);
}

// make this setup the active one
static void setup_use(State *state, Setup *setup) {
	b2World *world = state->world;
	// get rid of old platform bodies
	for (u32 i = 0; i < state->nplatforms; ++i) {
		Platform *p = &state->platforms[i];
		if (p->body)
			world->DestroyBody(p->body);
	}
	memcpy(state->platforms, setup->platforms, setup->nplatforms * sizeof(Platform));
	state->nplatforms = setup->nplatforms;
	// create new bodies
	for (u32 i = 0; i < state->nplatforms; ++i) {
		Platform *p = &state->platforms[i];
		p->body = NULL;
		platform_make_body(state, p, i);
	}
	assert((u32)world->GetBodyCount() == state->nplatforms + 2); // platforms + 2 walls
}

static void setup_reset(State *state) {
	{ // reset ball
		Ball *ball = &state->ball;
		b2World *world = state->world;
		if (ball->body)
			world->DestroyBody(ball->body);


		ball->radius = 0.3f;
		ball->pos = V2(BALL_STARTING_X, 10.0f);

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
		ball_fixture.restitution = 0.6f; // bounciness

		ball_body->CreateFixture(&ball_fixture);

	}
	for (Platform *platform = state->platforms, *end = platform + state->nplatforms; platform != end; ++platform) { // reset platforms
		b2Body *body = platform->body;
		assert(body);
		platform->angle = platform->start_angle;
		if (platform->moves) platform->center = platform->move_p1;
		body->SetTransform(v2_to_b2(platform->center), platform->angle);
		if (platform->moves) {
			float speed = platform->move_speed;
			v2 p1 = platform->move_p1, p2 = platform->move_p2;
			v2 direction = v2_normalize(v2_sub(p2, p1));
			v2 velocity = v2_scale(direction, speed);
			body->SetLinearVelocity(v2_to_b2(velocity));
		} 
		body->SetAngularVelocity(platform->rotate_speed);
	}
	state->setting_move_p2 = false;
	state->furthest_ball_x_pos = 0;
	state->stuck_time = 0;
	state->total_time = 0;
	state->time_residue = 0;
}

static float setup_score(State *state, Setup *setup) {
	setup_use(state, setup);
	setup_reset(state);
	Ball *ball = &state->ball;
	float starting_line = platforms_starting_line(setup->platforms, setup->nplatforms);
	while (ball->body) {
		simulate_time(state, 0.1f);
	}
	setup->score = ball->pos.x - starting_line;
	return setup->score;
}

static bool setup_write_to_file(Setup const *setup, char const *filename) {
	FILE *fp = fopen(filename, "wb");
	if (fp) {
		u32 nplatforms = setup->nplatforms;
		fwrite_u32(fp, nplatforms);
		for (u32 i = 0; i < nplatforms; ++i) {
			platform_write_to_file(&setup->platforms[i], fp);
		}
		fclose(fp);
		return true;
	} else {
		logln("Couldn't write setup to %s.", filename);
		return false;
	}
}

static bool setup_read_from_file(Setup *setup, char const *filename) {
	FILE *fp = fopen(filename, "rb");
	if (fp) {
		u32 nplatforms = setup->nplatforms = fread_u32(fp);
		for (u32 i = 0; i < nplatforms; ++i) {
			platform_read_from_file(&setup->platforms[i], fp);
		}
		fclose(fp);
		return true;
	} else {
		logln("Couldn't read setup from %s.", filename);
		return false;
	}
}

