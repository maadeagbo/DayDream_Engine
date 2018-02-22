#version 430

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;
layout (location = 3) in vec3 TangentNormal;
layout (location = 4) in mat4 InstanceMatrix;
layout (location = 8) in vec3 InstanceColor;

uniform bool enable_clip1 = false;
uniform vec4 cplane_01;
uniform mat4 Model;
uniform mat4 VP;
uniform mat4 Norm; // for non-uniform scaling

out VS_OUT {
	vec2 TexCoord;
	vec2 Debug;
	vec3 FragPos;
	vec3 Normal;
	vec3 InstanceColor;
	mat3 TBN;
} vs_out;

uniform bool multiplierMat = false;

void main() {
	vs_out.TexCoord = VertexTexCoord;
	// cannot be used with instancing
	vs_out.Normal = normalize(vec3(Norm * vec4(VertexNormal, 0.0))); 
	// can be used with instancing
	//vs_out.Normal = normalize(vec3(InstanceMatrix * MVP * vec4(VertexNormal, 0.0)));
	vs_out.FragPos = vec3( InstanceMatrix * Model * vec4(VertexPosition, 1.0f));
	if (multiplierMat) {
		vs_out.InstanceColor = InstanceColor;
	}

	// normalize and create TBN matrix
	vec3 tanNorm = normalize(vec3( InstanceMatrix * Model * vec4(TangentNormal, 0.0)));
	vec3 norm = normalize( vec3( InstanceMatrix * Model * vec4(VertexNormal, 0.0)));
	vec3 bitanNorm = cross(norm, tanNorm);
	vs_out.TBN = mat3(tanNorm, bitanNorm, norm);
	
	vec4 wsPos = InstanceMatrix * VP * Model * vec4(VertexPosition, 1.0);
	gl_Position = wsPos;
	if (enable_clip1) {
		//gl_ClipDistance[0] = dot(cplane_01, wsPos);
	}
}