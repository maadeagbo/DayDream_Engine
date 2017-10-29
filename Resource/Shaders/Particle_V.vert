#version 430

layout (location = 0) in vec4 VertexPosition;
layout (location = 1) in vec4 VertexVelocity;
layout (location = 2) in vec4 VertexColor;
layout (location = 3) in float VertexLife;

uniform mat4 MV;
uniform mat4 Proj;

out vec4 V_Color;
out vec4 V_Velocity;
out float V_Life;

void main() {
    gl_Position = MV * VertexPosition;
    V_Color = VertexColor;
    V_Velocity = VertexVelocity;
    V_Life = VertexLife;
}