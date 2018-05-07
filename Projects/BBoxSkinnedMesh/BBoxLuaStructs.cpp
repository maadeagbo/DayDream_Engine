#include "BBoxLuaStructs.h"
#include "ddInput.h"
#include "ddModel.h"
#include "ddTimer.h"
#include "imgui.h"
#include "imgui_tabs.h"

namespace {
// bbox container
std::map<unsigned, BBTransform> bbox_trans;

cbuff<8> pos_str = "pos";
cbuff<8> rot_str = "rot";
cbuff<8> scale_str = "scale";
cbuff<8> mirror_str = "mirror";
cbuff<8> joint_str = "jnts";

cbuff<8> output_str = "output";

dd_array<float> float_buf3(3);
dd_array<int64_t> int_buf3(3);
dd_array<float> float_buf6(6);

BoundingBox ref_bbox = BoundingBox(glm::vec3(-0.5), glm::vec3(0.5));

// text buffer
char export_name[32];
char skeleton_in[512];

// time tracker
float check_time = 0.f;

// tab bar controls
const char *tab_name[2] = {"Create", "Import"};
bool tab_open[2] = {true, true};
}  // namespace

//*****************************************************************************
//*****************************************************************************

#define BBOX_META_NAME "LuaClass.BBox"
#define check_bbox(L) (BBTransform **)luaL_checkudata(L, 1, BBOX_META_NAME)

static int set_val(lua_State *L);
static int get_val(lua_State *L);
static int bbox_index_func(lua_State *L);
static int check_intersect(lua_State *L);
static int get_corners(lua_State *L);
static int to_string(lua_State *L);

// method list
static const struct luaL_Reg bbox_methods[] = {
    {"__index", bbox_index_func}, {"__newindex", set_val},
    {"__tostring", to_string},    {"intersect", check_intersect},
    {"get_corners", get_corners}, {NULL, NULL}};

int set_val(lua_State *L) {
  BBTransform *bbox = *check_bbox(L);
  const char *arg = (const char *)luaL_checkstring(L, 2);

  // position
  if (pos_str.compare(arg) == 0) {
    read_buffer_from_lua(L, float_buf3);
    bbox->pos.x = float_buf3[0];
    bbox->pos.y = float_buf3[1];
    bbox->pos.z = float_buf3[2];
  } else if (rot_str.compare(arg) == 0) {
    read_buffer_from_lua(L, float_buf3);
    // rotation
    bbox->rot.x = float_buf3[0];
    bbox->rot.y = float_buf3[1];
    bbox->rot.z = float_buf3[2];
  } else if (scale_str.compare(arg) == 0) {
    read_buffer_from_lua(L, float_buf3);
    // scale
    bbox->scale.x = float_buf3[0];
    bbox->scale.y = float_buf3[1];
    bbox->scale.z = float_buf3[2];
  } else if (mirror_str.compare(arg) == 0) {
    read_buffer_from_lua(L, int_buf3);
    // mirror
    bbox->mirror.x = (unsigned)int_buf3[0];
    bbox->mirror.y = (unsigned)int_buf3[1];
    bbox->mirror.z = (unsigned)int_buf3[2];
  } else if (joint_str.compare(arg) == 0) {
    read_buffer_from_lua(L, int_buf3);
    // joint ids
    bbox->joint_ids.x = int_buf3[0];
    bbox->joint_ids.y = int_buf3[1];
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
  } else if (mirror_str.compare(arg) == 0) {
    // mirror
    push_ivec3_to_lua(L, bbox->mirror.x, bbox->mirror.y, bbox->mirror.z);
    return 1;
  } else if (joint_str.compare(arg) == 0) {
    // joint ids
    push_ivec3_to_lua(L, bbox->joint_ids.x, bbox->joint_ids.y, 0);
    return 1;
  }

  lua_pushnil(L);
  return 1;
}

static int to_string(lua_State *L) {
  BBTransform *bb = *check_bbox(L);
  std::string buff;

  cbuff<128> out;
  out.format("\n  pos: %.3f, %.3f, %.3f", bb->pos.x, bb->pos.y, bb->pos.z);
  buff += out.str();

  out.format("\n  sc: %.3f, %.3f, %.3f", bb->scale.x, bb->scale.y, bb->scale.z);
  buff += out.str();

  out.format("\n  rot: %.3f, %.3f, %.3f", bb->rot.x, bb->rot.y, bb->rot.z);
  buff += out.str();

  lua_pushstring(L, buff.c_str());
  return 1;
}

static int get_corners(lua_State *L) {
  BBTransform *bbox = *check_bbox(L);

  glm::mat4 model_mat = createMatrix(bbox->pos, bbox->rot, bbox->scale);

  // apply mirror operation if necessary
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  bool flag = (bool)lua_toboolean(L, 2);
  if (flag) {
    glm::vec3 s_vec = glm::vec3(1.f);
    s_vec.x = (bbox->mirror.x == 1) ? -1.f : 1.f;
    s_vec.y = (bbox->mirror.y == 1) ? -1.f : 1.f;
    s_vec.z = (bbox->mirror.z == 1) ? -1.f : 1.f;
    model_mat = glm::scale(glm::mat4(), s_vec) * model_mat;
  }

  // get vertices
  BoundingBox _box = ref_bbox.transformCorners(model_mat);

  // push vertices
  push_vec3_to_lua(L, _box.corner1.x, _box.corner1.y, _box.corner1.z);
  push_vec3_to_lua(L, _box.corner2.x, _box.corner2.y, _box.corner2.z);
  push_vec3_to_lua(L, _box.corner3.x, _box.corner3.y, _box.corner3.z);
  push_vec3_to_lua(L, _box.corner4.x, _box.corner4.y, _box.corner4.z);
  push_vec3_to_lua(L, _box.corner5.x, _box.corner5.y, _box.corner5.z);
  push_vec3_to_lua(L, _box.corner6.x, _box.corner6.y, _box.corner6.z);
  push_vec3_to_lua(L, _box.corner7.x, _box.corner7.y, _box.corner7.z);
  push_vec3_to_lua(L, _box.corner8.x, _box.corner8.y, _box.corner8.z);

  return 8;
}

const unsigned bbox_ptr_size = sizeof(BoundingBox *);

static int new_bbox(lua_State *L) {
  // create userdata for instance
  BBTransform **bbox = (BBTransform **)lua_newuserdata(L, bbox_ptr_size);

  // assign new bounding box
  const unsigned new_idx = bbox_trans.size();
  bbox_trans[new_idx] = BBTransform();

  (*bbox) = &bbox_trans[new_idx];

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
  BBTransform *bb_t = nullptr;
  LvlCtrl *l_ctrl = nullptr;

  // get bounding box index
  if (top >= 1) {
    bb_t = *check_bbox(L);
  }
  // get LvlCtrl object
  if (top >= 2) {
    l_ctrl = *((LvlCtrl **)luaL_checkudata(L, 2, "LuaClass.LvlCtrl"));
  }

  bool win_on = bb_t && l_ctrl;
  if (win_on) {
    ImVec4 col(0.f, 0.f, 1.f, 1.f);
    ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_TitleBg, col);
    ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_TitleBgActive, col);

    const InputData inp = ddInput::get_input();
    if (inp.keys[DD_Keys::MOUSE_RIGHT].active) {
      ImGui::SetNextWindowPos(ImVec2(inp.keys[DD_Keys::MOUSE_X].order,
                                     inp.keys[DD_Keys::MOUSE_Y].order));
    }

    ImGui::Begin("Modify Bounding Box", &win_on,
                 ImGuiWindowFlags_AlwaysAutoResize);

    // ImGui::Text("ID: %d", bbox_id);
    // ImGui::Separator();

    // mirror along axiz
    ImGui::Text("Mirror (along axis selected)");
    bool bvec[] = {(bool)bb_t->mirror[0], (bool)bb_t->mirror[1],
                   (bool)bb_t->mirror[2]};

    ImGui::Checkbox("x", &bvec[0]);
    ImGui::SameLine();
    ImGui::Checkbox("y", &bvec[1]);
    ImGui::SameLine();
    ImGui::Checkbox("z", &bvec[2]);

    // save values
    bb_t->mirror[0] = bvec[0] ? 1 : 0;
    bb_t->mirror[1] = bvec[1] ? 1 : 0;
    bb_t->mirror[2] = bvec[2] ? 1 : 0;

    ImGui::Separator();

    // joint index on skeleton
    ImGui::Text("Joint IDs");
    ImGui::InputInt("Joint", &bb_t->joint_ids[0]);
    if (bvec[0] || bvec[1] || bvec[2]) {
      ImGui::InputInt("Mirrored Joint", &bb_t->joint_ids[1]);
    }
    ImGui::Separator();

    // level controls
    ImGui::Checkbox("Scale", &l_ctrl->flags[2]);
    ImGui::SameLine();
    ImGui::Checkbox("Rotate", &l_ctrl->flags[1]);
    ImGui::SameLine();
    ImGui::Checkbox("Translate", &l_ctrl->flags[0]);

    ImGui::Checkbox("X", &l_ctrl->flags[3]);
    ImGui::SameLine();
    ImGui::Checkbox("Y", &l_ctrl->flags[4]);
    ImGui::SameLine();
    ImGui::Checkbox("Z", &l_ctrl->flags[5]);

    ImGui::Separator();

    // numerical view & edit
    ImGui::InputFloat3("Pos", &bb_t->pos[0]);
    ImGui::InputFloat3("Rot", &bb_t->rot[0]);
    ImGui::InputFloat3("Scale", &bb_t->scale[0]);

    ImGui::End();

    ImGui::PopStyleColor(2);
  }

  return 0;
}

static const struct luaL_Reg bbox_lib[] = {
    {"new", new_bbox}, {"modify_bbox", modify_bbox}, {NULL, NULL}};

std::map<unsigned, BBTransform> &get_all_transforms() { return bbox_trans; }

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
//*****************************************************************************

#define LVLCTR_META_NAME "LuaClass.LvlCtrl"
#define check_lvlctrl(L) (LvlCtrl **)luaL_checkudata(L, 1, LVLCTR_META_NAME)

static int set_ctrl(lua_State *L) {
  LvlCtrl *lc_ptr = *check_lvlctrl(L);
  int idx = (int)luaL_checkinteger(L, 2);
  luaL_checktype(L, 3, LUA_TBOOLEAN);
  bool flag = (bool)lua_toboolean(L, 3);

  if (idx < (int)LvlCtrlEnums::NUM_FLAGS) {
    lc_ptr->flags[idx] = flag;

    // turn off other options
    if (flag) {
      switch (idx) {
        case (int)LvlCtrlEnums::TRANSLATE:
          lc_ptr->flags[(int)LvlCtrlEnums::ROTATE] = false;
          lc_ptr->flags[(int)LvlCtrlEnums::SCALE] = false;
          break;
        case (int)LvlCtrlEnums::ROTATE:
          lc_ptr->flags[(int)LvlCtrlEnums::TRANSLATE] = false;
          lc_ptr->flags[(int)LvlCtrlEnums::SCALE] = false;
          break;
        case (int)LvlCtrlEnums::SCALE:
          lc_ptr->flags[(int)LvlCtrlEnums::ROTATE] = false;
          lc_ptr->flags[(int)LvlCtrlEnums::TRANSLATE] = false;
          break;
        case (int)LvlCtrlEnums::X:
          lc_ptr->flags[(int)LvlCtrlEnums::Y] = false;
          lc_ptr->flags[(int)LvlCtrlEnums::Z] = false;
          break;
        case (int)LvlCtrlEnums::Y:
          lc_ptr->flags[(int)LvlCtrlEnums::X] = false;
          lc_ptr->flags[(int)LvlCtrlEnums::Z] = false;
          break;
        case (int)LvlCtrlEnums::Z:
          lc_ptr->flags[(int)LvlCtrlEnums::Y] = false;
          lc_ptr->flags[(int)LvlCtrlEnums::X] = false;
          break;
        default:
          break;
      }
    }
  }

  return 0;
}

static int get_ctrl(lua_State *L) {
  LvlCtrl *lc_ptr = *check_lvlctrl(L);
  if ((bool)lua_isinteger(L, 2)) {
    unsigned idx = (unsigned)luaL_checkinteger(L, 2);

    if (idx < (unsigned)LvlCtrlEnums::NUM_FLAGS) {
      lua_pushboolean(L, lc_ptr->flags[idx]);
      return 1;
    } 
  } else if ((bool)lua_isstring(L, 2)) {
    cbuff<8> id = (const char*)luaL_checkstring(L, 2);

    if (id == output_str) {
      lua_pushstring(L, lc_ptr->output.str());
      return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

/**
 * \brief garbage collect LvlCtrl
 */
static int lvlctrl_gc(lua_State *L) {
  LvlCtrl *lc_ptr = *check_lvlctrl(L);
  delete lc_ptr;

  return 0;
}

static const struct luaL_Reg lvlctrl_methods[] = {{"__index", get_ctrl},
                                                  {"__newindex", set_ctrl},
                                                  {"__gc", lvlctrl_gc},
                                                  {NULL, NULL}};

const unsigned lvlctrl_ptr_size = sizeof(LvlCtrl *);

static int get_lvlctrl(lua_State *L) {
  // create userdata for instance
  LvlCtrl **ctrl = (LvlCtrl **)lua_newuserdata(L, lvlctrl_ptr_size);

  // assign level controller
  (*ctrl) = new LvlCtrl();

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
//*****************************************************************************

int render_sk_controls(lua_State *L) {
  bool win_on = true;
  bool output[2] = {false, false};
  check_time += ddTime::get_frame_time();
  
  LvlCtrl *lc_ptr = *check_lvlctrl(L);

  ImGui::Begin("Bounding Box Controls", &win_on,
               ImGuiWindowFlags_AlwaysAutoResize);

  // tab bar "Creation"
  ImGui::BeginTabBar("Box Creation", ImGuiTabBarFlags_SizingPolicyDefault_);

  for (unsigned i = 0; i < 2; i++) {
    //const bool selected = ImGui::TabItem(tab_name[i], &tab_open[i]);
    const bool selected = ImGui::TabItem(tab_name[i]);
    if (!selected) continue;

    switch (i) {
      case 0:
        // button for creating new bounding box
        if (ImGui::Button("New Box") && check_time > 0.5f) {
          output[0] = true;
          check_time = 0.f;
        }
        ImGui::SameLine();

        // button for exporting
        if (ImGui::Button("Export") && check_time > 0.5f) {
          output[1] = true;
          check_time = 0.f;
        }

        // text input box for export
        if (ImGui::InputText("Export ID", export_name, 32,
                              ImGuiInputTextFlags_EnterReturnsTrue)) {
          lc_ptr->output = export_name;
        }
        break;
      case 1:
        // text input box for loading skeleton
        if (ImGui::InputText("Skeleton Path", skeleton_in, 512,
                              ImGuiInputTextFlags_EnterReturnsTrue)) {
          //
        }
        break;
      default:
        break;
    }
  }

  // tab bar "Creation"
  ImGui::EndTabBar();

  ImGui::End();

  lua_pushboolean(L, output[0]);
  lua_pushboolean(L, output[1]);
  return 2;
}
