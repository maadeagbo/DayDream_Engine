#version 430

layout( location = 0 ) out vec4 FragColor;
layout( location = 1 ) out vec4 OutColor;

uniform vec4 color;

void main() {
    //OutColor = texture(Tex01, out_uv);
    OutColor = color;
}