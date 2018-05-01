#version 430

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;
layout (location = 3) in vec3 TangentNormal;
layout (location = 4) in vec4 BlendWeight;
layout (location = 5) in vec4 JointIndex;

layout (location = 6) in mat4 InstanceMatrix;
layout (location = 10) in vec3 InstanceColor;
layout ( std430, binding = 11 ) buffer JNT { mat4 Joints[]; };

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

vec4 getSkinnedVec4(const vec4 in_v, const uint jnt_idx)
{
	const uint idx = uint(JointIndex[jnt_idx]);
    return (Joints[idx] * in_v) * BlendWeight[jnt_idx];
}

void main() {
	vs_out.TexCoord = VertexTexCoord;
	// cannot be used with instancing (normally used w/ non-uniform scaling)
    // leads to incorrect/flickering normals
	// vs_out.Normal = normalize(vec3(Norm * vec4(VertexNormal, 0.0))); 

    // calculate and apply skinning transformations
    vec4 v_pos = vec4(0.0);
    vec4 v_norm = vec4(0.0);
    vec4 v_tan = vec4(0.0);
    for (uint i = 0; i < 4; i++) {
        v_pos = getSkinnedVec4( vec4(VertexPosition, 1.0), i) + v_pos;
        v_norm = getSkinnedVec4( vec4(VertexNormal, 0.0), i) + v_norm;
        v_tan = getSkinnedVec4( vec4(TangentNormal, 0.0), i) + v_tan;
    }

	// normalize and create TBN matrix
	vec3 tanNorm = normalize(InstanceMatrix * Model * v_tan ).xyz;
	vec3 norm = normalize(InstanceMatrix * Model * v_norm ).xyz;
	vec3 bitanNorm = cross(norm, tanNorm);

    // output to fragment shader
	vs_out.Normal = norm;
	vs_out.FragPos = (InstanceMatrix * Model * v_pos).xyz;
	vs_out.TBN = mat3(tanNorm, bitanNorm, norm);
	if (multiplierMat) { vs_out.InstanceColor = InstanceColor; }

	//vs_out.Debug = vec2( JointIndex.x/10.0, BlendWeight[2]);
	//vs_out.Debug = VertexTexCoord;

    // worldspace position
	gl_Position = InstanceMatrix * VP * Model * v_pos;
}
