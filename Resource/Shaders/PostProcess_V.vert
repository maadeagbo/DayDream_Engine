#version 430

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;

uniform mat4 MVP;

out VS_OUT {
	vec2 TexCoord;
	vec4 LightSpaceCoord;
} vs_out;

uniform bool SampleShadow = false;
uniform bool flip_y_coord = false;
uniform bool flip_x_coord = false;
uniform bool uv_rotate = false;
uniform mat4 LSM;
uniform mat4 rotate_uv_mat;

void main() {
	vec2 t_coord = VertexTexCoord;
	if (uv_rotate) { t_coord = (rotate_uv_mat * vec4(t_coord.xy, 0.0, 1.0)).xy; }
	if (flip_y_coord) { t_coord.y = 1 - t_coord.y; }
	if (flip_x_coord) { t_coord.x = 1 - t_coord.x; }
	vs_out.TexCoord = t_coord;
	
	if (SampleShadow) {
		vs_out.LightSpaceCoord = LSM * vec4(VertexPosition, 1.0);
	}
	
    gl_Position = MVP * vec4(VertexPosition, 1.0);
}