#version 430

// Frag output
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec3 PositionData;
layout (location = 2) out vec3 NormalData;
layout (location = 3) out vec4 ColorData;

in vec4 FragPos;
uniform vec4 color;

void main() {
    PositionData = FragPos.xyz;
    ColorData = color;
    NormalData = vec3(0.0, 1.0, 0.0);
}