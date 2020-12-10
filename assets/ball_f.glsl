#version 110
varying vec4 color;
varying vec2 pos;
uniform vec2 center;
uniform float radius;

void main() {
	float dist_squared = dot(pos - center, pos - center);
	float threshold = 0.8 * radius; // radius border starts at
	float thickness = radius - threshold;

	if (dist_squared > threshold * threshold && dist_squared < radius * radius) {
		float dist = sqrt(dist_squared);
		float v = (dist - threshold) / thickness;
		v = 2.0 * v - 1.0;
		gl_FragColor = color * (1.0 - v * v);
	} else {
		discard;
	}
}
