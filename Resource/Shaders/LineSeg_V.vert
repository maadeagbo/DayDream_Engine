#version 430

layout (location = 0) in vec3 VertexPosition;

uniform mat4 MVP;

out vec4 FragPos;

void main() {
    FragPos = vec4(VertexPosition, 1.0);
    gl_Position = MVP * vec4(VertexPosition, 1.0);
}