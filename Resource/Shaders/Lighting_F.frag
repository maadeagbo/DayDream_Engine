#version 430

// Frag output
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 OutColor;

// in GS_OUT {
// 	vec2 TexCoord;
// 	vec3 SkyCoord;
// } fs_in;
in vec3 SkyCoord;

// G-Buffer textures
layout (binding = 0) uniform sampler2D PositionTex;
layout (binding = 1) uniform sampler2D ColorTex;
layout (binding = 2) uniform sampler2D NormalTex;
// Shadow Map textures
layout (binding = 3) uniform sampler2D DepthTex;
// Skybox textures
layout (binding = 4) uniform samplerCube skybox;

struct LightInfo {
	vec3 position;
	vec3 direction;
	vec3 color;
	int type;

	float cutoff_i;
	float cutoff_o;
	float spotExponent;

	float lumin_cutoff;
	float linear;
	float quadratic;
	float lumin_r709;
};

uniform LightInfo Light;
uniform mat4 LSM;
uniform vec3 viewPos;
uniform vec2 screenDimension;
uniform bool DrawSky;
uniform bool ShadowMap;

uniform bool Debug;
uniform vec4 Debug_Color;

float ShadowCalculation(vec4 lightSpace, vec3 normal, vec3 lightDir) {
	// perform perspective divide
	vec3 projCoords = lightSpace.xyz / lightSpace.w;
	// Transform to [0,1] range
	projCoords = projCoords * 0.5 + 0.5;

	//* variance shadow map (taken from Nvidia paper)
	// compares currentDepth value to a distibution of values using Chebyshev's
	// inequality. Chebyshev's inequality gives a bound on that percentage of 
	// of values given the average (expected value E(x)) and a variance
	float currentDepth = projCoords.z;
	vec2 moments = texture(DepthTex, projCoords.xy).xy;
	float E_x2 = moments.y; // average squared value over region
	float Ex_2 = moments.x * moments.x;	// average value over region (squared)
	float var = E_x2 - Ex_2; // variance = E(x2) - (E(x) * E(x))
	var = max(var, 0.00002);
	float mD = currentDepth - moments.x;
	float mD_2 = mD * mD;
	float p_max = var / (var + mD_2); // probability of occlusion
	// high variance occurs at silhouette edges. Use p_max to drive the equation
	// towards using the variance or using a "minimum" (o.2 for light bleed)
	float shadow = max(p_max, (currentDepth <= moments.x) ? 1.0 : 0.2);
	// light bleed can occur at high depth / variance values (need to implement
	// cascaded shadow maps to control)
	//*/

	/*
	// PCF soft shadows
	float closestDepth = texture(DepthTex, projCoords.xy).r;
	float currentDepth = projCoords.z;
	// Check whether current frag pos is in shadow
	// bias serves to get rid of moire pattern by lifting up the shadow off the surface
	float bias = max(0.0043 * (1.0 - dot(normal, lightDir)), 0.0028); 
	// PCF
	float shadow = 0.0;
	vec2 texelSize = 1.0 / textureSize(DepthTex, 0); // sample texture at native resolution
	
	// 9 x 9 grid per pixel
	for(int x = -1; x <= 1; ++x) {
		for(int y = -1; y <= 1; ++y) {
			float pcfDepth = texture(DepthTex, projCoords.xy + vec2(x, y) * texelSize).r; 
			shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
		}    
	}
	shadow /= 9.0;
	shadow = (1.0 - shadow);
	//*/

	// Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
	if(projCoords.z > 1.0) {
		shadow = 1.0;
	}
	
	//return shadow;
	return shadow;
}

vec4 pointSpotLightModel( vec3 lightDir, vec3 viewDir, vec3 norm, vec4 albedo, 
													float _distance, float spec_val ) {
	vec3 ambient =  vec3(albedo) * 0.1;
	// diffuse
	float kd = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = Light.color * vec3(albedo) * kd;
	// specular
	vec3 halfDir = normalize(lightDir + viewDir);
	float ks = pow(max(dot(norm, halfDir), 0.0), 40.0);
	vec3 spec = Light.color * ks * spec_val;
	
	float spot = 1.0;//, intensity = 1.0;

	// Attenuation
	float attenuation = 1.0 / pow((_distance + 1), 2);
	attenuation = (attenuation - Light.lumin_cutoff) / (1.0 - Light.lumin_cutoff);
	attenuation = max(attenuation, 0);
	
	// spot light calc
	if ( Light.type > 1 ) {
		// spot light inner outer/ rings for fading effect
		float theta = dot(lightDir, normalize(-Light.direction) );	
		float epsilon = Light.cutoff_i - Light.cutoff_o;
		//intensity = pow( theta, Light.spotExponent);
	
		//****************************************************
		// Don't need dynamic branching at all, precompute 
		// falloff(i will call it spot)
		spot = clamp(((theta - Light.cutoff_o)/epsilon), 0.0, 1.0);
		//****************************************************
	}

	// float attenuation = 1.0f / (Light.constant + Light.linear * _distance + Light.quadratic * 
	// 	(_distance * _distance)); 
	diffuse *= attenuation; 
	ambient *= attenuation; 
	spec *= attenuation; 
	
	return vec4((diffuse * spot + ambient + spec * spot), albedo.a);

	//return vec4((diffuse * spot * intensity + ambient * intensity + spec * spot * 
	//						intensity), albedo.a);
}

vec4 directionLightModel( vec3 lightDir, vec3 viewDir, vec3 norm, vec4 albedo, 
	vec3 color, float shadow, float spec_val) 
{
	vec3 ambient =  vec3(albedo) * 0.1;
	// diffuse
	float kd = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = color * vec3(albedo) * kd;
	// specular
	vec3 halfDir = normalize(lightDir + viewDir);
	float ks = pow(max(dot(norm, halfDir), 0.0), 20.0);
	vec3 spec = color * ks * spec_val;

	return vec4((ambient + (diffuse + spec) * shadow), albedo.a);
	//return vec4(vec3(shadow), 1.0);
}

void main() {
	vec2 tex_coord = gl_FragCoord.xy / screenDimension;

	vec3 pos = vec3( texture( PositionTex, tex_coord ) );
	vec3 norm = vec3( texture( NormalTex, tex_coord ) );
	float spec = texture( NormalTex, tex_coord ).a;
	vec4 albedo = texture( ColorTex, tex_coord );
	//vec4 albedo = vec4(0.5, 0.5, 0.5, 1.0);
	vec3 viewDir = normalize(viewPos - pos);
	vec3 lightDir = vec3(0.0);
	// uniform braching to select light
	vec4 finalColor = vec4(0.0);

	if ( DrawSky ) {
		// sky box
		vec4 skyboxColor = texture( skybox, SkyCoord );
		vec3 color = skyboxColor.xyz;
		// gamma correction
		float gamma = 2.2;
		color = pow(color, vec3(gamma));

		OutColor = vec4(color, skyboxColor.w);
	} else {
		// directional lighting
		if ( Light.type == 0 ) {
			lightDir = -normalize( Light.direction );
			// shadow calc
			float shadow = 1.0f;
			if (ShadowMap) {
				vec4 PositionLS = LSM * vec4(pos, 1.0f);
				shadow = ShadowCalculation(PositionLS, norm, lightDir);
				//finalColor = vec4(texture( DepthTex, tex_coord ).r);
			}
			finalColor = directionLightModel( lightDir, viewDir, norm, 
				albedo, Light.color, shadow, spec);
		} else {
			// point and spot light
			vec3 _dist = Light.position - pos;
			lightDir = normalize(_dist);
			finalColor = pointSpotLightModel( lightDir, viewDir, norm, albedo, 
				length(_dist), spec);
		}

		OutColor = finalColor;
		//OutColor = vec4(1.0, 0.0, 0.0, 1.0);
		//OutColor = vec4(norm, 1.0);
		//OutColor = albedo;
	}
	
	if ( Debug ) {
		OutColor = Debug_Color;
	}
}