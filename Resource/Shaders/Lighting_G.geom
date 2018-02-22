#version 430
layout( points ) in;
layout( triangle_strip, max_vertices = 24 ) out;

uniform float half_width;
uniform float half_height;
uniform float half_depth;
uniform mat4 View;
uniform mat4 Proj;
uniform vec3 viewPos;

uniform int light_geo;
uniform vec2 screenDimension;

// in VS_OUT {
// 	vec2 TexCoord;
// 	vec3 SkyCoord;
// } gs_in;

out GS_OUT {
	vec2 TexCoord;
	vec3 SkyCoord;
} gs_out;

struct EmitVerts {
	vec4 top_left;
	vec4 top_right;
	vec4 bot_left;
	vec4 bot_right;
};

void out_uniforms(const vec2 tex_coord) {
	// pass thru coordinates
	gs_out.TexCoord = tex_coord;
	gs_out.SkyCoord = vec3(1.0);
}

EmitVerts get_verts(const vec4 dir_1, const vec4 dir_2, const float dist_1, 
                        const float dist_2, const vec4 center) {
  EmitVerts out_v;

  out_v.bot_left = Proj * (dir_1 * -dist_1 + dir_2 * -dist_2 + center);
  out_v.bot_right = Proj * (dir_1 * dist_1 + dir_2 * -dist_2 + center);
  out_v.top_left = Proj * (dir_1 * -dist_1 + dir_2 * dist_2 + center);
  out_v.top_right = Proj * (dir_1 * dist_1 + dir_2 * dist_2 + center);

  return out_v;
}

// void emit_quad(const vec4 dir_1, const vec4 dir_2, const float dist_1, 
//               const float dist_2, const vec4 center) {
//   // points generated in triangle strip order
// 	// bottom left
// 	gl_Position = Proj * View * (dir_1 * -dist_1 + dir_2 * -dist_2 + center);
//   EmitVertex();

// 	// bottom right
// 	gl_Position = Proj * View * (dir_1 * dist_1 + dir_2 * -dist_2 + center);
// 	EmitVertex();

// 	// top left
// 	gl_Position = Proj * View * (dir_1 * -dist_1 + dir_2 * dist_2 + center);
// 	EmitVertex();

// 	// top right
// 	gl_Position = Proj * View * (dir_1 * dist_1 + dir_2 * dist_2 + center);
// 	EmitVertex();
// }


void emit_quad(const EmitVerts verts) {
  // points generated in triangle strip order
	// bottom left
	gl_Position = verts.bot_left;
  EmitVertex();

	// bottom right
	gl_Position = verts.bot_right;
	EmitVertex();

	// top left
	gl_Position = verts.top_left;
	EmitVertex();

	// top right
	gl_Position = verts.top_right;
	EmitVertex();
}

void main() {
  out_uniforms(vec2(0.0));

  // view space
  const vec4 r_dir = vec4(View[0][0], View[1][0], View[2][0], 0.0);
  const vec4 u_dir = vec4(View[0][1], View[1][1], View[2][1], 0.0);
  const vec4 f_dir = vec4(View[0][2], View[1][2], View[2][2], 0.0);

  // world space 
  const vec4 w_r_dir = View * vec4(1.0, 0.0, 0.0, 0.0);
  const vec4 w_u_dir = View * vec4(0.0, 1.0, 0.0, 0.0);
  const vec4 w_f_dir = View * vec4(0.0, 0.0, -1.0, 0.0);

  if (light_geo == 0) {
    EmitVerts verts = 
      get_verts(r_dir, u_dir, half_width, half_height, gl_in[0].gl_Position);
    emit_quad(verts);

    EndPrimitive();
  }
  else if (light_geo == 1) {
    // camera space location of light
    const vec4 light_pos = View * gl_in[0].gl_Position;
    // check if inside volume
    const float cam_dist = distance(vec4(viewPos, 1.0), light_pos);

    // front
    const vec4 f_1 = light_pos + w_f_dir * -half_depth;
    EmitVerts verts_f = get_verts(w_r_dir, w_u_dir, half_width, half_height, f_1);
    emit_quad(verts_f);
    
    EndPrimitive();
    
    // back
    const vec4 f_2 = light_pos + w_f_dir * half_depth;
    EmitVerts verts_b = get_verts(-w_r_dir, w_u_dir, half_width, half_height, f_2);
    emit_quad(verts_b);

    EndPrimitive();

    // right
    const vec4 f_3 = light_pos + w_r_dir * half_depth;
    EmitVerts verts_r = get_verts(w_f_dir, w_u_dir, half_depth, half_height, f_3);
    emit_quad(verts_r);
    
    EndPrimitive();

    // left
    const vec4 f_4 = light_pos + w_r_dir * -half_depth;
    EmitVerts verts_l = get_verts(-w_f_dir, w_u_dir, half_depth, half_height, f_4);
    emit_quad(verts_l);
    
    EndPrimitive();

    // top
    const vec4 f_5 = light_pos + w_u_dir * half_height;
    EmitVerts verts_tp = get_verts(w_r_dir, w_f_dir, half_width, half_depth, f_5);
    emit_quad(verts_tp);
    
    EndPrimitive();

    // bottom
    const vec4 f_6 = light_pos + w_u_dir * -half_height;
    EmitVerts verts_bt = get_verts(-w_r_dir, w_f_dir, half_width, half_depth, f_6);
    emit_quad(verts_bt);
    
    EndPrimitive();
  }
}