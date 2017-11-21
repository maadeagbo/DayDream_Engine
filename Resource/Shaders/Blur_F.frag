#version 430

// Frag output
layout (location = 0) out vec4 OutColor4;
layout (location = 1) out vec2 OutColor2;

in VS_OUT {
	vec2 TexCoord;
	vec4 LightSpaceCoord;
} fs_in;

layout (binding = 0) uniform sampler2D InputTex;

uniform bool shadow = false;
uniform bool blur = false;
uniform vec2 direction_flag;

const int stride_11 = 5;
const float kernel_11[] = float[11] (
    0.000003, 0.000229, 0.005977, 0.060598, 0.24173,
    0.382925,
    0.24173, 0.060598, 0.005977, 0.000229, 0.000003
);

const int stride_21 = 10;
const float kernel_21[] = float[21] (
    0.000272337, 0.00089296, 0.002583865, 0.00659813, 0.014869116,
    0.029570767, 0.051898313, 0.080381679, 0.109868729, 0.132526984, 
    0.14107424,
    0.132526984, 0.109868729, 0.080381679, 0.051898313, 0.029570767,
    0.014869116, 0.00659813, 0.002583865, 0.00089296, 0.000272337
);

void blur_image() {
    //
}

void main() {
	vec2 tex_coord = fs_in.TexCoord;
    //OutColor4 = vec4(1.0);
    OutColor2 = vec2(1.0);
}