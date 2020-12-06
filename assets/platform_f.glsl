varying vec4 color;
varying vec2 p1, p2;
varying vec2 pos;
uniform float thickness;

void main() {
	// thanks to https://www.youtube.com/watch?v=PMltMdi1Wzg
	float h = clamp(dot(pos-p1, p2-p1) / dot(p2-p1, p2-p1), 0.0, 1.0);
	float d = length(pos - p1 - (p2-p1) * h);
	
	float v = max(thickness - d, 0.0);
	v /= thickness;

	gl_FragColor = color * v;
}
