#version 430

layout (location = 0) out vec2 FragColor;

in VS_OUT {
	vec4 FragPos;
} fs_in;

void main() 
{
	// perform perspective divide
	vec3 pos = fs_in.FragPos.xyz/fs_in.FragPos.w;
	//pos.z += 0.001; // shadow bias

	float depth = (pos.z + 1) * 0.5; // Transform to [0,1] range
	float moment1 = depth;
	float moment2 = depth * depth;
	
	FragColor = vec2(moment1, moment2);
} 