#version 430

layout( points ) in;
layout( triangle_strip, max_vertices = 4 ) out;

uniform float Size2;  // half width of quad
uniform float EmitterLife;
uniform mat4 Proj;
uniform bool fireFlag = false;

out vec2 TexCoord;
out vec4 G_Color;
out vec4 G_Velocity;
out float G_Life;

in vec4 V_Color[];
in vec4 V_Velocity[];
in float V_Life[];

void out_uniforms() {
	// pass thru colors and life time
	G_Color = vec4(V_Color[0]);
	G_Velocity = vec4(V_Velocity[0]);
	G_Life = V_Life[0];
}

void main() {
	out_uniforms();

	float _sizeW = Size2;
	float _sizeH = Size2;
	if (fireFlag) {
		float lifeF = (V_Life[0] / EmitterLife);
		float tF = step(lifeF, 0.1); // return 1 near end of life
		if (tF > 0.99) {
			_sizeW = _sizeW *  4.0;
			_sizeH = _sizeH * 3 * 4.0;
		}
		else {
			_sizeW = _sizeW * (V_Life[0] * 5.0 / EmitterLife);
			_sizeH = _sizeH * 2 * (V_Life[0] * 5.0 / EmitterLife);
		}
	}

	// points generated in triangle strip order
	// first point & UV
	gl_Position = Proj * (vec4(-_sizeW, -_sizeH, 0.0, 0.0) + gl_in[0].gl_Position);
	TexCoord = vec2(0.0, 0.0);

	EmitVertex();

	// second point & UV
	gl_Position = Proj * (vec4(_sizeW, -_sizeH, 0.0, 0.0) + gl_in[0].gl_Position);
	TexCoord = vec2(1.0, 0.0);
	
	EmitVertex();

	// third point & UV
	gl_Position = Proj * (vec4(-_sizeW, _sizeH, 0.0, 0.0) + gl_in[0].gl_Position);
	TexCoord = vec2(0.0, 1.0);
	
	EmitVertex();

	// fourth point & UV
	gl_Position = Proj * (vec4(_sizeW, _sizeH, 0.0, 0.0) + gl_in[0].gl_Position);
	TexCoord = vec2(1.0, 1.0);
	
	EmitVertex();

	if (V_Life[0] > 0.001) {
		EndPrimitive();
	}
}