#version 430

// Frag output
layout (location = 0) out vec4 FragColor;

in VS_OUT {
	vec2 TexCoord;
	vec4 LightSpaceCoord;
} fs_in;

layout (binding = 0) uniform sampler2D ColorTex;
layout (binding = 1) uniform sampler2D ParticleTex;

uniform mat3 rgb2xyz = mat3( 
	0.4124564, 0.2126729, 0.0193339,
	0.3575761, 0.7151522, 0.1191920,
	0.1804375, 0.0721750, 0.9503041 );

uniform mat3 xyz2rgb = mat3(
	3.2404542, -0.9692660, 0.0556434,
	-1.5371385, 1.8760108, -0.2040259,
	-0.4985314, 0.0415560, 1.0572252 );

uniform float AveLum;
uniform float Exposure;
uniform float White;
uniform bool DoToneMap = true;
uniform bool SampleShadow = false;

vec4 ToneMap(vec4 gammaColor, float expos, float avgLuminance, float whiteCol) {
	// Convert to XYZ
	vec3 xyzCol = rgb2xyz * vec3(gammaColor);

	// Convert to xyY
	float xyzSum = xyzCol.x + xyzCol.y + xyzCol.z;
	vec3 xyYCol = vec3(0.0);
	if (xyzSum > 0.0) { // avoid divide by zero
		xyYCol = vec3( xyzCol.x / xyzSum, xyzCol.y / xyzSum, xyzCol.y);
	}

	// Apply the tone mapping operation to the luminance (xyYCol.z or xyzCol.y)
	float L = (expos * xyYCol.z) / avgLuminance;
	L = (L * ( 1 + L / (whiteCol * whiteCol) )) / ( 1 + L );

	// Using the new luminance, convert back to XYZ
	if (xyzCol.y > 0.0) { // avoid divide by zero
		xyzCol.x = (L * xyYCol.x) / (xyYCol.y);
		xyzCol.y = L;
		xyzCol.z = (L * (1 - xyYCol.x - xyYCol.y))/xyYCol.y;
	}
	
	return vec4( xyz2rgb * xyzCol, gammaColor.a );
}

void main() {
	vec2 tex_coord = fs_in.TexCoord;
	vec4 finalColor = texture( ColorTex, tex_coord );
	vec4 particleColor = texture( ParticleTex, tex_coord );
	 
	// apply gamma correction
    float gamma = 2.2;
	finalColor.rgb = pow(finalColor.rgb, vec3(1.0/gamma));
	//particleColor.rgb = pow(particleColor.rgb, vec3(1.0/gamma));

	if (DoToneMap) {
		finalColor = ToneMap(finalColor, Exposure, AveLum, White);
		//particleColor = ToneMap(particleColor, Exposure, AveLum, White);
	}

	if(SampleShadow) {
		// perform perspective divide
		vec3 projCoords = fs_in.LightSpaceCoord.xyz / fs_in.LightSpaceCoord.w;
		// Transform to [0,1] range
		projCoords = projCoords * 0.5 + 0.5;

		float shadow = texture( ColorTex, projCoords.xy ).r;
		finalColor = vec4( vec3(shadow), 1.0);
	}

	// replace particle colors if present (remove pure black background)
	bvec4 bg = equal(vec4(vec3(0.0), 1.0), particleColor);
	float alphaTest = step(0.01, particleColor.a);
	float bgTest = 1.0 - float(bg.x && bg.y && bg.z && bg.w);
    
	// replace
	FragColor = mix(finalColor, particleColor, bgTest * alphaTest * particleColor.a);
	// additive
	//FragColor = particleColor;
	//FragColor = mix(particleColor, finalColor, test);
}