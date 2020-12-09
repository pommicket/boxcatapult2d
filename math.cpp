#ifdef MATH_GL
#undef MATH_GL
#define MATH_GL 1
#endif

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#define PIf 3.14159265358979f
#define HALF_PIf 1.5707963267948966f
#define TAUf 6.283185307179586f
#define SQRT2f 1.4142135623730951f
#define HALF_SQRT2f 0.7071067811865476f
#define SQRT3f 1.7320508075688772f
#define HALF_SQRT3f 0.8660254037844386f

#include <math.h>

static float degrees(float r) {
	return r * (180.0f / PIf);
}
static float radians(float r) {
	return r * (PIf / 180.f);
}

// map x from the interval [0, 1] to the interval [a, b]. does NOT clamp.
static float lerpf(float x, float a, float b) {
	return x * (b-a) + a;
}

// opposite of lerp; map x from the interval [a, b] to the interval [0, 1]. does NOT clamp.
static float normf(float x, float a, float b) {
	return (x-a) / (b-a);
}

static float clampf(float x, float a, float b) {
	if (x < a) return a;
	if (x > b) return b;
	return x;
}

static int clampi(int x, int a, int b) {
	if (x < a) return a;
	if (x > b) return b;
	return x;
}

static i32 clampi32(i32 x, i32 a, i32 b) {
	if (x < a) return a;
	if (x > b) return b;
	return x;
}

// remap x from the interval [from_a, from_b] to the interval [to_a, to_b], NOT clamping if x is outside the "from" interval.
static float remapf(float x, float from_a, float from_b, float to_a, float to_b) {
	float pos = (x - from_a) / (from_b - from_a);
	return lerpf(pos, to_a, to_b);
}

static float minf(float a, float b) {
	return a < b ? a : b;
}

static float maxf(float a, float b) {
	return a > b ? a : b;
}

static float smoothstepf(float x) {
	if (x <= 0) return 0;
	if (x >= 1) return 1;
	return x * x * (3 - 2 * x);
}

static float randf(void) {
	return (float)rand() / (float)((ulong)RAND_MAX + 1);
}

static u32 rand_u32(void) {
	return ((u32)rand() & 0xfff)
		| ((u32)rand() & 0xfff) << 12
		| ((u32)rand() & 0xff) << 24;
}

static float rand_uniform(float from, float to) {
	return lerpf(randf(), from, to);
}

static float sigmoidf(float x) {
	return 1.0f / (1.0f + expf(-x));
}

// returns ⌈x/y⌉ (x/y rounded up)
static i32 ceildivi32(i32 x, i32 y) {
	if (y < 0) {
		// negating both operands doesn't change the answer
		x = -x;
		y = -y;
	}
	if (x < 0) {
		// truncation is the same as ceiling for negative numbers
		return x / y;
	} else {
		return (x + (y-1)) / y;
	}
}

typedef struct {
	float x, y;
} v2;

static v2 const v2_zero = {0, 0};
static v2 V2(float x, float y) {
	v2 v;
	v.x = x;
	v.y = y;
	return v;
}

static v2 v2_add(v2 a, v2 b) {
	return V2(a.x + b.x, a.y + b.y);
}

static v2 v2_add_const(v2 a, float c) {
	return V2(a.x + c, a.y + c);
}

static v2 v2_sub(v2 a, v2 b) {
	return V2(a.x - b.x, a.y - b.y);
}

static v2 v2_scale(v2 v, float s) {
	return V2(v.x * s, v.y * s);
}

static v2 v2_mul(v2 a, v2 b) {
	return V2(a.x * b.x, a.y * b.y);
}

static float v2_dot(v2 a, v2 b) {
	return a.x * b.x + a.y * b.y;
}

static float v2_len(v2 v) {
	return sqrtf(v2_dot(v, v));
}

static v2 v2_lerp(float x, v2 a, v2 b) {
	return V2(lerpf(x, a.x, b.x), lerpf(x, a.y, b.y));
}

static v2 v2_normalize(v2 v) {
	float len = v2_len(v);
	float mul = len == 0.0f ? 1.0f : 1.0f/len;
	return v2_scale(v, mul);
}

static float v2_dist(v2 a, v2 b) {
	return v2_len(v2_sub(a, b));
}

static float v2_dist_squared(v2 a, v2 b) {
	v2 diff = v2_sub(a, b);
	return v2_dot(diff, diff);
}

static void v2_print(v2 v) {
	printf("(%f, %f)\n", v.x, v.y);
}

#if MATH_GL
static void v2_gl_vertex(v2 v) {
	glVertex2f(v.x, v.y);
}
#endif

static v2 v2_rand_unit(void) {
	float theta = rand_uniform(0, TAUf);
	return V2(cosf(theta), sinf(theta));
}

#if MATH_GL
static void v2_rand_unit_test(void) {
	int i;
	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_POINTS);
	for (i = 0; i < 100000; ++i) {
		v2_gl_vertex(v2_rand_unit());
	}
	glEnd();
}
#endif

static v2 v2_polar(float r, float theta) {
	return V2(r * cosf(theta), r * sinf(theta));
}

typedef struct {
	float x, y, z;
} v3;

static v3 const v3_zero = {0, 0, 0};

static v3 V3(float x, float y, float z) {
	v3 v;
	v.x = x;
	v.y = y;
	v.z = z;
	return v;
}

#if MATH_GL
static void v3_gl_vertex(v3 v) {
	glVertex3f(v.x, v.y, v.z);
}

static void v3_gl_color(v3 v) {
	glColor3f(v.x, v.y, v.z);
}

static void v3_gl_color_alpha(v3 v, float alpha) {
	glColor4f(v.x, v.y, v.z, alpha);
}
#endif

static v3 v3_add(v3 a, v3 b) {
	return V3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static v3 v3_sub(v3 a, v3 b) {
	return V3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static v3 v3_scale(v3 v, float s) {
	return V3(v.x * s, v.y * s, v.z * s);
}

static v3 v3_lerp(float x, v3 a, v3 b) {
	return V3(lerpf(x, a.x, b.x), lerpf(x, a.y, b.y), lerpf(x, a.z, b.z));
}

static float v3_dot(v3 u, v3 v) {
	return u.x*v.x + u.y*v.y + u.z*v.z;
}

static v3 v3_cross(v3 u, v3 v) {
	v3 prod = V3(u.y*v.z - u.z*v.y, u.z*v.x - u.x*v.z, u.x*v.y - u.y*v.x);
	return prod;
}

static float v3_len(v3 v) {
	return sqrtf(v3_dot(v, v));
}

static float v3_dist(v3 a, v3 b) {
	return v3_len(v3_sub(a, b));
}

static float v3_dist_squared(v3 a, v3 b) {
	v3 diff = v3_sub(a, b);
	return v3_dot(diff, diff);
}

static v3 v3_normalize(v3 v) {
	float len = v3_len(v);
	float mul = len == 0.0f ? 1.0f : 1.0f/len;
	return v3_scale(v, mul);
}

static v2 v3_xy(v3 v) {
	return V2(v.x, v.y);
}

// a point on a unit sphere 
static v3 v3_on_sphere(float yaw, float pitch) {
	return V3(cosf(yaw) * cosf(pitch), sinf(pitch), sinf(yaw) * cosf(pitch));
}

static void v3_print(v3 v) {
	printf("(%f, %f, %f)\n", v.x, v.y, v.z);
}

static v3 v3_rand(void) {
	return V3(randf(), randf(), randf());
}

static v3 v3_rand_unit(void) {
	/*
		monte carlo method
		keep generating random points in cube of radius 1 (width 2) centered at origin,
		until you get a point in the unit sphere, then extend it to find the point lying
		on the sphere.
	*/
	while (1) {
		v3 v = V3(rand_uniform(-1.0f, +1.0f), rand_uniform(-1.0f, +1.0f), rand_uniform(-1.0f, +1.0f));
		float dist_squared_to_origin = v3_dot(v, v);
		if (dist_squared_to_origin <= 1 && dist_squared_to_origin != 0.0f) {
			return v3_scale(v, 1.0f / sqrtf(dist_squared_to_origin));
		}
	}
	return V3(0, 0, 0);
}

#if MATH_GL
static void v3_rand_unit_test(void) {
	int i;
	glColor3f(1.0f, 0.0f, 0.0f);
	glBegin(GL_POINTS);
	for (i = 0; i < 100000; ++i) {
		v3_gl_vertex(v3_rand_unit());
	}
	glEnd();
}
#endif

typedef struct {
	float x, y, z, w;
} v4;

static v4 const v4_zero = {0, 0, 0, 0};

static v4 V4(float x, float y, float z, float w) {
	v4 v;
	v.x = x;
	v.y = y;
	v.z = z;
	v.w = w;
	return v;
}

#if MATH_GL
static void v4_gl_color(v4 v) {
	glColor4f(v.x, v.y, v.z, v.w);
}
#endif

static v4 v4_add(v4 a, v4 b) {
	return V4(a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w);
}

static v4 v4_sub(v4 a, v4 b) {
	return V4(a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w);
}

static v4 v4_scale(v4 v, float s) {
	return V4(v.x * s, v.y * s, v.z * s, v.w * s);
}

static v4 v4_scale_xyz(v4 v, float s) {
	return V4(v.x * s, v.y * s, v.z * s, v.w);
}

static v4 v4_lerp(float x, v4 a, v4 b) {
	return V4(lerpf(x, a.x, b.x), lerpf(x, a.y, b.y), lerpf(x, a.z, b.z), lerpf(x, a.w, b.w));
}

static float v4_dot(v4 u, v4 v) {
	return u.x*v.x + u.y*v.y + u.z*v.z + u.w*v.w;
}

// create a new vector by multiplying the respective components of u and v 
static v4 v4_mul(v4 u, v4 v) {
	return V4(u.x * v.x, u.y * v.y, u.z * v.z, u.w * v.w);
}

static float v4_len(v4 v) {
	return sqrtf(v4_dot(v, v));
}

static v4 v4_normalize(v4 v) {
	float len = v4_len(v);
	float mul = len == 0.0f ? 1.0f : 1.0f/len;
	return v4_scale(v, mul);
}

static v3 v4_xyz(v4 v) {
	return V3(v.x, v.y, v.z);
}

static v4 v4_rand(void) {
	return V4(randf(), randf(), randf(), randf());
}

static void v4_print(v4 v) {
	printf("(%f, %f, %f, %f)\n", v.x, v.y, v.z, v.w);
}


// matrices are column-major, because that's what they are in OpenGL 
typedef struct {
	float e[16];
} m4;

static m4 const m4_identity = {{
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, 1, 0,
	0, 0, 0, 1
}};

static void m4_print(m4 m) {
	int i;
	for (i = 0; i < 4; ++i)
		printf("[ %f %f %f %f ]\n", m.e[i], m.e[i+4], m.e[i+8], m.e[i+12]);
	printf("\n");
}

static m4 M4(
	float a, float b, float c, float d,
	float e, float f, float g, float h,
	float i, float j, float k, float l,
	float m, float n, float o, float p) {
	m4 ret;
	float *x = ret.e;
	x[0] = a; x[4] = b; x[ 8] = c; x[12] = d;
	x[1] = e; x[5] = f; x[ 9] = g; x[13] = h;
	x[2] = i; x[6] = j; x[10] = k; x[14] = l;
	x[3] = m; x[7] = n; x[11] = o; x[15] = p;
	return ret;
}

// see https://en.wikipedia.org/wiki/Rotation_matrix#General_rotations 
static m4 m4_yaw(float yaw) {
	float c = cosf(yaw), s = sinf(yaw);
	return M4(
		c, 0, -s, 0,
		0, 1,  0, 0,
		s, 0,  c, 0,
		0, 0,  0, 1
	);
}

static m4 m4_pitch(float pitch) {
	float c = cosf(pitch), s = sinf(pitch);
	return M4(
		1, 0,  0, 0,
		0, c, -s, 0,
		0, s,  c, 0,
		0, 0,  0, 1
	);
}

// https://en.wikipedia.org/wiki/Translation_(geometry) 
static m4 m4_translate(v3 t) {
	return M4(
		1, 0, 0, t.x,
		0, 1, 0, t.y,
		0, 0, 1, t.z,
		0, 0, 0, 1
	);
}

// multiply m by [v.x, v.y, v.z, 1] 
static v3 m4_mul_v3(m4 m, v3 v) {
	return v3_add(v3_scale(V3(m.e[0], m.e[1], m.e[2]), v.x), v3_add(v3_scale(V3(m.e[4], m.e[5], m.e[6]), v.y),
		v3_add(v3_scale(V3(m.e[8], m.e[9], m.e[10]), v.z), V3(m.e[12], m.e[13], m.e[14]))));
}

/*
4x4 perspective matrix.
fov - field of view in radians, aspect - width:height aspect ratio, z_near/z_far - clipping planes
math stolen from gluPerspective (https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/gluPerspective.xml)
*/
static m4 m4_perspective(float fov, float aspect, float z_near, float z_far) {
	float f = 1.0f / tanf(fov / 2.0f);
	return M4(
		f/aspect, 0, 0, 0,
		0, f, 0, 0,
		0, 0, (z_far+z_near) / (z_near-z_far), (2.0f*z_far*z_near) / (z_near-z_far),
		0, 0, -1, 0
	);
}

// windows.h defines near and far, so let's not use those 
static m4 m4_ortho(float left, float right, float bottom, float top, float near_, float far_) {
	float tx = -(right + left)/(right - left);
	float ty = -(top + bottom)/(top - bottom);
	float tz = -(far_ + near_)/(far_ - near_);
	return M4(
		2.0f / (right - left), 0, 0, tx,
		0, 2.0f / (top - bottom), 0, ty,
		0, 0, -2.0f / (far_ - near_), tz,
		0, 0, 0, 1
	);
}


static m4 m4_mul(m4 a, m4 b) {
	m4 prod = {0};
	int i, j;
	float *x = prod.e;
	for (i = 0; i < 4; ++i) {
		for (j = 0; j < 4; ++j, ++x) {
			float *as = &a.e[j];
			float *bs = &b.e[4*i];
			*x = as[0]*bs[0] + as[4]*bs[1] + as[8]*bs[2] + as[12]*bs[3];
		}
	}
	return prod;
}

static m4 m4_inv(m4 mat) {
	m4 ret;
	float *inv = ret.e;
	float *m = mat.e;

	inv[0] = m[5] * m[10] * m[15] - m[5] * m[11] * m[14] - m[9] * m[6] * m[15] + m[9] * m[7] * m[14] + m[13] * m[6] * m[11] - m[13] * m[7] * m[10];
	inv[4] = -m[4] * m[10] * m[15] + m[4] * m[11] * m[14] + m[8] * m[6] * m[15] - m[8] * m[7] * m[14] - m[12] * m[6] * m[11] + m[12] * m[7] * m[10];
	inv[8] = m[4] * m[9] * m[15] - m[4] * m[11] * m[13] - m[8] * m[5] * m[15] + m[8] * m[7] * m[13] + m[12] * m[5] * m[11] - m[12] * m[7] * m[9];
	inv[12] = -m[4] * m[9] * m[14] + m[4] * m[10] * m[13] + m[8] * m[5] * m[14] - m[8] * m[6] * m[13] - m[12] * m[5] * m[10] + m[12] * m[6] * m[9];
	inv[1] = -m[1] * m[10] * m[15] + m[1] * m[11] * m[14] + m[9] * m[2] * m[15] - m[9] * m[3] * m[14] - m[13] * m[2] * m[11] + m[13] * m[3] * m[10];
	inv[5] = m[0] * m[10] * m[15] - m[0] * m[11] * m[14] - m[8] * m[2] * m[15] + m[8] * m[3] * m[14] + m[12] * m[2] * m[11] - m[12] * m[3] * m[10];
	inv[9] = -m[0] * m[9] * m[15] + m[0] * m[11] * m[13] + m[8] * m[1] * m[15] - m[8] * m[3] * m[13] - m[12] * m[1] * m[11] + m[12] * m[3] * m[9];
	inv[13] = m[0] * m[9] * m[14] - m[0] * m[10] * m[13] - m[8] * m[1] * m[14] + m[8] * m[2] * m[13] + m[12] * m[1] * m[10] - m[12] * m[2] * m[9];
	inv[2] = m[1] * m[6] * m[15] - m[1] * m[7] * m[14] - m[5] * m[2] * m[15] + m[5] * m[3] * m[14] + m[13] * m[2] * m[7] - m[13] * m[3] * m[6];
	inv[6] = -m[0] * m[6] * m[15] + m[0] * m[7] * m[14] + m[4] * m[2] * m[15] - m[4] * m[3] * m[14] - m[12] * m[2] * m[7] + m[12] * m[3] * m[6];
	inv[10] = m[0] * m[5] * m[15] - m[0] * m[7] * m[13] - m[4] * m[1] * m[15] + m[4] * m[3] * m[13] + m[12] * m[1] * m[7] - m[12] * m[3] * m[5];
	inv[14] = -m[0] * m[5] * m[14] + m[0] * m[6] * m[13] + m[4] * m[1] * m[14] - m[4] * m[2] * m[13] - m[12] * m[1] * m[6] + m[12] * m[2] * m[5];
	inv[3] = -m[1] * m[6] * m[11] + m[1] * m[7] * m[10] + m[5] * m[2] * m[11] - m[5] * m[3] * m[10] - m[9] * m[2] * m[7] + m[9] * m[3] * m[6];
	inv[7] = m[0] * m[6] * m[11] - m[0] * m[7] * m[10] - m[4] * m[2] * m[11] + m[4] * m[3] * m[10] + m[8] * m[2] * m[7] - m[8] * m[3] * m[6];
	inv[11] = -m[0] * m[5] * m[11] + m[0] * m[7] * m[9] + m[4] * m[1] * m[11] - m[4] * m[3] * m[9] - m[8] * m[1] * m[7] + m[8] * m[3] * m[5];
	inv[15] = m[0] * m[5] * m[10] - m[0] * m[6] * m[9] - m[4] * m[1] * m[10] + m[4] * m[2] * m[9] + m[8] * m[1] * m[6] - m[8] * m[2] * m[5];

	float det = m[0] * inv[0] + m[1] * inv[4] + m[2] * inv[8] + m[3] * inv[12];

	if (det == 0) {
		memset(inv, 0, sizeof *inv);
	} else {
		det = 1 / det;

		for (int i = 0; i < 16; i++)
			inv[i] *= det;
	}

	return ret;
}
typedef struct {
	int x, y;
} v2i;

static v2i V2I(int x, int y) {
	v2i v;
	v.x = x;
	v.y = y;
	return v;
}

static bool rect_contains_point_v2(v2 pos, v2 size, v2 point) {
	float x1 = pos.x, y1 = pos.y, x2 = pos.x + size.x, y2 = pos.y + size.y,
		x = point.x, y = point.y;
	return x >= x1 && x < x2 && y >= y1 && y < y2;
}

static bool centered_rect_contains_point(v2 center, v2 size, v2 point) {
	return rect_contains_point_v2(v2_sub(center, v2_scale(size, 0.5f)), size, point);
}

typedef struct {
	v2 pos, size;
} Rect;

static Rect rect(v2 pos, v2 size) {
	Rect r;
	r.pos = pos;
	r.size = size;
	return r;
}

static Rect rect4(float x1, float y1, float x2, float y2) {
	assert(x2 >= x1);
	assert(y2 >= y1);
	return rect(V2(x1,y1), V2(x2-x1, y2-y1));
}

static Rect rect_centered(v2 center, v2 size) {
	Rect r;
	r.pos = v2_sub(center, v2_scale(size, 0.5f));
	r.size = size;
	return r;
}

static v2 rect_center(Rect r) {
	return v2_add(r.pos, v2_scale(r.size, 0.5f));
}

static bool rect_contains_point(Rect r, v2 point) {
	return rect_contains_point_v2(r.pos, r.size, point);
}

static float rect_x1(Rect r) { return r.pos.x; }
static float rect_y1(Rect r) { return r.pos.y; }
static float rect_x2(Rect r) { return r.pos.x + r.size.x; }
static float rect_y2(Rect r) { return r.pos.y + r.size.y; }

static void rect_coords(Rect r, float *x1, float *y1, float *x2, float *y2) {
	*x1 = r.pos.x;
	*y1 = r.pos.y;
	*x2 = r.pos.x + r.size.x;
	*y2 = r.pos.y + r.size.y;
}

static void rect_print(Rect r) {
	printf("Position: (%f, %f), Size: (%f, %f)\n", r.pos.x, r.pos.y, r.size.x, r.size.y);
}

#if MATH_GL
// must be rendering GL_QUADS to use these functions! 

static void rect_render(Rect r) {
	float x1 = r.pos.x, y1 = r.pos.y, x2 = x1 + r.size.x, y2 = y1 + r.size.y;
	glVertex2f(x1, y1);
	glVertex2f(x2, y1);
	glVertex2f(x2, y2);
	glVertex2f(x1, y2);
}

static void rect_render_border(Rect r, float border_radius) {
	float x1 = r.pos.x, y1 = r.pos.y, x2 = x1 + r.size.x, y2 = y1 + r.size.y;
	//float a = 0.3f; // for debugging

	//glColor4f(1,0,0,a);
	glVertex2f(x1+border_radius, y1-border_radius);
	glVertex2f(x1+border_radius, y1+border_radius);
	glVertex2f(x2+border_radius, y1+border_radius);
	glVertex2f(x2+border_radius, y1-border_radius);
	
	//glColor4f(0,1,0,a);
	glVertex2f(x1-border_radius, y2-border_radius);
	glVertex2f(x1-border_radius, y2+border_radius);
	glVertex2f(x2-border_radius, y2+border_radius);
	glVertex2f(x2-border_radius, y2-border_radius);
	
	//glColor4f(0,0,1,a);
	glVertex2f(x1-border_radius, y1-border_radius);
	glVertex2f(x1+border_radius, y1-border_radius);
	glVertex2f(x1+border_radius, y2-border_radius);
	glVertex2f(x1-border_radius, y2-border_radius);
	
	//glColor4f(1,1,0,a);
	glVertex2f(x2-border_radius, y1+border_radius);
	glVertex2f(x2+border_radius, y1+border_radius);
	glVertex2f(x2+border_radius, y2+border_radius);
	glVertex2f(x2-border_radius, y2+border_radius);
}


/* 
	gl grayscale color
	i am tired of not having this
*/
static void gl_color1f(float v) {
	glColor3f(v,v,v);
}

/* 
	gl grayscale+alpha color
*/
static void gl_color2f(float v, float a) {
	glColor4f(v,v,v,a);
}

// color is 0xRRGGBBAA
static void gl_rgbacolor(u32 color) {
	glColor4ub((u8)(color >> 24), (u8)(color >> 16), (u8)(color >> 8), (u8)color);
}

// color is 0xRRGGBB
static void gl_rgbcolor(u32 color) {
	gl_rgbacolor((color << 8) | 0xff);
}

static void gl_quad(float x1, float y1, float x2, float y2) {
	glVertex2f(x1, y1);
	glVertex2f(x2, y1);
	glVertex2f(x2, y2);
	glVertex2f(x1, y2);
}
#endif

