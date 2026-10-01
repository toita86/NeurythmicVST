#version 150

in vec4 in_pos;
in vec4 vertex_color;

uniform mat4 modelViewProjectionMatrix;

out vec4 v_color;

void main(){
	gl_Position = modelViewProjectionMatrix * in_pos;
	v_color = vertex_color;
}
