#version 110
// "position" within line (0,0) = bottom-left corner, (1,1) = top-right corner
attribute vec2 vertex_p1, vertex_p2;
varying vec4 color;
varying vec2 p1, p2;
varying vec2 pos;
uniform float thickness;
uniform mat4 transform;

void main() {
	gl_Position = transform * gl_Vertex;
	pos = gl_Vertex.xy;
	color = gl_Color;
#if 1
	p1 = vertex_p1 + thickness * normalize(vertex_p2 - vertex_p1);
	p2 = vertex_p2 + thickness * normalize(vertex_p1 - vertex_p2);
#else
	p1 = vertex_p1;
	p2 = vertex_p2;
#endif
}
