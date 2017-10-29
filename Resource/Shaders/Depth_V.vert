#version 430

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;
layout (location = 3) in vec3 TangentNormal;
layout (location = 4) in mat4 InstanceMatrix;

uniform mat4 MVP;
uniform mat4 LightSpace;

void main() {
    gl_Position = LightSpace * MVP * InstanceMatrix * vec4(VertexPosition, 1.0f);
}  
