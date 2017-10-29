#version 430

layout (location = 0) in vec4 VertexPosition;
layout (location = 1) in vec4 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;

uniform mat4 MVP;

out vec4 Normal;
out vec4 mvpPos;
out vec2 TexCoord;

void main() {
	TexCoord = VertexTexCoord;
	Normal = VertexNormal;
	gl_Position = MVP * VertexPosition;
	mvpPos = gl_Position;
}