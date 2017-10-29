#version 430

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;

uniform mat4 MVP;
uniform bool DrawSky;
uniform bool QuadRender;
uniform bool LightVolume;

out VS_OUT {
	vec2 TexCoord;
	vec3 SkyCoord;
} vs_out;

void main() {
    // quad plane
	vs_out.TexCoord = VertexTexCoord;
    gl_Position = vec4(VertexPosition, 1.0);

	if (LightVolume) {
		gl_Position = MVP * vec4(VertexPosition, 1.0);
	}
	
	if (DrawSky) {
		vs_out.SkyCoord = VertexPosition;
		gl_Position = MVP * vec4(VertexPosition, 1.0);
		gl_Position.z = gl_Position.w - 0.00001;
	} 
}