#version 430

// Frag output
layout( location = 0 ) out vec4 FragColor;
layout( location = 1 ) out vec4 OutColor;

uniform sampler2D ClothTex;

in vec4 Normal;
in vec4 mvpPos;
in vec2 TexCoord;

void main() {
    vec4 color = texture(ClothTex, TexCoord);
    if (color.a <= 0.01) {
		discard;
	}

    vec3 lightDir = vec3(0.0) - vec3(1000.0f, 1000.f, 2000.0f);
    lightDir = normalize(lightDir);
    vec3 viewDir = normalize(vec3(-mvpPos)); 

    vec3 ambient =  vec3(color) * 0.1;
	// diffuse
	float kd = max(dot(vec3(Normal), lightDir), 0.0);
	vec3 diffuse = vec3(color) * kd;
	// specular
	vec3 halfDir = normalize(lightDir + viewDir);
	float ks = pow(max(dot(vec3(Normal), halfDir), 0.0), 40.0);
	vec3 spec = vec3(ks * 0.1);

    OutColor = vec4((ambient + diffuse + spec), 1.0);

    //OutColor = vec4(TexCoord, 1.0, 1.0);
    //OutColor = vec4(vec3(0.5), 1.0);
}