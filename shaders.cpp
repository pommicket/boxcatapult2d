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

