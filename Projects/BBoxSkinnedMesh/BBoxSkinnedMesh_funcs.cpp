// level script for BBoxSkinnedMesh cpp implementations
#include "bbox_shader_enums.h"
#include "ddLevelPrototype.h"
#include "ddSceneManager.h"
#include "ddTerminal.h"

#include "BBoxGraphics.h"

/// \brief level control flags
enum class LvlCtrlEnums { TRANSLATE, ROTATE, SCALE, X, Y, Z, NUM_FLAGS };

/// \brief level controls
struct LvlCtrl {
  bool flags[(unsigned)LvlCtrlEnums::NUM_FLAGS];
};

namespace {
BBoxGraphics bb_graphics;
LvlCtrl lvl_ctrl;

dd_array<float> float_buf3(3);
dd_array<float> float_buf6(6);

BoundingBox ref_bbox;

cbuff<8> pos_str = "pos";
cbuff<8> rot_str = "rot";
cbuff<8> scale_str = "scale";

}  // namespace

//****************************************************************

#define BBOX_META_NAME "LuaClass.BBox"
#define check_bbox(L) (BBTransform **)luaL_checkudata(L, 1, BBOX_META_NAME)

static int set_val(lua_State *L);
static int get_val(lua_State *L);
static int bbox_index_func(lua_State *L);
static int check_intersect(lua_State *L);

// method list
static const struct luaL_Reg bbox_methods[] = {{"__index", bbox_index_func},
                                               {"__newindex", set_val},
                                               {"intersect", check_intersect},
                                               {NULL, NULL}};

int set_val(lua_State *L) {
  BBTransform *bbox = *check_bbox(L);
  const char *arg = (const char *)luaL_checkstring(L, 2);
  read_buffer_from_lua(L, float_buf3);

  // position
  if (pos_str.compare(arg) == 0) {
    bbox->pos.x = float_buf3[0];
    bbox->pos.y = float_buf3[1];
    bbox->pos.z = float_buf3[2];
  } else if (rot_str.compare(arg) == 0) {
    // rotation
    bbox->rot.x = float_buf3[0];
    bbox->rot.y = float_buf3[1];
    bbox->rot.z = float_buf3[2];
  } else if (scale_str.compare(arg) == 0) {
    // scale
    bbox->scale.x = float_buf3[0];
    bbox->scale.y = float_buf3[1];
    bbox->scale.z = float_buf3[2];
  }

  return 0;
}

int get_val(lua_State *L) {
  BBTransform *bbox = *check_bbox(L);
  const char *arg = (const char *)luaL_checkstring(L, 2);

  // position
  if (pos_str.compare(arg) == 0) {
    push_vec3_to_lua(L, bbox->pos.x, bbox->pos.y, bbox->pos.z);
    return 1;
  } else if (rot_str.compare(arg) == 0) {
    // rotation
    push_vec3_to_lua(L, bbox->rot.x, bbox->rot.y, bbox->rot.z);
    return 1;
  } else if (scale_str.compare(arg) == 0) {
    // scale
    push_vec3_to_lua(L, bbox->scale.x, bbox->scale.y, bbox->scale.z);
    return 1;
  }

  lua_pushnil(L);
  return 1;
}

const unsigned bbox_ptr_size = sizeof(BoundingBox *);

static int new_bbox(lua_State *L) {
  // create userdata for instance
  BBTransform **bbox = (BBTransform **)lua_newuserdata(L, bbox_ptr_size);

  // assign new bounding box
  const unsigned new_idx = bb_graphics.bbox_trans.size();
  bb_graphics.bbox_trans[new_idx] = BBTransform();
  bb_graphics.bbox_mirror[new_idx] = glm::uvec3(0);
  bb_graphics.bbox_joint_id[new_idx] = -1;

  (*bbox) = &bb_graphics.bbox_trans[new_idx];

  // set metatable
  luaL_getmetatable(L, BBOX_META_NAME);
  lua_setmetatable(L, -2);

  return 1;
}

struct bb_intersect {
  bool hit = false;
  float time = 0.f;
};

/** \brief Get intersection of OOBB w/ ray
 * per axis:
 *	- get angle (cos theta) between ray dir & oobb axis normal
 *	- get distance b/t ray origin & oobb center along oobb axis normal
 *	- calc intersection tmin & tmax & update "global" tmin & tmax
 */
static bb_intersect ray_bb_intersect(const glm::vec3 origin,
                                     const glm::vec3 dir, BBTransform &box) {
  bb_intersect output;

  // create direction vectors for oobb axis
  glm::mat4 rot = glm::rotate(glm::mat4(), box.rot.x, global_Xv3);
  rot = glm::rotate(rot, box.rot.y, global_Yv3);
  rot = glm::rotate(rot, box.rot.z, global_Zv3);
  const glm::vec3 half_ext = glm::abs(ref_bbox.max * box.scale);

  // select vector direction based on camera direction (correction for z axis)
  const glm::vec3 x_ax(rot[0].x, rot[0].y, rot[0].z);
  const glm::vec3 y_ax(rot[1].x, rot[1].y, rot[1].z);
  glm::vec3 z_ax(rot[2].x, rot[2].y, rot[2].z);
  z_ax *= dir.z > 0.f ? 1 : -1;
  const glm::vec3 dist_vec = box.pos - origin;

  const float epsilon = 0.0000000001f;

  // uses slab method of intersection (Kay and Kayjia siggraph)
  // furthest near t vs closest far t (t = time to intersect)
  double t_min = std::numeric_limits<double>::lowest();
  double t_max = std::numeric_limits<double>::max();

  // check x (intersection of x-component)
  float cos_theta = glm::dot(dir, x_ax);
  float dist = glm::dot(dist_vec, x_ax);
  if (glm::abs(cos_theta) > epsilon) {
    const float denom = 1.f / cos_theta;
    const double t1 = (dist - half_ext.x) * denom;
    const double t2 = (dist + half_ext.x) * denom;

    t_min = std::max(t_min, std::min(t1, t2));
    t_max = std::min(t_max, std::max(t1, t2));
  }

  // check y (intersection of y-component)
  cos_theta = glm::dot(dir, y_ax);
  dist = glm::dot(dist_vec, y_ax);
  if (glm::abs(cos_theta) > epsilon) {
    const float denom = 1.f / cos_theta;
    const double t1 = (dist - half_ext.y) * denom;
    const double t2 = (dist + half_ext.y) * denom;

    t_min = std::max(t_min, std::min(t1, t2));
    t_max = std::min(t_max, std::max(t1, t2));
  }

  // check z (intersection of z-component)
  cos_theta = glm::abs(glm::dot(dir, z_ax));
  dist = glm::dot(dist_vec, z_ax);
  if (glm::abs(cos_theta) > epsilon) {
    const float denom = 1.f / cos_theta;
    const double t1 = (dist - half_ext.z) * denom;
    const double t2 = (dist + half_ext.z) * denom;

    t_min = std::max(t_min, std::min(t1, t2));
    t_max = std::min(t_max, std::max(t1, t2));
  }

  output.hit = t_max >= t_min && t_max > 0.0;
  output.time = t_min;

  return output;
}

int bbox_index_func(lua_State *L) {
  BBTransform *bbox = *check_bbox(L);

  luaL_getmetatable(L, BBOX_META_NAME);  // find meta
  lua_pushvalue(L, 2);  // copy metatable on top of stack for rawget
  lua_rawget(L, -2);    // attempt to call method in table (nil if no method)

  if (lua_isnil(L, -1)) {
    /* found no method, so get value */
    return get_val(L);
  };

  return 1;  // returns method to lua
}

/** \brief Checks if bounding box intersects with ray */
static int check_intersect(lua_State *L) {
  BBTransform *bbox = *check_bbox(L);
  bool hit = false;
  float time = 0;

  int top = lua_gettop(L);
  if (top == 3 && bbox) {
    // get values for origin and ray
    read_buffer_from_lua<float>(L, float_buf6);
    glm::vec3 origin(float_buf6[0], float_buf6[1], float_buf6[2]);
    glm::vec3 dir(float_buf6[3], float_buf6[4], float_buf6[5]);

    const bb_intersect check = ray_bb_intersect(origin, dir, *bbox);
    hit = check.hit;
    time = check.time;
  }

  lua_pushboolean(L, hit);
  lua_pushnumber(L, time);
  return 2;
}

static int modify_bbox(lua_State *L) {
  int top = lua_gettop(L);
  int bbox_id = -1;

  if (top == 1) {
    bbox_id = (unsigned)luaL_checkinteger(L, -1);
    if (bb_graphics.bbox_trans.count(bbox_id) == 0) {
      bbox_id = -1;
    }
  }

  bool win_on = bbox_id >= 0;
  if (win_on) {
    BBTransform &bb = bb_graphics.bbox_trans[bbox_id];
    glm::uvec3 &bb_m = bb_graphics.bbox_mirror[bbox_id];
    int &bb_j = bb_graphics.bbox_joint_id[bbox_id];

    ImColor col(0.f, 0.f, 1.f, 1.f);
    ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_TitleBg, col);
    ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_TitleBgActive, col);

    ImGui::Begin("Modify Bounding Box", &win_on, 0);

    ImGui::Text("ID: %d", bbox_id);
    ImGui::Separator();

    // joint index on skeleton
    ImGui::Text("Joint ID", bbox_id);
    ImGui::InputInt("Joint", &bb_j);
    ImGui::Separator();

    // mirror along axiz
    ImGui::Text("Mirror (along axis selected)");
    bool bvec[] = {(bool)bb_m[0], (bool)bb_m[1], (bool)bb_m[2]};

    ImGui::Checkbox("x", &bvec[0]);
    ImGui::SameLine();
    ImGui::Checkbox("y", &bvec[1]);
    ImGui::SameLine();
    ImGui::Checkbox("z", &bvec[2]);

    // save values
    bb_m[0] = bvec[0] ? 1 : 0;
    bb_m[1] = bvec[1] ? 1 : 0;
    bb_m[2] = bvec[2] ? 1 : 0;

    ImGui::Separator();

		// level controls
		ImGui::Checkbox("Translate", &lvl_ctrl.flags[0]); ImGui::SameLine();
		ImGui::Checkbox("Rotate", &lvl_ctrl.flags[1]); ImGui::SameLine();
		ImGui::Checkbox("Scale", &lvl_ctrl.flags[2]);

    ImGui::End();

    ImGui::PopStyleColor(2);
  }

  return 0;
}

static const struct luaL_Reg bbox_lib[] = {
    {"new", new_bbox}, {"modify_bbox", modify_bbox}, {NULL, NULL}};

int luaopen_bbox(lua_State *L) {
  // get and log functions in metatable
  luaL_newmetatable(L, BBOX_META_NAME);  // create meta table
  lua_pushvalue(L, -1);                  /* duplicate the metatable */
  lua_setfield(L, -2, "__index");        /* mt.__index = mt */
  luaL_setfuncs(L, bbox_methods, 0);     /* register metamethods */

  // library functions
  luaL_newlib(L, bbox_lib);
  return 1;
}

//*****************************************************************************

#define LVLCTR_META_NAME "LuaClass.LvlCtrl"
#define check_lvlctrl(L) (LvlCtrl **)luaL_checkudata(L, 1, LVLCTR_META_NAME)

static int set_ctrl(lua_State *L) {
  LvlCtrl *lc_ptr = *check_lvlctrl(L);
  int idx = (int)luaL_checkinteger(L, 2);

  switch (idx) {
    case (int)LvlCtrlEnums::TRANSLATE:
      lc_ptr->flags[idx] ^= 1;
      if (lc_ptr->flags[idx]) {
        lc_ptr->flags[(int)LvlCtrlEnums::ROTATE] = false;
        lc_ptr->flags[(int)LvlCtrlEnums::SCALE] = false;
      }
      break;
    case (int)LvlCtrlEnums::ROTATE:
      lc_ptr->flags[idx] ^= 1;
      if (lc_ptr->flags[idx]) {
        lc_ptr->flags[(int)LvlCtrlEnums::TRANSLATE] = false;
        lc_ptr->flags[(int)LvlCtrlEnums::SCALE] = false;
      }
      break;
    case (int)LvlCtrlEnums::SCALE:
      lc_ptr->flags[idx] ^= 1;
      if (lc_ptr->flags[idx]) {
        lc_ptr->flags[(int)LvlCtrlEnums::ROTATE] = false;
        lc_ptr->flags[(int)LvlCtrlEnums::TRANSLATE] = false;
      }
      break;
    case (int)LvlCtrlEnums::X:
      lc_ptr->flags[idx] ^= 1;
      if (lc_ptr->flags[idx]) {
        lc_ptr->flags[(int)LvlCtrlEnums::Y] = false;
        lc_ptr->flags[(int)LvlCtrlEnums::Z] = false;
      }
      break;
    case (int)LvlCtrlEnums::Y:
      lc_ptr->flags[idx] ^= 1;
      if (lc_ptr->flags[idx]) {
        lc_ptr->flags[(int)LvlCtrlEnums::X] = false;
        lc_ptr->flags[(int)LvlCtrlEnums::Z] = false;
      }
      break;
    case (int)LvlCtrlEnums::Z:
      lc_ptr->flags[idx] ^= 1;
      if (lc_ptr->flags[idx]) {
        lc_ptr->flags[(int)LvlCtrlEnums::Y] = false;
        lc_ptr->flags[(int)LvlCtrlEnums::X] = false;
      }
      break;
    default:
      break;
  }
  return 0;
}
static int get_ctrl(lua_State *L) {
  LvlCtrl *lc_ptr = *check_lvlctrl(L);
  unsigned idx = (unsigned)luaL_checkinteger(L, 2);

  if (idx < (unsigned)LvlCtrlEnums::NUM_FLAGS) {
    lua_pushboolean(L, lc_ptr->flags[idx]);
    return 1;
  }
  lua_pushnil(L);
  return 1;
}

static const struct luaL_Reg lvlctrl_methods[] = {
    {"__index", get_ctrl}, {"__newindex", set_ctrl}, {NULL, NULL}};

const unsigned lvlctrl_ptr_size = sizeof(LvlCtrl *);

static int get_lvlctrl(lua_State *L) {
  // create userdata for instance
  LvlCtrl **ctrl = (LvlCtrl **)lua_newuserdata(L, lvlctrl_ptr_size);

  // assign level controller
  (*ctrl) = &lvl_ctrl;

  // set metatable
  luaL_getmetatable(L, LVLCTR_META_NAME);
  lua_setmetatable(L, -2);

  return 1;
}

static const struct luaL_Reg lvlctrl_lib[] = {{"get", get_lvlctrl},
                                              {NULL, NULL}};

int luaopen_lvlctrl(lua_State *L) {
  // get and log functions in metatable
  luaL_newmetatable(L, LVLCTR_META_NAME);  // create meta table
  lua_pushvalue(L, -1);                    /* duplicate the metatable */
  lua_setfield(L, -2, "__index");          /* mt.__index = mt */
  luaL_setfuncs(L, lvlctrl_methods, 0);    /* register metamethods */

  // library functions
  luaL_newlib(L, lvlctrl_lib);
  return 1;
}

//*****************************************************************************

// log lua function that can be called in scripts thru this function
void BBoxSkinnedMesh_func_register(lua_State *L);

// initialize gpu stuff ( run once during level update )
int init_gpu_stuff(lua_State *L);

// render grid
void render_grid();

// fill in buffer
void fill_buffer(const BoundingBox &bbox);

// render bbox
void render_bbox();

// Proxy struct that enables reflection
struct BBoxSkinnedMesh_reflect : public ddLvlPrototype {
  BBoxSkinnedMesh_reflect() {
    add_lua_function("BBoxSkinnedMesh", BBoxSkinnedMesh_func_register);

    bb_graphics.generate_grid();

    // set up reference bounding box
    ref_bbox.min = glm::vec3(-0.5f);
    ref_bbox.max = glm::vec3(0.5f);
    ref_bbox.SetCorners();
    fill_buffer(ref_bbox);
  }
};

void BBoxSkinnedMesh_func_register(lua_State *L) {
  // log functions using register_callback_lua
  register_callback_lua(L, "init_gpu_stuff", init_gpu_stuff);

  // log bounding box libraries
	luaL_requiref(L, "ddBBox", luaopen_bbox, 1);
	luaL_requiref(L, "Lvlctrl", luaopen_lvlctrl, 1);

  // clear stack
  int top = lua_gettop(L);
  lua_pop(L, top);
}

int init_gpu_stuff(lua_State *L) {
  // shader init
  cbuff<256> fname;
  bb_graphics.bbox_sh.init();

  fname.format("%s/BBoxSkinnedMesh/%s", PROJECT_DIR, "LineRend_V.vert");
  bb_graphics.bbox_sh.create_vert_shader(fname.str());
  fname.format("%s/BBoxSkinnedMesh/%s", PROJECT_DIR, "LineRend_F.frag");
  bb_graphics.bbox_sh.create_frag_shader(fname.str());

  // grid lines vao
  ddGPUFrontEnd::create_vao(bb_graphics.grid_vao);

  // grid line storage buffer
  ddGPUFrontEnd::create_storage_buffer(bb_graphics.grid_ssbo,
                                       num_grid_points * sizeof(float));

  // grid line index buffer
  ddGPUFrontEnd::create_index_buffer(bb_graphics.grid_ebo,
                                     num_grid_indices * sizeof(unsigned),
                                     bb_graphics.grid_indices);

  // bind vao and set storage data
  ddGPUFrontEnd::bind_storage_buffer_atrribute(
      bb_graphics.grid_vao, bb_graphics.grid_ssbo, ddAttribPrimitive::FLOAT, 0,
      3, 3 * sizeof(float), 0);
  ddGPUFrontEnd::set_storage_buffer_contents(bb_graphics.grid_ssbo,
                                             num_grid_points * sizeof(float), 0,
                                             bb_graphics.grid_points);
  ddGPUFrontEnd::bind_index_buffer(bb_graphics.grid_vao, bb_graphics.grid_ebo);

  // bounding box (8 vec3 floats & 24 floats) storage buffer allocation
  ddGPUFrontEnd::create_vao(bb_graphics.bbox_vao);
  ddGPUFrontEnd::create_storage_buffer(bb_graphics.bbox_ssbo,
                                       8 * sizeof(glm::vec3));
  ddGPUFrontEnd::set_storage_buffer_contents(
      bb_graphics.bbox_ssbo, 8 * 3 * sizeof(float), 0, bb_graphics.bbox_buffer);
  ddGPUFrontEnd::create_index_buffer(
      bb_graphics.bbox_ebo, 8 * 3 * sizeof(unsigned), bb_graphics.bbox_indices);
  ddGPUFrontEnd::bind_storage_buffer_atrribute(
      bb_graphics.bbox_vao, bb_graphics.bbox_ssbo, ddAttribPrimitive::FLOAT, 0,
      3, 3 * sizeof(float), 0);
  ddGPUFrontEnd::bind_index_buffer(bb_graphics.bbox_vao, bb_graphics.bbox_ebo);

  // set up particle task for grid
  bb_graphics.draw_grid.lifespan = 1.f;
  bb_graphics.draw_grid.remain_on_q = true;
  bb_graphics.draw_grid.rfunc = render_grid;

  ddParticleSys::add_task(bb_graphics.draw_grid);

  // set up particle task for bounding boxes
  bb_graphics.draw_bbox.lifespan = 1.f;
  bb_graphics.draw_bbox.remain_on_q = true;
  bb_graphics.draw_bbox.rfunc = render_bbox;

  ddParticleSys::add_task(bb_graphics.draw_bbox);

  // set background color
  ddCam *cam = ddSceneManager::get_active_cam();
  if (cam) {
    cam->background_color = glm::vec4(0.2f, 0.2f, 0.2f, 1.f);
  }

  return 0;
}

void render_grid() {
  // bind and render lines of grid
  ddCam *cam = ddSceneManager::get_active_cam();

  if (cam) {
    bb_graphics.bbox_sh.use();

    // get camera matrices
    const glm::mat4 v_mat = ddSceneManager::calc_view_matrix(cam);
    const glm::mat4 p_mat = ddSceneManager::calc_p_proj_matrix(cam);

    // set uniforms
    bb_graphics.bbox_sh.set_uniform((int)RE_Line::MVP_m4x4, p_mat * v_mat);
    bb_graphics.bbox_sh.set_uniform((int)RE_Line::color_v4,
                                    glm::vec4(1.f, 1.f, 1.f, 0.5f));

    // draw lines
    ddGPUFrontEnd::draw_indexed_lines_vao(bb_graphics.grid_vao,
                                          num_grid_indices, 0);
  }
}

void fill_buffer(const BoundingBox &bbox) {
  auto set_val = [](const unsigned idx, const glm::vec3 &corner) {
    bb_graphics.bbox_buffer[idx * 3] = corner.x;
    bb_graphics.bbox_buffer[idx * 3 + 1] = corner.y;
    bb_graphics.bbox_buffer[idx * 3 + 2] = corner.z;
  };

  for (unsigned i = 0; i < 8; i++) {
    switch (i) {
      case 0:
        set_val(i, bbox.corner1);
        break;
      case 1:
        set_val(i, bbox.corner2);
        break;
      case 2:
        set_val(i, bbox.corner3);
        break;
      case 3:
        set_val(i, bbox.corner4);
        break;
      case 4:
        set_val(i, bbox.corner5);
        break;
      case 5:
        set_val(i, bbox.corner6);
        break;
      case 6:
        set_val(i, bbox.corner7);
        break;
      case 7:
        set_val(i, bbox.corner8);
        break;
      default:
        break;
    }
  }
}

void render_bbox() {
  // bind and render lines of bounding box
  ddCam *cam = ddSceneManager::get_active_cam();

  if (cam) {
    bb_graphics.bbox_sh.use();

    // get camera matrices
    const glm::mat4 v_mat = ddSceneManager::calc_view_matrix(cam);
    const glm::mat4 p_mat = ddSceneManager::calc_p_proj_matrix(cam);

    // set uniforms
    // bb_graphics.bbox_sh.set_uniform((int)RE_Line::MVP_m4x4, p_mat * v_mat);

    // draw bounding boxes
    unsigned idx = 0;
    // for (auto &box : bb_graphics.bbox_container) {
    //  // color
    //  bb_graphics.bbox_sh.set_uniform((int)RE_Line::color_v4,
    //                                  glm::vec4(1.f, 0.f, 0.f, 1.f));

    //  fill_buffer(box.second);
    //  ddGPUFrontEnd::set_storage_buffer_contents(bb_graphics.bbox_ssbo,
    //                                             8 * 3 * sizeof(float), 0,
    //                                             bb_graphics.bbox_buffer);
    //  ddGPUFrontEnd::draw_indexed_lines_vao(bb_graphics.bbox_vao, 24, 0);

    //  const glm::uvec3 mirror = bb_graphics.bbox_mirror[idx];
    //  if (mirror.x == 1 || mirror.y == 1 || mirror.z == 1) {
    //    // color
    //    bb_graphics.bbox_sh.set_uniform((int)RE_Line::color_v4,
    //                                    glm::vec4(0.f, 0.f, 1.f, 1.f));

    //    // mirror the bounding box
    //    BoundingBox m_box;
    //    glm::vec3 m_vec(1.f);
    //    m_vec.x = mirror.x == 1 ? -1 : 1;
    //    m_vec.y = mirror.y == 1 ? -1 : 1;
    //    m_vec.z = mirror.z == 1 ? -1 : 1;

    //    m_box.min = box.second.min * m_vec;
    //    m_box.max = box.second.max * m_vec;
    //    m_box.SetCorners();

    //    fill_buffer(m_box);
    //    ddGPUFrontEnd::set_storage_buffer_contents(bb_graphics.bbox_ssbo,
    //                                               8 * 3 * sizeof(float), 0,
    //                                               bb_graphics.bbox_buffer);
    //    ddGPUFrontEnd::draw_indexed_lines_vao(bb_graphics.bbox_vao, 24, 0);
    //  }
    //}
    // loop thru transforms
    for (auto &box : bb_graphics.bbox_trans) {
      glm::mat4 m_mat =
          createMatrix(box.second.pos, box.second.rot, box.second.scale);
      // set uniforms
      bb_graphics.bbox_sh.set_uniform((int)RE_Line::MVP_m4x4,
                                      p_mat * v_mat * m_mat);
      bb_graphics.bbox_sh.set_uniform((int)RE_Line::color_v4,
                                      glm::vec4(1.f, 0.f, 0.f, 1.f));
      ddGPUFrontEnd::draw_indexed_lines_vao(bb_graphics.bbox_vao, 24, 0);

      // mirror
      const glm::uvec3 mirror = bb_graphics.bbox_mirror[idx];
      if (mirror.x == 1 || mirror.y == 1 || mirror.z == 1) {
        // color
        bb_graphics.bbox_sh.set_uniform((int)RE_Line::color_v4,
                                        glm::vec4(0.f, 0.f, 1.f, 1.f));

        // mirror the model matrix
        glm::vec3 m_vec(1.f);
        m_vec.x = mirror.x == 1 ? -1 : 1;
        m_vec.y = mirror.y == 1 ? -1 : 1;
        m_vec.z = mirror.z == 1 ? -1 : 1;
        m_mat = glm::scale(glm::mat4(), m_vec) * m_mat;

        bb_graphics.bbox_sh.set_uniform((int)RE_Line::MVP_m4x4,
                                        p_mat * v_mat * m_mat);
        ddGPUFrontEnd::draw_indexed_lines_vao(bb_graphics.bbox_vao, 24, 0);
      }
      idx++;
    }
  }
}

// log reflection
BBoxSkinnedMesh_reflect BBoxSkinnedMesh_proxy;
