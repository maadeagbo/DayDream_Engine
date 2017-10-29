#version 430

layout( location = 0 ) out vec4 FragColor;
layout( location = 1 ) out vec4 OutColor;

// from Geometry shader
in vec2 TexCoord;
in vec4 G_Color;
in vec4 G_Velocity;
in float G_Life;

uniform sampler2D ParticleTex01;
uniform sampler2D ParticleTex02;
uniform float EmitterLife;

uniform bool texFlag01 = false;
uniform bool fireFlag = false;

void main() {
	vec4 OGcolor = G_Color;
	vec4 color = G_Color;
	if (texFlag01) {
		color = texture(ParticleTex01, TexCoord);
		// alpha channel on texture
		if (color.a <= 0.01) {
			discard;
		}
		//color.rbg *= OGcolor.rgb; 

		// Kill alpha based on life time
		color.a = (G_Life / EmitterLife);
	}
	if (fireFlag) {
		// make flames white near bottom
		float lifeF = (G_Life / EmitterLife);
		float wF = step(0.8, lifeF);
		wF = mix(0.0, 1.0, wF);
		color.rgb += vec3(1.0, 0.0, 0.0) * wF;
		// make flames black near top
		float rF = step(lifeF, 0.3);
		rF = mix(0.0, 1.0, rF);
		if (wF < 0.1 && rF > 0.99) {
			//color.rgb *= vec3(1.0, 0.5, 0.5);
			color *= vec4(0.2, 0.2, 0.2, 1.0);
			//color = texture(ParticleTex02, TexCoord);
		}
		
		color *= OGcolor;
		// Kill alpha based on life time
		float lF = step(0.01, lifeF);
		color.a = lF * color.a;
	}

	// gamma correction
	//float gamma = 2.2;
	//color.rgb = pow( color.rgb, vec3(gamma));
	
	// alpha channel
	if (color.a <= 0.1) {
		discard;
	}

	OutColor = color;
}