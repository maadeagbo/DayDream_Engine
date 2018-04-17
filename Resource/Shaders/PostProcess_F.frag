#version 430

// Frag output
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec2 FragColor2;

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
uniform bool SampleMap = false;
uniform bool Blur = false;
uniform bool GammaCorrect = false;
uniform bool BlendParticle = false;
uniform bool output2D = false;

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

uniform vec2 direction_flag;
const int stride_11 = 5;
const float kernel_11[] = float[11] (
    0.000003, 0.000229, 0.005977, 0.060598, 0.24173,
    0.382925,
    0.24173, 0.060598, 0.005977, 0.000229, 0.000003
);

const int stride_21 = 10;
const float kernel_21[] = float[21] (
    0.000272337, 0.00089296, 0.002583865, 0.00659813, 0.014869116,
    0.029570767, 0.051898313, 0.080381679, 0.109868729, 0.132526984, 
    0.14107424,
    0.132526984, 0.109868729, 0.080381679, 0.051898313, 0.029570767,
    0.014869116, 0.00659813, 0.002583865, 0.00089296, 0.000272337
);

vec4 BlurImage(const vec2 mask, const vec2 uv) {
	vec2 delta = 1.0/textureSize(ColorTex, 0);
	vec4 color_out = vec4(0.0);
	int index = 0;
	int stride = stride_21;

	for (int i = -stride; i <= stride; i++) {
		vec2 offset = vec2(i * delta.x * mask.x, i * delta.y * mask.y);
		vec4 source = (output2D) ? 
			vec4(texture(ColorTex, uv + offset).xy, 0.0, 1.0) : 
			texture(ColorTex, uv + offset);
		color_out += kernel_21[index] * source;
		index += 1;
	}
	return color_out;
}

void main() {
	vec2 tex_coord = fs_in.TexCoord;
	vec4 finalColor;
	
	finalColor = (output2D) ? 
		vec4(texture( ColorTex, tex_coord ).xy, 0.0, 0.0) : 
		texture( ColorTex, tex_coord );
    
	// apply tone mapping
	if (DoToneMap) {
		finalColor = ToneMap(finalColor, Exposure, AveLum, White);
	}
	 
	// apply gamma correction
	if (GammaCorrect) {
		float gamma = 2.2;
		finalColor.rgb = pow(finalColor.rgb, vec3(1.0/gamma));
	}

	// apply blur filter
	if (Blur) {
		finalColor = BlurImage(direction_flag, tex_coord);
	}

	if(SampleShadow) {
		float shadow = texture( ColorTex, tex_coord ).r;
		finalColor = vec4( vec3(shadow), 1.0);
	}

	if(SampleMap) {
		finalColor = texture( ColorTex, tex_coord );
	}

	// replace particle colors if present (remove pure black background)
	if (BlendParticle) {
		vec4 particleColor = texture( ParticleTex, tex_coord );

		bvec4 bg = equal(vec4(vec3(0.0), 1.0), particleColor);
		float alphaTest = step(0.01, particleColor.a);
		float bgTest = 1.0 - float(bg.x && bg.y && bg.z && bg.w);
		// replace
		FragColor = mix(
			finalColor, particleColor, bgTest * alphaTest * particleColor.a);
		// additive
		//FragColor = particleColor;
		//FragColor = mix(particleColor, finalColor, 0);
	}
	else {
		if (output2D) { // output to FragColor2
			FragColor2 = finalColor.xy;
		}
		else { 			// output to FragColor
			FragColor = finalColor;
		}
	}
}