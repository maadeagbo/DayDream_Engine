#include "ddLuaLib.h"
#include <string>
#include "ddAssetManager.h"
#include "ddSceneManager.h"
#include "ddTerminal.h"
#include "ddTimer.h"

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

// ddLib library
static const struct luaL_Reg dd_lib[] = {{"print", ddprint},
                                         {"ftime", ddftime},
                                         {"gtime", ddgtime},
                                         {"hres_time", dd_high_res_time},
                                         {NULL, NULL}};

int luaopen_ddLib(lua_State *L) {
  // library functions
  luaL_newlib(L, dd_lib);
  return 1;
}

//****************************************************************************
// Camera
//****************************************************************************

const unsigned ddcam_ptr_size = sizeof(ddCam *);

/** \brief "New" function for ddCam */
static int new_ddCam(lua_State *L) {
  // create userdata for instance
  ddCam **cam = (ddCam **)lua_newuserdata(L, ddcam_ptr_size);

  int num_args = lua_gettop(L);
  if (num_args != 3) {
    ddTerminal::post("[error]ddCam::Must provide id and parent agent id");
    return 0;
  }

  // get id
  int curr_arg = 1;
  bool id_flag = lua_type(L, curr_arg) == LUA_TSTRING;
  // luaL_argcheck(L, id_flag, 2, "ddCam::Must give camera id");
  if (!id_flag) {
    ddTerminal::post("[error]ddCam::Invalid 1st arg (id : string)");
    return 0;
  }
  const char *id = lua_tostring(L, curr_arg);

  // assign parent id
  curr_arg++;
  size_t p_id = 0;
  bool p_flag = lua_isinteger(L, curr_arg);
  if (!p_flag) {
    ddTerminal::post("[error]ddCam::Invalid 2nd arg (parent id : int)");
    return 0;
  } else {
    p_id = (size_t)lua_tointeger(L, curr_arg);
    ddAgent *ag = find_ddAgent(p_id);
    if (!ag) {
      ddTerminal::post("[error]ddCam::Agent id does not exist");
      return 0;
    }
  }

  (*cam) = spawn_ddCam(getCharHash(id));
  if (!(*cam)) {
    ddTerminal::post("[error]ddCam::Failed to allocate new camera");
    return 1;
  }
  // initialize
  glm::uvec2 scr_dim = ddSceneManager::get_screen_dimensions();
  (*cam)->width = scr_dim.x;
  (*cam)->height = scr_dim.y;
  (*cam)->fovh = glm::radians(60.f);
  (*cam)->n_plane = 0.1f;
  (*cam)->f_plane = 50.f;
  (*cam)->parent = p_id;

  // set metatable
  luaL_getmetatable(L, ddCam_meta_name());
  lua_setmetatable(L, -2);

  return 1; /* light user data on stack */
}

/** \brief Camera direction */
static int get_cam_dir(lua_State *L) {
  ddCam *cam = *(ddCam **)lua_touserdata(L, 1);
  if (cam) {
    ddAgent *ag = find_ddAgent(cam->parent);
    if (ag) {
      glm::vec3 d = ddSceneManager::cam_forward_dir(cam, &ag->body);
      push_vec3_to_lua(L, d.x, d.y, d.z);
			return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

/** \brief garbage collect ddCam */
static int ddCam_gc(lua_State *L) {
  // ddCam** cam = (ddCam **)luaL_checkudata(L, 1, ddCam_meta_name());
  ddCam *cam = *(ddCam **)lua_touserdata(L, 1);
  if (cam) {
    destroy_ddCam(cam->id);
  }

  return 0;
}

// ddCam library
static const struct luaL_Reg cam_m2[] = {
    {"__gc", ddCam_gc}, {"dir", get_cam_dir}, {NULL, NULL}};
static const struct luaL_Reg cam_lib[] = {{"new", new_ddCam}, {NULL, NULL}};

int luaopen_ddCam(lua_State *L) {
  // get and log functions in metatable
  log_meta_ddCam(L);
  luaL_setfuncs(L, cam_m2, 0);

  // library functions
  luaL_newlib(L, cam_lib);
  return 1;
}

//****************************************************************************
// Agent
//****************************************************************************

const unsigned ddAgent_ptr_size = sizeof(ddAgent *);

/** \brief "New" function for ddAgent */
static int new_ddAgent(lua_State *L) {
  // create userdata for instance
  ddAgent **ag = (ddAgent **)lua_newuserdata(L, ddAgent_ptr_size);

  int num_args = lua_gettop(L);
  if (num_args == 0) {
    ddTerminal::post("[error]ddAgent::Must provide id at initialization");
    return 0;
  }

  // get id
  int curr_arg = 1;
  bool id_flag = lua_type(L, curr_arg) == LUA_TSTRING;
  // luaL_argcheck(L, id_flag, 2, "ddCam::Must give camera id");
  if (!id_flag) {
    ddTerminal::post("[error]ddAgent::Invalid 1st arg (id : string)");
    return 0;
  }
  const char *id = lua_tostring(L, curr_arg);

  // get mass
  curr_arg++;
  float mass = 0.f;
  if (num_args > curr_arg) {
    bool mass_flag = lua_type(L, curr_arg) == LUA_TNUMBER;
    if (!mass_flag) {
      ddTerminal::post("[error]ddAgent::Ignoring 2nd arg (mass : float)");
    } else {
      mass = lua_tonumber(L, curr_arg);
    }
  }

  // get type
  curr_arg++;
  RBType type = RBType::BOX;
  if (num_args > curr_arg) {
    bool type_flag = lua_type(L, curr_arg) == LUA_TSTRING;
    if (!type_flag) {
      ddTerminal::post("[error]ddAgent::Ignoring 3rd arg (AABB type : string)");
    } else {
      cbuff<64> t = lua_tostring(L, curr_arg);
      if (t.compare("box") == 0)
        type = RBType::BOX;
      else if (t.compare("sphere") == 0)
        type = RBType::SPHERE;
      else if (t.compare("free") == 0)
        type = RBType::FREE_FORM;
      else if (t.compare("kinematic") == 0)
        type = RBType::KIN;
    }
  }

  // get mesh
  curr_arg++;
  ddModelData *mdata = nullptr;
  if (num_args > curr_arg) {
    bool mdl_flag = lua_isinteger(L, curr_arg);
    if (!mdl_flag) {
      ddTerminal::post("[error]ddAgent::Ignoring 4th arg (model id : int)");
    } else {
      mdata = find_ddModelData((size_t)lua_tointeger(L, curr_arg));
      if (!mdata) {
        ddTerminal::post("[error]ddAgent::Invalid model id. Mesh not added");
      }
    }
  }

  (*ag) = spawn_ddAgent(getCharHash(id));
  if (!(*ag)) {
    ddTerminal::post("[error]ddAgent::Failed to allocate new agent");
    return 1;
  }

  // initialize
  if (mdata) {
    // modify ModelIDs struct
    (*ag)->mesh.resize(1);
    (*ag)->mesh[0].model = mdata->id;
    // initialize material buffer
    (*ag)->mesh[0].material.resize(mdata->mesh_info.size());
    DD_FOREACH(DDM_Data, data, mdata->mesh_info) {
      (*ag)->mesh[0].material[data.i] = data.ptr->mat_id;
    }
  }
  // add ddBody to agent and then agent to world
  ddAssets::add_body((*ag), mdata, glm::vec3(), glm::vec3(), mass, type);

  // set metatable
  luaL_getmetatable(L, ddAgent_meta_name());
  lua_setmetatable(L, -2);

  return 1;
}

/** \brief garbage collect ddAgent */
static int ddAgent_gc(lua_State *L) {
  ddAgent *ag = *(ddAgent **)lua_touserdata(L, 1);
  if (ag && ag->body.bt_bod) {
    ddAssets::remove_rigid_body(ag);
    destroy_ddAgent(ag->id);
  }

  return 0;
}

// ddAgent library
static const struct luaL_Reg agent_m2[] = {{"__gc", ddAgent_gc}, {NULL, NULL}};
static const struct luaL_Reg agent_lib[] = {{"new", new_ddAgent}, {NULL, NULL}};

int luaopen_ddAgent(lua_State *L) {
  // get and log functions in metatable
  log_meta_ddAgent(L);
  luaL_setfuncs(L, agent_m2, 0);

  // library functions
  luaL_newlib(L, agent_lib);
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

  // clear stack
  int top = lua_gettop(L);
  lua_pop(L, top);
}
