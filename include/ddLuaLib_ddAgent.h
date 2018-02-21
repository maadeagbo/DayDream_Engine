#pragma once

#include "LuaHooks.h"
#include "ddAssetManager.h"
#include "ddSceneManager.h"
#include "ddTerminal.h"

const unsigned ddAgent_ptr_size = sizeof(ddAgent *);

/**
 * \brief "New" function for ddAgent
 */
static int new_ddAgent(lua_State *L) {
  // create userdata for instance
  ddAgent **ag = (ddAgent **)lua_newuserdata(L, ddAgent_ptr_size);

  int num_args = lua_gettop(L);
  if (num_args == 0) {
    ddTerminal::post("[error]ddAgent::Must provide id at initialization");
    return 1;
  }

  // get id
  int curr_arg = 1;
  bool id_flag = lua_type(L, curr_arg) == LUA_TSTRING;
  // luaL_argcheck(L, id_flag, 2, "ddCam::Must give camera id");
  if (!id_flag) {
    ddTerminal::post("[error]ddAgent::Invalid 1st arg (id : string)");
    return 1;
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

  // check if already exists
  (*ag) = find_ddAgent(getCharHash(id));
  if (*ag) {
    ddTerminal::post("[error]ddAgent::Returning already allocated agent");
    return 1;
  }

  // create new
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

/**
 * \brief garbage collect ddAgent
 */
static int ddAgent_gc(lua_State *L) {  // still need to implement proper delete
  ddAgent *ag = *(ddAgent **)lua_touserdata(L, 1);
  if (ag && ag->body.bt_bod) {
    ddAssets::remove_rigid_body(ag);
    destroy_ddAgent(ag->id);
  }

  return 0;
}

/**
 * \brief Add mesh to ddAgent
 */
static int add_mesh(lua_State *L) {
  ddAgent *ag = *(ddAgent **)lua_touserdata(L, 1);
  if (ag && ag->body.bt_bod) {
    int top = lua_gettop(L);

    if (top >= 2 && lua_isinteger(L, 2)) {
      size_t mdl_id = (size_t)lua_tointeger(L, 2);
      float near_p = 0.1f;
      float far_p = 100.f;

      // near plane
      if (top >= 3 && lua_isnumber(L, 3)) {
        near_p = (float)lua_tonumber(L, 3);
      }
      // far plane
      if (top >= 4 && lua_isnumber(L, 4)) {
        far_p = (float)lua_tonumber(L, 4);
      }

      ddModelData *mdata = find_ddModelData(mdl_id);
      if (mdata) {
        // set up agent data
        const unsigned num_mdl = ag->mesh.size();
        if (num_mdl == 0) {
          // initialize model buffer
          ag->mesh.resize(1);
          ag->mesh[0].model = mdata->id;
          ag->mesh[0]._near = near_p;
          ag->mesh[0]._far = far_p;
          // initialize material buffer
          ag->mesh[0].material.resize(mdata->mesh_info.size());
          DD_FOREACH(DDM_Data, data, mdata->mesh_info) {
            ag->mesh[0].material[data.i] = data.ptr->mat_id;
          }
        } else {
          // allocate new space in model buffer
          const int idx = num_mdl;
          dd_array<ModelIDs> temp(num_mdl + 1);
          temp = ag->mesh;
          ag->mesh = std::move(temp);

          ag->mesh[idx].model = mdata->id;
          ag->mesh[idx]._near = near_p;
          ag->mesh[idx]._far = far_p;
          ag->mesh[idx].material.resize(mdata->mesh_info.size());
          DD_FOREACH(DDM_Data, data, mdata->mesh_info) {
            ag->mesh[idx].material[data.i] = data.ptr->mat_id;
          }
        }
        // skip loading if using opengl api and on separate thread
        bool skip = DD_GRAPHICS_API == 0 && ddAssets::load_screen_check();
        if (!skip) {
          // load to gpu
          ddAssets::load_agent_to_gpu(ag);
        }

        lua_pushboolean(L, true);
        return 1;
      }
    }
  }

  lua_pushboolean(L, false);
  return 1;
}

// ddAgent library
static const struct luaL_Reg agent_m2[] = {
    {"add_mesh", add_mesh}, {"__gc", ddAgent_gc}, {NULL, NULL}};
static const struct luaL_Reg agent_lib[] = {{"new", new_ddAgent}, {NULL, NULL}};

int luaopen_ddAgent(lua_State *L) {
  // get and log functions in metatable
  log_meta_ddAgent(L);
  luaL_setfuncs(L, agent_m2, 0);

  // library functions
  luaL_newlib(L, agent_lib);
  return 1;
}
