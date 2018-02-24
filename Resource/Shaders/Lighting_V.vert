#version 430

layout (location = 0) in vec3 VertexPosition;
layout (location = 1) in vec3 VertexNormal;
layout (location = 2) in vec2 VertexTexCoord;

uniform mat4 Model;
uniform mat4 View;
uniform mat4 Proj;
uniform bool DrawSky;

// out VS_OUT {
// 	vec2 TexCoord;
// 	vec3 SkyCoord;
// } vs_out;
out vec3 SkyCoord;

void main() {
  // quad plane
	//vs_out.TexCoord = VertexTexCoord;
  gl_Position = Proj * View * Model * vec4(VertexPosition, 1.0);

	//if (LightVolume) {
		// vec3 pos = light_vol_center + 
		// 	(cam_right_ws * VertexPosition.x * light_vol_radius) + 
		// 	(cam_up_ws * VertexPosition.y * light_vol_radius);
		// gl_Position = vec4(pos, 1.0);
	//}
	
	if (DrawSky) {
		SkyCoord = VertexPosition;
		gl_Position = Proj * View * vec4(VertexPosition, 1.0);
		gl_Position.z = gl_Position.w - 0.00001;
	} 
}