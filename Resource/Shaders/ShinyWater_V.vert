#version 430

layout (location = 0) in vec4 VertexPosition;
layout (location = 1) in vec4 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;

uniform mat4 MVP;

out vec3 Normal;
out vec3 FragPos;
out vec2 TexCoord;
out mat3 TBN;

void main() {
	Normal = vec3(VertexNormal);
	gl_Position = MVP * VertexPosition;
	FragPos = vec3(VertexPosition);
    TexCoord = VertexTexCoord;

    vec3 tanNorm = vec3(1.0, 0.0, 0.0);
	vec3 norm = normalize(vec3(VertexNormal));
	vec3 bitanNorm = cross(norm, tanNorm);
	TBN = mat3(tanNorm, bitanNorm, norm);
}