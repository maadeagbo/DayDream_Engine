#include "ddRenderEngine.h"
#include "GPUFrontEnd.h"
#include "ddAssetManager.h"
#include "ddFileIO.h"
#include "ddParticleSystem.h"
#include "ddSceneManager.h"
#include "ddShader.h"
#include "ddShaderReflect.h"
#include "ddTerminal.h"

enum class ShaderType : unsigned { VERT = 0, FRAG, COMP, GEOM };

namespace {
DD_FuncBuff fb;

ASSET_DECL(ddShader)
ASSET_CREATE(ddShader, shaders, MAX_SHADERS)
ASSET_DEF(ddShader, shaders)

/** \brief Native width & height */
unsigned scr_width = 0, scr_height = 0;
/** \brief Native shadow resolution */
unsigned shadow_width = 0, shadow_height = 0;
/** \brief Native cube map resolution */
unsigned cube_width = 0, cube_height = 0;

/** \brief Ping pong texture container */
dd_array<ImageInfo> lumin_textures(10);
/** \brief Ping pong texture container */
dd_array<float> lumin_output;
/** \brief Luminance shader compute divisor */
const unsigned lumin_divisor = 8;

/** \brief Light plane hash */
const cbuff<32> light_plane_id = "_dd_light_plane";

/** \brief Flag for controlling shadows */
bool shadows_on = true;

/** \brief Load screen texture */
cbuff<32> load_screen_tex = "load_screen";
/** \brief Load screen matrices */
glm::mat4 load_scale_mat, load_trans_mat, load_rot_mat;
/** \brief Load screen time counter */
float load_ticks = 0.f;

/** \brief Draw call information */
unsigned draw_calls = 0;
unsigned tris_in_frame = 0;
// unsigned lines_in_frame = 0;
unsigned objects_in_frame = 0;

/** \brief frustum cull buffer */
dd_array<ddAgent *> _agents = dd_array<ddAgent *>(ASSETS_CONTAINER_MAX_SIZE);
/** \brief active lights buffer */
dd_array<ddLBulb *> _lights = dd_array<ddLBulb *>(ASSETS_CONTAINER_MIN_SIZE);

/** \brief Main shader ids */
cbuff<32> geom_sh = "geometry";
cbuff<32> light_sh = "light";
cbuff<32> postp_sh = "postprocess";
cbuff<32> text_sh = "text";
cbuff<32> pingp_sh = "pingpong";
cbuff<32> lumin_sh = "luminance";
cbuff<32> line_sh = "line";
cbuff<32> lstencil_sh = "light_stencil";
cbuff<32> depth_sh = "shadow";
cbuff<32> depthsk_sh = "shadow_skinned";

}  // namespace

/** \brief Internal draw function */
void draw_scene(const glm::mat4 cam_view_m, const glm::mat4 cam_proj_m,
                const glm::vec3 cam_pos);
/** \brief Geometry buffer pass */
void gbuffer_pass(const glm::mat4 cam_view_m, const glm::mat4 cam_proj_m);
/** \brief Static mesh render pass */
void render_static(const glm::mat4 cam_view_m, const glm::mat4 cam_proj_m,
                   ddAgent *agent, ddLBulb *shadow_light = nullptr);
/** \brief Light buffer pass */
void light_pass(const glm::mat4 cam_view_m, const glm::mat4 cam_proj_m,
                const glm::vec3 cam_pos);

/**
 * \brief Import shader thru lua script
 */
int import_shader(lua_State *L) {
  parse_lua_events(L, fb);

  // get shader path
  const char *path = fb.get_func_val<const char>("path");
  const char *s_id = fb.get_func_val<const char>("id");
  int64_t *type = fb.get_func_val<int64_t>("type");

  if (path && type && s_id) {
    ddShader *sh = nullptr;
    // check that path exists
    ddIO check;
    if (check.open(path, ddIOflag::READ)) {
      // check if shader already exists
      size_t new_id = getCharHash(s_id);
      sh = find_ddShader(new_id);
      if (!sh) sh = spawn_ddShader(new_id);

      // add to list (if not full)
      if (sh) {
        ShaderType t((ShaderType)*type);
        switch (t) {
          case ShaderType::VERT:
            printf("Adding vertex shader->   %s : %s\n", s_id, path);
            sh->vs = path;
            break;
          case ShaderType::FRAG:
            printf("Adding fragment shader-> %s : %s\n", s_id, path);
            sh->fs = path;
            break;
          case ShaderType::COMP:
            printf("Adding compute shader->  %s : %s\n", s_id, path);
            sh->cs = path;
            break;
          case ShaderType::GEOM:
            printf("Adding geometry shader-> %s : %s\n", s_id, path);
            sh->gs = path;
            break;
          default:
            // invalid type provided
            printf("[error]Invalid shader type <%u>::%s\n", (unsigned)t, path);
            break;
        }
        return 0;
      }
    }
  }
  fprintf(stderr, "Failed to create/add to shader\n");
  return 0;
}

/** \brief Set shadow resolution */
int set_shadow_res(lua_State *L) {
  parse_lua_events(L, fb);

  int64_t *w = fb.get_func_val<int64_t>("width");
  int64_t *h = fb.get_func_val<int64_t>("height");

  if (w && h) {
    shadow_width = (unsigned)(*w);
    shadow_height = (unsigned)(*h);

    printf("Shadow resolution-> %u:%u\n", shadow_width, shadow_height);
    return 0;
  }
  fprintf(stderr, "Shadow resolution not set\n");
  return 0;
}

/** \brief Set cube map resolution */
int set_cube_res(lua_State *L) {
  parse_lua_events(L, fb);

  int64_t *w = fb.get_func_val<int64_t>("width");
  int64_t *h = fb.get_func_val<int64_t>("height");

  if (w && h) {
    cube_width = (unsigned)(*w);
    cube_height = (unsigned)(*h);

    printf("Cube map resolution-> %u:%u\n", cube_width, cube_height);
    return 0;
  }
  fprintf(stderr, "Cube map resolution not set\n");
  return 0;
}

/** \brief Calculate and assign texture resolutions for pingpong shader */
void calc_pingpong_textures() {
  unsigned tex_w = scr_width, tex_h = scr_height;
  unsigned limit = std::min(tex_h, tex_w);
  unsigned index = 0;
  while (limit > lumin_divisor) {
    // first change texture size
    tex_w = tex_w / lumin_divisor;
    tex_h = tex_h / lumin_divisor;
    limit = std::min(tex_h, tex_w);
    lumin_textures[index].width = tex_w;
    lumin_textures[index].height = tex_h;
    index++;
  }
  // resize texture size array
  index = (index == 0) ? 1 : index;
  dd_array<ImageInfo> temp(index);
  temp = lumin_textures;
  lumin_textures = std::move(temp);
  // use texture sizes for computing luminance
  lumin_output.resize(lumin_textures[index - 1].width *
                      lumin_textures[index - 1].height * 4);
  for (unsigned i = 0; i < index; ++i) {
    printf("compute_W: %u, compute_H: %u\n", lumin_textures[i].width,
           lumin_textures[i].height);
    ddGPUFrontEnd::generate_texture2D_RGBA16F_LR(lumin_textures[i]);
  }
}

namespace ddRenderer {

void init_lua_globals(lua_State *L) {
  // shader types
  set_lua_global(L, "VERT_SHADER", (int64_t)ShaderType::VERT);
  set_lua_global(L, "FRAG_SHADER", (int64_t)ShaderType::FRAG);
  set_lua_global(L, "COMP_SHADER", (int64_t)ShaderType::COMP);
  set_lua_global(L, "GEOM_SHADER", (int64_t)ShaderType::GEOM);

  // lua functions
  add_func_to_scripts(L, import_shader, "add_shader");
  add_func_to_scripts(L, set_shadow_res, "shadow_resolution");
  add_func_to_scripts(L, set_cube_res, "cubemap_resolution");

  // initialize shader bin
  fl_shaders.initialize((unsigned)shaders.size());
}

void initialize(const unsigned width, const unsigned height) {
  scr_width = width;
  scr_height = height;

  // initialize buffers in gpu frontend for rendering
  ddGPUFrontEnd::create_gbuffer(scr_width, scr_height);
  ddGPUFrontEnd::create_lbuffer(scr_width, scr_height);
  ddGPUFrontEnd::create_pbuffer(scr_width, scr_height);
  ddGPUFrontEnd::create_sbuffer(shadow_width, shadow_height);
  ddGPUFrontEnd::create_cbuffer(cube_width, cube_height);
  ddGPUFrontEnd::create_fbuffer(scr_width, scr_height, shadow_width,
                                shadow_height);

  // ping pong shader textures
  calc_pingpong_textures();

  // load mesh for rendering light volumes

  // set up load screen texture(s)
  ddTex2D *tex = find_ddTex2D(load_screen_tex.gethash());
  POW2_VERIFY_MSG(tex != nullptr, "Load screen texture not found", 0);
  bool success = ddGPUFrontEnd::generate_texture2D_RGBA8_LR(tex->image_info);
  POW2_VERIFY_MSG(success == true, "Load screen texture not generated", 0);
  // cleanup loaded image from ram
  tex->image_info.image_data[0].resize(0);

  // set up load screen MVP matrices
  load_rot_mat = glm::mat4();
  float width_factor = (float)scr_height / (float)scr_width;  // un-even res
  load_scale_mat =
      glm::scale(load_scale_mat, glm::vec3(width_factor, 1.0, 1.0));
  load_scale_mat = glm::scale(load_scale_mat, glm::vec3(0.1f));
  load_trans_mat = glm::translate(glm::mat4(), glm::vec3(0.9f, -0.85f, 0.0f));

  // load all shaders
  for (auto &idx : map_shaders) {
    ddShader *sh = &shaders[idx.second];
    sh->init();

    // compute
    if (sh->cs.compare("") != 0) {
      sh->create_comp_shader(sh->cs.str());
    } else {
      // vertex
      if (sh->vs.compare("") != 0) {
        sh->create_vert_shader(sh->vs.str());
      }
      // geomtry
      if (sh->gs.compare("") != 0) {
        sh->create_geom_shader(sh->gs.str());
      }
      // fragment
      if (sh->fs.compare("") != 0) {
        sh->create_frag_shader(sh->fs.str());
      }
    }
  }

  // light volumes/plane
  ddAgent *lplane = find_ddAgent(light_plane_id.gethash());
  POW2_VERIFY_MSG(lplane != nullptr, "Light plane agent not initialized", 0);
  ddAssets::remove_rigid_body(lplane);
  lplane->rend.flag_render = false;
}

void shutdown() {
  // shaders
  for (auto &idx : map_shaders) {
    shaders[idx.second].cleanup();
  }
}

void render_load_screen() {
  // rotate load circle
  if (ddTime::get_time_float() > load_ticks + 0.05f) {
    load_ticks = ddTime::get_time_float();
    load_rot_mat = glm::rotate(load_rot_mat, glm::radians(30.f), global_Zv3);
  }
  ddGPUFrontEnd::toggle_depth_test(false);

  // get shader
  ddShader *sh = find_ddShader(postp_sh.gethash());
  POW2_VERIFY_MSG(sh != nullptr, "Post processing shader missing", 0);
  sh->use();

  // set uniforms
  sh->set_uniform((int)RE_PostPr::MVP_m4x4,
                  load_trans_mat * load_scale_mat * load_rot_mat);
  sh->set_uniform((int)RE_PostPr::DoToneMap_b, false);
  sh->set_uniform((int)RE_PostPr::GammaCorrect_b, false);
  sh->set_uniform((int)RE_PostPr::Blur_b, false);
  sh->set_uniform((int)RE_PostPr::SampleShadow_b, false);
  sh->set_uniform((int)RE_PostPr::AveLum_f, 1.f);
  sh->set_uniform((int)RE_PostPr::Exposure_f, 0.75f);
  sh->set_uniform((int)RE_PostPr::White_f, 0.97f);

  // bind texture
  ddTex2D *tex = find_ddTex2D(load_screen_tex.gethash());
  POW2_VERIFY_MSG(tex != nullptr, "Load screen texture missing", 0);
  ddGPUFrontEnd::bind_texture(0, tex->image_info.tex_buff);
  sh->set_uniform((int)RE_PostPr::ColorTex_smp2d, 0);
  sh->set_uniform((int)RE_PostPr::SampleMap_b, true);

  ddGPUFrontEnd::toggle_alpha_blend(true);
  ddGPUFrontEnd::render_quad();
  ddGPUFrontEnd::toggle_alpha_blend(false);
}

void draw_world() {
  // draw information reset
  draw_calls = 0;
  tris_in_frame = 0;
  objects_in_frame = 0;

  // grab camera data
  ddCam *cam = ddSceneManager::get_active_cam();
  POW2_VERIFY_MSG(cam != nullptr, "No active camera", 0);
  ddAgent *cam_p = find_ddAgent(cam->parent);
  POW2_VERIFY_MSG(cam_p != nullptr, "Active camera has no parent", 0);

  glm::mat4 view_m = ddSceneManager::calc_view_matrix(cam);
  glm::mat4 proj_m = ddSceneManager::calc_p_proj_matrix(cam);

  // get frustum and cull objects
  FrustumBox cam_fr = ddSceneManager::get_current_frustum(cam);
  ddSceneManager::cull_objects(cam_fr, view_m, _agents);

  // draw scene
  draw_scene(view_m, proj_m, ddBodyFuncs::pos_ws(&cam_p->body));

	printf("objects in frame: %d\n", objects_in_frame);
}

}  // namespace ddRenderer

void draw_scene(const glm::mat4 cam_view_m, const glm::mat4 cam_proj_m,
                const glm::vec3 cam_pos) {
  // geometry buffer pass
  ddGPUFrontEnd::clear_screen(0.f, 0.f, 0.f, 0.f);
  gbuffer_pass(cam_view_m, cam_proj_m);

  // line pass

  // shadow pass

  // light pass
  light_pass(cam_view_m, cam_proj_m, cam_pos);

  // draw skybox

  // particle pass
  ddParticleSys::render_tasks();

  // post processing
  // ddGPUFrontEnd::clear_screen(1.0);
  ddGPUFrontEnd::toggle_depth_test(false);
  ddGPUFrontEnd::toggle_alpha_blend(true);

  ddShader *sh = find_ddShader(postp_sh.gethash());
  POW2_VERIFY_MSG(sh != nullptr, "Post processing shader missing", 0);
  sh->use();

  // hdri tone mapping
  sh->set_uniform((int)RE_PostPr::DoToneMap_b, true);
  sh->set_uniform((int)RE_PostPr::AveLum_f, 1.f);

  // set uniforms
  sh->set_uniform((int)RE_PostPr::MVP_m4x4, glm::mat4());
  sh->set_uniform((int)RE_PostPr::GammaCorrect_b, true);
  sh->set_uniform((int)RE_PostPr::Exposure_f, 0.75f);
  sh->set_uniform((int)RE_PostPr::White_f, 0.97f);
  sh->set_uniform((int)RE_PostPr::Blur_b, false);
  sh->set_uniform((int)RE_PostPr::SampleShadow_b, false);
  sh->set_uniform((int)RE_PostPr::output2D_b, false);

  // bind texture
  ddGPUFrontEnd::bind_pass_texture(ddBufferType::LIGHT);
  ddGPUFrontEnd::bind_pass_texture(ddBufferType::PARTICLE);
  sh->set_uniform((int)RE_PostPr::ColorTex_smp2d, 0);
  sh->set_uniform((int)RE_PostPr::ParticleTex_smp2d, 1);
  sh->set_uniform((int)RE_PostPr::SampleMap_b, false);
  sh->set_uniform((int)RE_PostPr::BlendParticle_b, true);

  ddGPUFrontEnd::render_quad();
  ddGPUFrontEnd::toggle_alpha_blend(false);

  return;
}

void gbuffer_pass(const glm::mat4 cam_view_m, const glm::mat4 cam_proj_m) {
  // set up new frame
  ddGPUFrontEnd::bind_framebuffer(ddBufferType::GEOM);
  ddGPUFrontEnd::clear_color_buffer();
  ddGPUFrontEnd::clear_depth_buffer();
  ddGPUFrontEnd::toggle_depth_test(true);
  ddGPUFrontEnd::toggle_face_cull(true);
  ddGPUFrontEnd::set_face_cull();
  // clipping (if necessary)
  ddGPUFrontEnd::toggle_clip_plane();

  DD_FOREACH(ddAgent *, ag, _agents) {
    // break on the first null pointer
    ddAgent *agent = *ag.ptr;
    if (!agent) break;

    // check if model can be rendered
    if (agent->rend.flag_render && agent->mesh.isValid()) {
      // check if model is instanced
      if (agent->inst.m4x4.size() == 1) {
        // render skinned mesh
        if (agent->rend.global_pose.isValid()) {
          //
        } else {
          // render static
          render_static(cam_view_m, cam_proj_m, agent);
        }
      }
    }
  }
  ddGPUFrontEnd::bind_framebuffer(ddBufferType::DEFAULT);

  return;
}

void render_static(const glm::mat4 cam_view_m, const glm::mat4 cam_proj_m,
                   ddAgent *agent, ddLBulb *shadow_light) {
  /** \brief simple utility function for retrieving index of flag */
  auto get_tex_idx = [](const TexType t_t) {
    return (size_t)std::log2((double)t_t);
  };
  /** \brief Get ddTextureData from ddMat && index */
  auto get_tex_handle = [](const ddMat *mat, const size_t idx) {
    ddTex2D *tex = find_ddTex2D(mat->textures[idx]);
    POW2_VERIFY_MSG(tex != nullptr, "Texture id provided but not found", 0);
    return tex->image_info.tex_buff;
  };
  // set up shader
  ddShader *sh = nullptr;
  if (shadow_light) {
    sh = find_ddShader(depth_sh.gethash());
    POW2_VERIFY_MSG(sh != nullptr, "Couldn't locate depth shader", 0);
    sh->use();
  } else {
    sh = find_ddShader(geom_sh.gethash());
    POW2_VERIFY_MSG(sh != nullptr, "Couldn't locate geometry shader", 0);
    sh->use();
  }
  // sh->set_uniform((int)RE_GBuffer::enable_clip1_b, false);  // always off

  // get mesh information
  DD_FOREACH(ModelIDs, mdl_id, agent->mesh) {
    ddModelData *mdl = find_ddModelData(mdl_id.ptr->model);
    POW2_VERIFY_MSG(mdl != nullptr, "Model id provided but not found", 0);

    // render each mesh in agent
    DD_FOREACH(DDM_Data, mdata, mdl->mesh_info) {
      // get material
      ddMat *mat = find_ddMat(mdl_id.ptr->material[mdata.i]);
      POW2_VERIFY_MSG(mat != nullptr, "Material provided but not found", 0);

      // set shader info
      if (shadow_light) {
        //
      } else {
        bool albedo = bool(TexType::ALBEDO & mat->texture_flag);
        sh->set_uniform((int)RE_GBuffer::albedoFlag_b, albedo);
        if (albedo) {
          size_t tex_idx = get_tex_idx(TexType::ALBEDO);
          ddGPUFrontEnd::bind_texture(0, get_tex_handle(mat, tex_idx));
          sh->set_uniform((int)RE_GBuffer::tex_albedo_smp2d, 0);
        }

        bool specular = bool(TexType::SPEC & mat->texture_flag);
        sh->set_uniform((int)RE_GBuffer::specFlag_b, specular);
        if (specular) {
          size_t tex_idx = get_tex_idx(TexType::SPEC);
          ddGPUFrontEnd::bind_texture(1, get_tex_handle(mat, tex_idx));
          sh->set_uniform((int)RE_GBuffer::tex_specular_smp2d, 1);
        }

        bool normal = bool(TexType::NORMAL & mat->texture_flag);
        sh->set_uniform((int)RE_GBuffer::normalFlag_b, normal);
        if (normal) {
          size_t tex_idx = get_tex_idx(TexType::NORMAL);
          ddGPUFrontEnd::bind_texture(2, get_tex_handle(mat, tex_idx));
          sh->set_uniform((int)RE_GBuffer::tex_normal_smp2d, 2);
        }
        sh->set_uniform((int)RE_GBuffer::diffuse_v3,
                        glm::vec3(mat->base_color));
        sh->set_uniform((int)RE_GBuffer::shininess_f, mat->spec_value);

        // matrices
        agent->inst.m4x4[0] = glm::mat4();  // identity
        glm::mat4 model_m = ddBodyFuncs::get_model_mat(&agent->body);
        glm::mat4 norm_m = glm::transpose(glm::inverse(model_m));
        sh->set_uniform((int)RE_GBuffer::Norm_m4x4, norm_m);
        sh->set_uniform((int)RE_GBuffer::Model_m4x4, model_m);
        sh->set_uniform((int)RE_GBuffer::VP_m4x4, cam_proj_m * cam_view_m);

        // bind/fill instance buffer
        ddGPUFrontEnd::set_instance_buffer_contents(agent->inst.inst_buff,
                                                    false, sizeof(glm::mat4), 0,
                                                    &agent->inst.m4x4[0]);
        // bind & fill color buffer
        sh->set_uniform((int)RE_GBuffer::multiplierMat_b, false);  // TODO: flag
        ddGPUFrontEnd::set_instance_buffer_contents(agent->inst.inst_buff, true,
                                                    sizeof(glm::vec3), 0,
                                                    &agent->inst.v3[0]);
        // debug flag
        sh->set_uniform((int)RE_GBuffer::useDebug_b, false);
      }
      // log stats
      tris_in_frame += (unsigned)mdata.ptr->indices.size() / 3;
      draw_calls += 1;

      // bind vao and render
      ddGPUFrontEnd::draw_instanced_vao(mdl_id.ptr->vao_handles[mdata.i],
                                        (unsigned)mdata.ptr->indices.size(), 1);

			// update stats
			objects_in_frame++;
    }
  }
  return;
}

glm::vec3 viewport_transform(const glm::vec3 ndc, const glm::vec2 dimension) {
  glm::vec3 out;
  glm::vec2 half_dim = dimension / 2.f;

  out.x = (ndc.x * half_dim.x) + half_dim.x;
  out.y = (ndc.y * half_dim.y) + half_dim.y;
  out.z = 1.f;

  return out;
}

glm::vec3 point_light_vert(const glm::vec3 origin, const glm::vec3 w_h_d) {
  glm::vec3 out_vec;

  out_vec.x = origin.x + w_h_d.x;
  out_vec.y = origin.y + w_h_d.y;
  out_vec.z = origin.z + w_h_d.z;

  return out_vec;
}

struct SpotLData {
  glm::mat4 model;
  glm::vec3 min;
  glm::vec3 max;
};

SpotLData get_spot_light_data(const ddLBulb *blb, const float light_extent) {
  SpotLData data;

  // calcalute center of circle at "end" of spot light
  const glm::vec3 f_dir = glm::normalize(blb->direction);
  const glm::vec3 center = light_extent * f_dir + blb->position;
  // vectors to left and up at "end" of cone
  const glm::vec3 r_dir = glm::cross(f_dir, global_Yv3);
  const glm::vec3 u_dir = glm::cross(r_dir, f_dir);

  // calculate radius at "end" of cone based on outer angle
  const float c_radius = glm::tan(blb->cutoff_o) * light_extent;

  const glm::vec3 center_r = r_dir * c_radius + center;
  const glm::vec3 center_l = -r_dir * c_radius + center;
  const glm::vec3 center_t = u_dir * c_radius + center;
  const glm::vec3 center_b = -u_dir * c_radius + center;

  // get AABB bounds using 5 vertices
  BoundingBox bbox;
  bbox.corner1 = center_r;
  bbox.corner2 = center_l;
  bbox.corner3 = center_t;
  bbox.corner4 = center_b;
  bbox.corner5 = blb->position;
  bbox.corner6 = blb->position;
  bbox.corner7 = blb->position;
  bbox.corner8 = blb->position;

  bbox.UpdateAABB_min();
  bbox.UpdateAABB_max();

  // get center of AABB
  const glm::vec3 bb_vec = bbox.min - bbox.max;
  const glm::vec3 bb_pnt =
      glm::normalize(bb_vec) * glm::length(bb_vec) * 0.5f + bbox.max;
  // get scale in x, y, z
  const float sc_x = bbox.max.x - bbox.min.x;
  const float sc_y = bbox.max.y - bbox.min.y;
  const float sc_z = bbox.max.z - bbox.min.z;

  // set data
  data.model = glm::translate(glm::mat4(), bb_pnt);
  data.model = glm::scale(data.model, glm::vec3(sc_x, sc_y, sc_z));
  data.min = bbox.min;
  data.max = bbox.max;

  return data;
}

void light_pass(const glm::mat4 cam_view_m, const glm::mat4 cam_proj_m,
                const glm::vec3 cam_pos) {
  // buffer setup
  ddGPUFrontEnd::blit_depth_buffer(ddBufferType::GEOM, ddBufferType::LIGHT,
                                   scr_width, scr_height);
  ddGPUFrontEnd::toggle_depth_mask();
  ddGPUFrontEnd::toggle_face_cull(false);
  ddGPUFrontEnd::bind_framebuffer(ddBufferType::LIGHT);
  ddGPUFrontEnd::clear_color_buffer();
  ddGPUFrontEnd::toggle_additive_blend(true);

  // shader setup
  ddShader *sh = find_ddShader(light_sh.gethash());
  POW2_VERIFY_MSG(sh != nullptr, "Light shader queried but not found", 0);
  sh->use();

  sh->set_uniform((int)RE_Light::viewPos_v3, cam_pos);
  sh->set_uniform((int)RE_Light::DrawSky_b, false);
  sh->set_uniform((int)RE_Light::Debug_b, false);

  // bind geometry buffer multipass texture
  sh->set_uniform((int)RE_Light::PositionTex_smp2d, 0);
  sh->set_uniform((int)RE_Light::ColorTex_smp2d, 1);
  sh->set_uniform((int)RE_Light::NormalTex_smp2d, 2);

  ddGPUFrontEnd::bind_pass_texture(ddBufferType::GEOM);

  // get shadow producing light (if one exists)
  ddLBulb *shadow_blb = ddSceneManager::get_shadow_light();

  // loop through lights
  ddSceneManager::get_active_lights(_lights);
  DD_FOREACH(ddLBulb *, l_b, _lights) {
    ddLBulb *blb = *l_b.ptr;
    if (!blb) break;

    // if parent exists, get tranformation
    glm::mat4 parent_mat;
    if (blb->parent_set) {
      ddAgent *blb_p = find_ddAgent(blb->parent_id);
      POW2_VERIFY_MSG(blb_p != nullptr, "ddLBulb parent set but not found", 0);
      parent_mat = ddBodyFuncs::get_model_mat(&blb_p->body);
    }

    // calculate light luminance using Rec 709
    const float light_lumin =
        glm::dot(blb->color, glm::vec3(0.2126, 0.7152, 0.0722));
    const float min_lumin =
        0.01f;  // may need to be scaled inversely by exposure

    // set shader uniforms per light
    glm::vec3 lpos = glm::vec3(parent_mat * glm::vec4(blb->position, 1.f));
    sh->set_uniform((int)RE_Light::Light_position_v3, lpos);
    sh->set_uniform((int)RE_Light::Light_type_i, (int)blb->type);
    // sh->set_uniform((int)RE_Light::light_geo_i, (int)blb->type);
    sh->set_uniform((int)RE_Light::Light_direction_v3, blb->direction);
    sh->set_uniform((int)RE_Light::Light_color_v3, blb->color);
    sh->set_uniform((int)RE_Light::Light_lumin_cutoff_f,
                    min_lumin / light_lumin);
    sh->set_uniform((int)RE_Light::Light_cutoff_i_f, blb->cutoff_i);
    sh->set_uniform((int)RE_Light::Light_cutoff_o_f, blb->cutoff_o);

    glm::vec2 scr_dim = glm::vec2((float)scr_width, (float)scr_height);
    sh->set_uniform((int)RE_Light::screenDimension_v2, scr_dim);

    // calculate model matrix for light _light volume = mesh approximation)
    float lv_radius = ddSceneManager::calc_lightvolume_radius(blb);
    glm::mat4 model;
    model = glm::translate(model, blb->position);
    model = glm::scale(model, glm::vec3(lv_radius));
    // glm::mat4 light_mvp = cam_proj_m * cam_view_m * parent_mat * model;

    // render
    switch (blb->type) {
      case LightType::DIRECTION_L: {
        if (shadows_on && shadow_blb) {
          // bind shadow texture
          ddGPUFrontEnd::bind_pass_texture(ddBufferType::SHADOW, 3, 0);
          // send light space matrix and activate shadow calculation
          sh->set_uniform((int)RE_Light::ShadowMap_b, true);
          sh->set_uniform((int)RE_Light::LSM_m4x4, shadow_blb->l_s_m);
        }
        // render full screen quad & turn off shadow calc
        sh->set_uniform((int)RE_Light::Model_m4x4, glm::mat4());
        sh->set_uniform((int)RE_Light::View_m4x4, glm::mat4());
        sh->set_uniform((int)RE_Light::Proj_m4x4, glm::mat4());

        // render full screen quad
        // sh->set_uniform((int)RE_Light::half_width_f, 1.f);
        // sh->set_uniform((int)RE_Light::half_height_f, 1.f);

        ddGPUFrontEnd::render_quad();

        sh->set_uniform((int)RE_Light::ShadowMap_b, false);
        break;
      }
      case LightType::POINT_L: {
        sh->set_uniform((int)RE_Light::Model_m4x4, model);
        sh->set_uniform((int)RE_Light::View_m4x4, cam_view_m);
        sh->set_uniform((int)RE_Light::Proj_m4x4, cam_proj_m);

        // check if inside volume
        const glm::vec3 w_h_d(lv_radius + 0.1f);
        glm::vec3 max_cnr = point_light_vert(blb->position, w_h_d);
        glm::vec3 min_cnr = point_light_vert(blb->position, -w_h_d);

        bool in_vol = true;
        in_vol = cam_pos.x <= max_cnr.x && cam_pos.x >= min_cnr.x && in_vol;
        in_vol = cam_pos.y <= max_cnr.y && cam_pos.y >= min_cnr.y && in_vol;
        in_vol = cam_pos.z <= max_cnr.z && cam_pos.z >= min_cnr.z && in_vol;

        if (in_vol) {
          ddGPUFrontEnd::toggle_depth_test(false);
        }

        sh->set_uniform((int)RE_Light::Debug_b, true);
        sh->set_uniform((int)RE_Light::Debug_Color_v4,
                        glm::vec4(1.f, 1.f, 1.f, 0.5f));

        ddGPUFrontEnd::render_cube();

        sh->set_uniform((int)RE_Light::Debug_b, false);

        if (in_vol) {
          ddGPUFrontEnd::toggle_depth_test(true);
        }
        break;
      }
      case LightType::SPOT_L: {
        const SpotLData s_data = get_spot_light_data(blb, lv_radius);

        sh->set_uniform((int)RE_Light::Model_m4x4, s_data.model);
        sh->set_uniform((int)RE_Light::View_m4x4, cam_view_m);
        sh->set_uniform((int)RE_Light::Proj_m4x4, cam_proj_m);

        // check if inside volume
        bool in_vol = true;
        const glm::vec3 max_cnr = s_data.max;
        const glm::vec3 min_cnr = s_data.min;
        in_vol = cam_pos.x <= max_cnr.x && cam_pos.x >= min_cnr.x && in_vol;
        in_vol = cam_pos.y <= max_cnr.y && cam_pos.y >= min_cnr.y && in_vol;
        in_vol = cam_pos.z <= max_cnr.z && cam_pos.z >= min_cnr.z && in_vol;

        if (in_vol) {
          ddGPUFrontEnd::toggle_depth_test(false);
        }

        sh->set_uniform((int)RE_Light::Debug_b, true);
        sh->set_uniform((int)RE_Light::Debug_Color_v4,
                        glm::vec4(1.f, 0.f, 0.f, 0.5f));

        ddGPUFrontEnd::render_cube();

        sh->set_uniform((int)RE_Light::Debug_b, false);

        if (in_vol) {
          ddGPUFrontEnd::toggle_depth_test(true);
        }
        break;
      }
      default:
        break;
    }
  }

  // reset
  ddGPUFrontEnd::toggle_additive_blend();
  ddGPUFrontEnd::toggle_depth_mask(true);
  ddGPUFrontEnd::toggle_face_cull(true);
  ddGPUFrontEnd::bind_framebuffer(ddBufferType::DEFAULT);
  ddGPUFrontEnd::bind_pass_texture(ddBufferType::DEFAULT);
  return;
}
