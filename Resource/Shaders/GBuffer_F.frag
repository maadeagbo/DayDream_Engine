#version 430

// Frag output
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec3 PositionData;
layout (location = 2) out vec3 NormalData;
layout (location = 3) out vec4 ColorData;

in VS_OUT {
	vec2 TexCoord;
	vec2 Debug;
	vec3 FragPos;
	vec3 Normal;
	vec3 InstanceColor;
	mat3 TBN;
} fs_in;

uniform sampler2D tex_albedo;
uniform sampler2D tex_specular;
uniform sampler2D tex_normal;
uniform vec3 diffuse = vec3(0.5);
uniform float shininess = 0.0;

uniform bool albedoFlag = false;
uniform bool specFlag = false;
uniform bool normalFlag = false;

uniform bool multiplierMat = false;

uniform bool useDebug = false;

void main() {	
	// store values to G buffer
    vec3 color;
    vec3 spec;
    vec3 normal;
    // normal
	if (normalFlag) {
		normal = texture( tex_normal, fs_in.TexCoord ).rgb;
		normal = normalize(normal * 2.0 - 1.0);
		normal = normalize(fs_in.TBN * normal);
	} else {
		normal = fs_in.Normal;
	}
    // albedo
    if (albedoFlag) {
	    color = texture( tex_albedo, fs_in.TexCoord ).rgb;
		if (multiplierMat) {
			color = color * fs_in.InstanceColor;
		}
    } else {
        color = diffuse;
		if (multiplierMat) {
			color = color * fs_in.InstanceColor;
		}
    }
	// gamma correction
	float gamma = 2.2;
	color = pow(color, vec3(gamma));

    // specular
    if (specFlag) {
        spec = texture( tex_specular, fs_in.TexCoord ).rgb; 
    } else {
        spec = vec3(shininess);
    }
    
    PositionData = fs_in.FragPos;
	ColorData = vec4(color, spec.r);
	if (useDebug) { ColorData = vec4(fs_in.Debug.y, 0.0, 0.0, 1.0); }
    NormalData = normal;
}