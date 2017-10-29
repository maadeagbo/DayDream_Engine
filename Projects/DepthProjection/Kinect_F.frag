#version 430

layout( location = 0 ) out vec4 FragColor;
layout( location = 1 ) out vec4 OutColor;

uniform sampler2D Tex01;
in vec2 out_uv;

void main() {
    OutColor = texture(Tex01, out_uv);
    //OutColor = vec4(1.0);
}