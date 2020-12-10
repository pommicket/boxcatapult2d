#version 110
varying vec4 color;
varying vec2 p1, p2;
varying vec2 pos;
uniform float thickness;

void main() {
	// thanks to https://www.youtube.com/watch?v=PMltMdi1Wzg
	// (calculates distance to line segment p1-p2)
	float h = clamp(dot(pos-p1, p2-p1) / dot(p2-p1, p2-p1), 0.0, 1.0);
	float d = length(pos - p1 - (p2-p1) * h);
	
	d /= thickness;
	float v = 1.0 - d * d;

	gl_FragColor = color * v;
}
