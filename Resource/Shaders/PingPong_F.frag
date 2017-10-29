#version 430

// Frag output
layout (location = 0) out vec4 FragColor;

in VS_OUT {
	vec2 TexCoord;
} fs_in;

layout (binding = 0) uniform sampler2D ColorTex;

void main() {
	vec2 tex_coord = fs_in.TexCoord;
	vec4 color = texture( ColorTex, tex_coord );
    
    FragColor = color;
}