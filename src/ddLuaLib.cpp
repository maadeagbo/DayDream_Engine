#include "ddLuaLib.h"
#include <string>
#include "ddAssetManager.h"
#include "ddSceneManager.h"
#include "ddTerminal.h"
#include "ddTimer.h"

#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

//****************************************************************************
// Base Agent
//****************************************************************************
#include "ddLuaLib_ddAgent.h"
//****************************************************************************
// Camera
//****************************************************************************
#include "ddLuaLib_ddCamera.h"
//****************************************************************************
// Material
//****************************************************************************
#include "ddLuaLib_ddMaterial.h"
//****************************************************************************
// Models
//****************************************************************************
#include "ddLuaLib_ddModelData.h"
//****************************************************************************
// Lights
//****************************************************************************
#include "ddLuaLib_ddLight.h"
//****************************************************************************
// Animation
//****************************************************************************
#include "ddLuaLib_Animation.h"

namespace {
DD_FuncBuff fb;
}

/** \brief Print string to terminal */
static int ddprint(lua_State *L) {
  int top = lua_gettop(L); /* arguments */
  std::string out = "";
  for (int i = 1; i <= top; i++) {
    out += lua_tostring(L, i);
  }
  ddTerminal::post(out.c_str());

  return 0;
}

/** \brief Average frame time */
static int ddftime(lua_State *L) {
  lua_pushnumber(L, ddTime::get_avg_frame_time());
  return 1;
}

/** \brief Engine time */
static int ddgtime(lua_State *L) {
  lua_pushnumber(L, ddTime::get_time_float());
  return 1;
}

/** \brief High-resolution engine time */
static int dd_high_res_time(lua_State *L) {
  lua_pushinteger(L, ddTime::GetHiResTime());
  return 1;
}

/** \brief Hashes string to uint64_t value */
static int dd_hash(lua_State *L) {
  int top = lua_gettop(L);
  if (top == 1 && lua_isstring(L, -1)) {
    lua_pushinteger(L, getCharHash(lua_tostring(L, -1)));
  } else {
    lua_pushnil(L);
  }
  return 1;
}

/** \brief Checks if mouse is hovering over imgui module */
static int dd_imgui_active(lua_State *L) {
  // get io for mouse & keyboard management
  ImGuiIO &imgui_io = ImGui::GetIO();
  lua_pushboolean(L, imgui_io.WantCaptureMouse);
  return 1;
}

/** \brief Screen resolution */
static int dd_scr_dimensions(lua_State *L) {
  glm::uvec2 dim = ddSceneManager::get_screen_dimensions();
  lua_pushnumber(L, dim.x);
  lua_pushnumber(L, dim.y);
  return 2;
}

/** \brief Returns ray from mouse click */
static int dd_raycast(lua_State *L) {
  float scr_mx = -1.f, scr_my = -1.f;
  ddCam *cam = nullptr;

  int top = lua_gettop(L);
  if (top == 3) {
    int arg = 1;
    // get camera
    if (lua_isnumber(L, arg)) {
      cam = find_ddCam((size_t)lua_tointeger(L, arg));
      if (!cam) {
        ddTerminal::post("[error] raycast::Invalid 1st arg (cam id : integer)");
      }
    }

    // get mouse x
    arg++;
    if (lua_isnumber(L, arg)) {
      scr_mx = (float)lua_tonumber(L, arg);
    } else {
      ddTerminal::post("[error] raycast::Invalid 2nd arg (mouse x : float)");
    }

    // get mouse y
    arg++;
    if (lua_isnumber(L, arg)) {
      scr_my = (float)lua_tonumber(L, arg);
    } else {
      ddTerminal::post("[error] raycast::Invalid 3rd arg (mouse y : float)");
    }
  }

  if (cam && scr_mx >= 0.f && scr_my >= 0.f) {
    glm::mat4 inv_v_mat = glm::inverse(ddSceneManager::calc_view_matrix(cam));
    glm::mat4 inv_p_mat = glm::inverse(ddSceneManager::calc_p_proj_matrix(cam));

    glm::uvec2 scr_dim = ddSceneManager::get_screen_dimensions();
    //glm::vec4 view_p = glm::vec4(0.f, 0.f, (float)scr_dim.x, (float)scr_dim.y);
    const float _x = scr_mx / (scr_dim.x * 0.5f) - 1.0f;
    // top left is 0
    const float _y = ((float)scr_dim.y - scr_my) / (scr_dim.y * 0.5f) - 1.0f;

    // ddTerminal::f_post("Coord: %.3f, %.3f", _x, _y);

    // clip space to view space
    glm::vec4 ray_v = inv_p_mat * glm::vec4(_x, _y, -1.f, 1.f);
    // view to world space
    glm::vec4 ray_w = inv_v_mat * glm::vec4(ray_v.x, ray_v.y, -1.f, 0.f);
    glm::vec3 ray = glm::normalize(glm::vec3(ray_w));

    // push to script
    push_vec3_to_lua(L, ray.x, ray.y, ray.z);
    return 1;
  }

  // return nil object
  ddTerminal::post("[error]raycast::Invalid arguments (int, float, float)");
  lua_pop(L, top);
  lua_pushnil(L);
  return 1;
}

/** \brief Checks if object (bounding box) intersects with ray */
static int dd_ray_bbox_check(lua_State *L) {
  int top = lua_gettop(L);
  if (top == 3) {
    size_t ag_id = 0;
    // get agent id
    if (lua_isinteger(L, 3)) {
      ag_id = (size_t)lua_tointeger(L, 3);
    } else {
      ddTerminal::post("[error]ray_bbox_check::Invalid 3rd arg (id : int)");
      // return nil
      lua_pushnil(L);
      return 1;
    }

    // get values for origin and ray
    dd_array<float> buff(6);
    read_buffer_from_lua<float>(L, buff);
    glm::vec3 origin(buff[0], buff[1], buff[2]);
    glm::vec3 dir(buff[3], buff[4], buff[5]);

    const bool check = ddSceneManager::ray_bbox_intersect(origin, dir, ag_id);
    lua_pushboolean(L, check);
    return 1;
  }

  // return nil object
  ddTerminal::post(
      "[error]ray_bbox_check::Invalid arguments ({float}, {float}"
      ", int)");
  lua_pushnil(L);
  return 1;
}

// ddLib library
static const struct luaL_Reg dd_lib[] = {{"print", ddprint},
                                         {"ftime", ddftime},
                                         {"gtime", ddgtime},
                                         {"hres_time", dd_high_res_time},
                                         {"get_hash", dd_hash},
                                         {"mouse_over_UI", dd_imgui_active},
                                         {"scr_dimensions", dd_scr_dimensions},
                                         {"raycast", dd_raycast},
                                         {"ray_bbox_check", dd_ray_bbox_check},
                                         {NULL, NULL}};

int luaopen_ddLib(lua_State *L) {
  // library functions
  luaL_newlib(L, dd_lib);
  return 1;
}

//****************************************************************************
// Register
//****************************************************************************

void register_dd_libraries(lua_State *L) {
  // ddlib module
  luaL_requiref(L, "ddLib", luaopen_ddLib, 1);
  // camera module
  luaL_requiref(L, "ddCam", luaopen_ddCam, 1);
  // agent module
  luaL_requiref(L, "ddAgent", luaopen_ddAgent, 1);
  // material module
  luaL_requiref(L, "ddMat", luaopen_ddMat, 1);
  // model module
  luaL_requiref(L, "ddModel", luaopen_ddModelData, 1);
  // light module
  luaL_requiref(L, "ddLight", luaopen_ddLBulb, 1);
  // animation module
  luaL_requiref(L, "ddAnimation", luaopen_Animation, 1);

  // clear stack
  int top = lua_gettop(L);
  lua_pop(L, top);
}
