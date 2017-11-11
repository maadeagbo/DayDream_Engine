#version 430

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;
layout (location = 3) in vec3 TangentNormal;
layout (location = 4) in vec4 BlendWeight;
layout (location = 5) in vec4 JointIndex;

layout (location = 6) in mat4 InstanceMatrix;
layout ( std430, binding = 11 ) buffer JNT { mat4 Joints[]; };

uniform mat4 MVP;
uniform mat4 LightSpace;

out VS_OUT {
	vec4 FragPos;
} vs_out;

vec4 getSkinnedVec4(const vec4 in_v, const uint jnt_idx)
{
	const uint idx = uint(JointIndex[jnt_idx]);
    return (Joints[idx] * in_v) * BlendWeight[jnt_idx];
}

void main() {
	// calculate and apply skinning transformations
    vec4 v_pos = vec4(0.0);
    for (uint i = 0; i < 4; i++) {
        v_pos = getSkinnedVec4( vec4(VertexPosition, 1.0), i) + v_pos;
    }
	v_pos = LightSpace * InstanceMatrix * v_pos;

    // output to fragment shader
	vs_out.FragPos = v_pos;
	gl_Position =  MVP * v_pos;
}
