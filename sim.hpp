#ifndef SIM_H_
#define SIM_H_

#if _WIN32
#include <box2d.h>
#else
#include <box2d/box2d.h>
#endif

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
no_warn_start
#include "lib/stb_truetype.h"
no_warn_end

// enums with a specified width are a clang C extension & available in C++11
#if defined __clang__ || __cplusplus >= 201103L
#define ENUM_U8 typedef enum : u8 
#define ENUM_U8_END(name) name
#define ENUM_U16 typedef enum : u16
#define ENUM_U16_END(name) name
#else
#define ENUM_U8 enum
#define ENUM_U8_END(name) ; typedef u8 name
#define ENUM_U16 enum
#define ENUM_U16_END(name) ; typedef u16 name
#endif


#ifdef __GNUC__
#define maybe_unused __attribute__((unused))
#else
#define maybe_unused
#endif

typedef void (APIENTRY *GLAttachShader)(GLuint program, GLuint shader);
typedef void (APIENTRY *GLCompileShader)(GLuint shader);
typedef GLuint (APIENTRY *GLCreateProgram)(void);
typedef GLuint (APIENTRY *GLCreateShader)(GLenum shaderType);
typedef void (APIENTRY *GLDeleteProgram)(GLuint program);
typedef void (APIENTRY *GLDeleteShader)(GLuint shader);
typedef GLint (APIENTRY *GLGetAttribLocation)(GLuint program, const char *name);
typedef void (APIENTRY *GLGetProgramInfoLog)(GLuint program, GLsizei maxLength, GLsizei *length, char *infoLog);
typedef void (APIENTRY *GLGetProgramiv)(GLuint program, GLenum pname, GLint *params);
typedef void (APIENTRY *GLGetShaderInfoLog)(GLuint shader, GLsizei maxLength, GLsizei *length, char *infoLog);
typedef void (APIENTRY *GLGetShaderiv)(GLuint shader, GLenum pname, GLint *params);
typedef GLint (APIENTRY *GLGetUniformLocation)(GLuint program, char const *name);
typedef void (APIENTRY *GLLinkProgram)(GLuint program);
typedef void (APIENTRY *GLShaderSource)(GLuint shader, GLsizei count, const char *const *string, const GLint *length);
typedef void (APIENTRY *GLUniform1f)(GLint location, GLfloat v0);
typedef void (APIENTRY *GLUniform2f)(GLint location, GLfloat v0, GLfloat v1);
typedef void (APIENTRY *GLUniform3f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY *GLUniform4f)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
typedef void (APIENTRY *GLUniform1i)(GLint location, GLint v0);
typedef void (APIENTRY *GLUniform2i)(GLint location, GLint v0, GLint v1);
typedef void (APIENTRY *GLUniform3i)(GLint location, GLint v0, GLint v1, GLint v2);
typedef void (APIENTRY *GLUniform4i)(GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
typedef void (APIENTRY *GLUniformMatrix4fv)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef void (APIENTRY *GLUseProgram)(GLuint program);
typedef void (APIENTRY *GLVertexAttrib1f)(GLuint index, GLfloat v0);
typedef void (APIENTRY *GLVertexAttrib2f)(GLuint index, GLfloat v0, GLfloat v1);
typedef void (APIENTRY *GLVertexAttrib3f)(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void (APIENTRY *GLVertexAttrib4f)(GLuint index, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

typedef struct {
	GLAttachShader AttachShader;
	GLCompileShader CompileShader;
	GLCreateProgram CreateProgram;
	GLCreateShader CreateShader;
	GLDeleteProgram DeleteProgram;
	GLDeleteShader DeleteShader;
	GLGetAttribLocation GetAttribLocation;
	GLGetProgramInfoLog GetProgramInfoLog;
	GLGetProgramiv GetProgramiv;
	GLGetShaderInfoLog GetShaderInfoLog;
	GLGetShaderiv GetShaderiv;
	GLGetUniformLocation GetUniformLocation;
	GLLinkProgram LinkProgram;
	GLShaderSource ShaderSource;
	GLUniform1f Uniform1f;
	GLUniform2f Uniform2f;
	GLUniform3f Uniform3f;
	GLUniform4f Uniform4f;
	GLUniform1i Uniform1i;
	GLUniform2i Uniform2i;
	GLUniform3i Uniform3i;
	GLUniform4i Uniform4i;
	GLUniformMatrix4fv UniformMatrix4fv;
	GLUseProgram UseProgram;
	GLVertexAttrib1f VertexAttrib1f;
	GLVertexAttrib2f VertexAttrib2f;
	GLVertexAttrib3f VertexAttrib3f;
	GLVertexAttrib4f VertexAttrib4f;
} GL;

typedef struct {
	float char_height;
	GLuint texture;
	int tex_width, tex_height;
	stbtt_bakedchar char_data[96];
} Font;

typedef struct {
	GLuint program;
#if DEBUG
	char vertex_filename[32];
	char fragment_filename[32];
	struct timespec last_modified;
#endif
} ShaderBase;

typedef GLuint VertexAttributeLocation;
typedef GLint UniformLocation;

// shader for platforms
typedef struct {
	ShaderBase base;
	VertexAttributeLocation vertex_p1, vertex_p2; // endpoints of platform
	UniformLocation uniform_thickness; // this is half the "width" of the platform
	UniformLocation uniform_transform; // 4x4 matrix position is multiplied by
} ShaderPlatform;

typedef struct {	
	ShaderBase base;
	UniformLocation uniform_transform, uniform_center, uniform_radius;
} ShaderBall;

typedef struct {
	b2Body *body; // Box2D body for platform -- created when setup_use is called (for setups), or when the platform is manually built


	v2 center;
	float radius; // half of the width of the platform
	
	// save the starting position and rotation of the platform so we
	// can restore it to reset the setup
	float start_angle;

	float angle;

	bool moves; // does this platform move?
	bool rotates; // does this platform rotate?

	// if it's a moving platform
	float move_speed;
	v2 move_p1;
	v2 move_p2;

	float rotate_speed; // rotation speed. if this isn't a rotating platform, this should be 0

	u32 color;
} Platform;

typedef struct {
	v2 pos; // position
	float radius;
	b2Body *body;
} Ball;

#define USER_DATA_TYPE_SHIFT 12
#define USER_DATA_TYPE (((uintptr_t)-1) << USER_DATA_TYPE_SHIFT)
#define USER_DATA_INDEX (~USER_DATA_TYPE)
#define USER_DATA_TYPE_PLATFORM ((uintptr_t)0x1)
// indicator that this Box2D fixture is a platform
#define USER_DATA_PLATFORM (USER_DATA_TYPE_PLATFORM << USER_DATA_TYPE_SHIFT)

#define MAX_PLATFORMS 32
typedef struct {
	float score; // distance this setup can throw the ball
	float total_time; // time it took to finish
	u64 mutations;
	u32 nplatforms;
	Platform platforms[MAX_PLATFORMS];
} Setup;

typedef struct {
	bool initialized;

	float win_width, win_height; // width,height of window in pixels
	float aspect_ratio; // width / height

	v2 mouse_pos; // mouse position in Box2D (not GL) coordinates
	v2 mouse_pos_gl; // mouse position in GL coordinates

	bool shift, ctrl; // is either shift/ctrl key down?

	float dt; // time in seconds since last frame

	// physics time left unsimulated last frame
	// (we need to always pass Box2D the same dt for consistency, so there
	// will be some left over, if the frame time is not a multiple of the fixed time step)
	float time_residue; 
	m4 transform; // the transform for converting our coordinates to GL coordinates
	m4 inv_transform; // inverse of transform (for converting GL coordinates to our coordinates)

	GL gl; // gl functions
	ShaderPlatform shader_platform;
	ShaderBall shader_ball;

	bool start_menu; // "press any key to begin"
	bool pressed_any_key_to_begin; // we need to delay beginning by a frame to show "Loading..."
	bool building; // is the user building a setup?
	bool setting_move_p2; // is the user setting the move_p2 of the platform they're placing?
	bool simulating; // are we simulating the world's physics?
	bool evolve_menu; // is the evolve menu shown?
	bool evolving; // are we simulating generations?
	bool run_one_generation; // only run one generation, then stop.

	u32 scoring_next; // which of this generation's setups we are scoring next

	u64 generation; // which generation we are on

	b2World *world; // Box2D world

	Ball ball;
	float furthest_ball_x_pos; // furthest distance the ball has reached
	float stuck_time; // amount of time furthest_ball_x_pos hasn't changed for
	float total_time; // amount of time the simulation has been running for

	float bottom_y; // y-position of "floor" (if y goes below here, it's over)
	float left_x; // y-position of left wall

	v2 pan; // pan for the editor

	Font font;
	Font small_font;
	Font large_font;

	Platform platform_building; // the platform the user is currently placing
	float platform_thickness;

	u32 nplatforms;
	Platform platforms[MAX_PLATFORMS];

#define GENERATION_SIZE 100
#define TOP_KEPT 10 // keep top this many setups after every generation
	Setup setups[TOP_KEPT + GENERATION_SIZE];

	u32 tmp_mem_used; // this is not measured in bytes, but in MaxAligns 
#define TMP_MEM_BYTES (4L<<20)
	MaxAlign tmp_mem[TMP_MEM_BYTES / sizeof(MaxAlign)];
#if DEBUG
	u32 magic_number;
	#define MAGIC_NUMBER 0x1234ACAB
#endif
} State;

#endif // SIM_H_
