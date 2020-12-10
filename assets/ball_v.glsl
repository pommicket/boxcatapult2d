#version 110
varying vec4 color;
varying vec2 pos;
uniform mat4 transform;

void main() {
	gl_Position = transform * gl_Vertex;
	pos = gl_Vertex.xy;
	color = gl_Color;
}
