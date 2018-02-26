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

static int dd_high_res_time(lua_State *L) {
  lua_pushinteger(L, ddTime::GetHiResTime());
  return 1;
}

static int dd_hash(lua_State *L) {
  int top = lua_gettop(L);
  if (top == 1 && lua_isstring(L, -1)) {
    lua_pushinteger(L, getCharHash(lua_tostring(L, -1)));
  } else {
    lua_pushnil(L);
  }
  return 1;
}

static int dd_imgui_active(lua_State *L) {
  // get io for mouse & keyboard management
  ImGuiIO &imgui_io = ImGui::GetIO();
  lua_pushboolean(L, imgui_io.WantCaptureMouse);
  return 1;
}

// ddLib library
static const struct luaL_Reg dd_lib[] = {
    {"print", ddprint},    {"ftime", ddftime},
    {"gtime", ddgtime},    {"hres_time", dd_high_res_time},
    {"get_hash", dd_hash}, {"mouse_over_UI", dd_imgui_active},
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

  // clear stack
  int top = lua_gettop(L);
  lua_pop(L, top);
}
