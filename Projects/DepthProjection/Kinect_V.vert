#version 430

layout (location = 0) in vec4 VertexPosition;

uniform mat4 MV;
uniform mat4 Proj;

uniform mat4x3 colorKRT = mat4x3(
    969.97049608000, 10.33259931000, 0.00660000000,
    -4.55400488000, 972.64763118000, 0.00210000000,
    945.57846118000, 577.35579511000, 1.00000000000,
    -49703.81680447507, 1172.97384615694, 2.10059033000
);
uniform mat4 upsideDown;

out vec2 calc_uv;

void main() {
    gl_Position = MV * upsideDown * VertexPosition;

    // calculate uv
    vec4 pos = vec4(VertexPosition.x, VertexPosition.yzw);
    pos.w = 1.0;
    vec3 temp_uv = colorKRT * pos;
    vec3 uv = temp_uv / temp_uv.z;
    uv.x = uv.x/1920.0;
    uv.y = uv.y/1080.0;

    uv.x = 1.0 - uv.x;      // flip color image

    calc_uv = vec2(uv.xy);
}