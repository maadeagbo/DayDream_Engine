#include "ddRenderEngine.h"
#include <SOIL.h>
#include "GPUFrontEnd.h"
#include "ddAssetManager.h"
#include "ddFileIO.h"
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

/** \brief Load screen texture */
cbuff<32> load_screen_tex = "load_screen";
/** \brief Load screen matrices */
glm::mat4 load_scale_mat, load_trans_mat, load_rot_mat;
/** \brief Load screen time counter */
float load_ticks = 0.f;

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
  // cleanup ram
  SOIL_free_image_data(tex->image_info.image_data[0]);
  tex->image_info.image_data[0] = nullptr;

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
}

void shutdown() {
  // shaders
  for (auto &idx : map_shaders) {
    shaders[idx.second].cleanup();
  }
}

void render_load_screen() {
  // rotate load circle
  if (ddTime::get_time_float() > load_ticks + 0.1f) {
    load_ticks = ddTime::get_time_float();
    load_rot_mat =
        glm::rotate(load_rot_mat, glm::radians(30.f), glm::vec3(0.f, 0.f, 1.f));
  }

  // get shader
  ddShader *sh = find_ddShader(postp_sh.gethash());
  POW2_VERIFY_MSG(sh != nullptr, "Post processing shader missing", 0);
  sh->use();

  // set uniforms
  sh->set_uniform((int)RE_PostPr::MVP_m4x4,
                  load_trans_mat * load_scale_mat * load_rot_mat);
  sh->set_uniform((int)RE_PostPr::DoToneMap_b, false);
  sh->set_uniform((int)RE_PostPr::AveLum_f, 1.f);
  sh->set_uniform((int)RE_PostPr::Exposure_f, 0.75f);
  sh->set_uniform((int)RE_PostPr::White_f, 0.97f);

  // bind texture
  ddTex2D *tex = find_ddTex2D(load_screen_tex.gethash());
  POW2_VERIFY_MSG(tex != nullptr, "Load screen texture missing", 0);
  ddGPUFrontEnd::bind_texture(0, tex->image_info.tex_buff);
  sh->set_uniform((int)RE_PostPr::ColorTex_smp2d, 0);

  ddGPUFrontEnd::render_quad();
}

}  // namespace ddRenderer
