#version 430

layout( points ) in;
layout( triangle_strip, max_vertices = 4 ) out;

uniform float HalfWidth;  // half width of quad
uniform mat4 Proj;

in vec2 calc_uv[];
out vec2 out_uv;

void main() {
    float W = HalfWidth;
	float H = HalfWidth;
	// pass thru
	out_uv = vec2(calc_uv[0]);

    // points generated in triangle strip order
	// first point 
	gl_Position = Proj * (vec4(-W, -H, 0.0, 0.0) + gl_in[0].gl_Position);
	EmitVertex();
	// second point 
	gl_Position = Proj * (vec4(W, -H, 0.0, 0.0) + gl_in[0].gl_Position);
	EmitVertex();
	// third point 
	gl_Position = Proj * (vec4(-W, H, 0.0, 0.0) + gl_in[0].gl_Position);
	EmitVertex();
	// fourth point 
	gl_Position = Proj * (vec4(W, H, 0.0, 0.0) + gl_in[0].gl_Position);
	EmitVertex();

    EndPrimitive();
}