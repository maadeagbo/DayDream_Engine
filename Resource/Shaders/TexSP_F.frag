#version 430 core

// Frag output
layout (location = 0) out vec4 FragColor;

in VS_OUT {
	vec2 TexCoord;
} fs_in;

uniform sampler2D DepthTex;
uniform float near_plane;
uniform float far_plane;

float LinearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0; // Back to NDC 
    return (2.0 * near_plane * far_plane) / (far_plane + near_plane - z * (far_plane - near_plane));
}

void main() {             
    float depthValue = texture(DepthTex, fs_in.TexCoord).x;
    //FragColor = vec4(vec3(LinearizeDepth(depthValue) / far_plane), 1.0); // perspective
    FragColor = vec4(vec3(depthValue), 1.0); // orthographic
	//FragColor.rgb = vec3(pow((depthValue), 64));
	//FragColor.a = 1.0;
}  