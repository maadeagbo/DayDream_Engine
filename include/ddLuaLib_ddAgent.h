#pragma once

#include "LuaHooks.h"
#include "ddAssetManager.h"
#include "ddImport_Model.h"
#include "ddSceneManager.h"
#include "ddTerminal.h"

const unsigned ddAgent_ptr_size = sizeof(ddAgent *);
const unsigned ddAnimState_ptr_size = sizeof(ddAnimState *);

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
      mass = (float)lua_tonumber(L, curr_arg);
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
        const unsigned num_mdl = (unsigned)ag->mesh.size();
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

/**
 * \brief Add skeleton to ddAgent
 */
int set_skeleton(lua_State *L) {
  ddAgent *ag = *(ddAgent **)lua_touserdata(L, 1);
  int top = lua_gettop(L);

  if (top != 2) {
    ddTerminal::post("[error]ddAgent::set_skeleton::Must provide skeleton id");
    lua_pushboolean(L, false);
    return 1;
  }

  // get skeleton id
  bool id_flag = lua_type(L, 2) == LUA_TSTRING;
  if (!id_flag) {
    ddTerminal::post(
        "[error]ddAgent::set_skeleton::Invalid 1st arg "
        "(id : string)");
    lua_pushboolean(L, false);
    return 1;
  }
  const char *id = lua_tostring(L, 2);

  // Check if skeleton exists
  ddSkeleton *sk = find_ddSkeleton(getCharHash(id));
  if (!sk) {
    ddTerminal::post("[error]ddAgent::set_skeleton::Skeleton not found");
    lua_pushboolean(L, false);
    return 1;
  }

  // based on skeleton, allocate ddAnimInfo buffers
  ag->anim.global_pose.resize(sk->bones.size());
  ag->anim.inv_bp.resize(sk->bones.size());
  ag->anim.local_pose.resize(sk->bones.size());
  ag->anim.sk_id = sk->id;
  ddTerminal::f_post("agent skeleton:: %llu :: %u(bones)",
                     (long long unsigned)sk->id,
                     (unsigned)ag->anim.global_pose.size());

  lua_pushboolean(L, true);
  return 1;
}

/**
 * \brief Add animation to ddAgent
 */
int add_animation(lua_State *L) {
  ddAgent *ag = *(ddAgent **)lua_touserdata(L, 1);
  int top = lua_gettop(L);

  // create userdata for anim state
  ddAnimState **a_state =
      (ddAnimState **)lua_newuserdata(L, ddAnimState_ptr_size);

  // log state id & animation clip id
  if (top != 3) {
    ddTerminal::post(
        "[error]ddAnimState::add_animation::Must provide new id & "
        "existing animation clip id");
    return 1;
  }

  int curr_arg = 2;
  bool id_flag = lua_type(L, curr_arg) == LUA_TSTRING;
  if (!id_flag) {
    ddTerminal::post(
        "[error]ddAnimState::add_animation::Invalid 1st arg (id : "
        "string)");
    return 1;
  }
  size_t id = getCharHash(lua_tostring(L, curr_arg));

  // get animation clip id
  curr_arg++;
  id_flag = lua_type(L, curr_arg) == LUA_TSTRING;
  if (!id_flag) {
    ddTerminal::post(
        "[error]ddAnimState::add_animation::Invalid 2nd arg (clip "
        "id : string)");
    return 1;
  }
  size_t anim_id = getCharHash(lua_tostring(L, curr_arg));

  // check if animation clip exists
  ddAnimClip *a_clip = find_ddAnimClip(anim_id);
  if (!a_clip) {
    ddTerminal::post("[error]ddAnimState::add_animation::Invalid clip id");
    return 1;
  }

  (*a_state) = nullptr;
  // check if state already exists in agent
  DD_FOREACH(ddAnimState, state, ag->anim.states) {
    if (state.ptr->id == id) (*a_state) = state.ptr;
  }

  // create new state
  if (!(*a_state)) {
    dd_array<ddAnimState> temp(ag->anim.states.size() + 1);
    temp = ag->anim.states;
    ag->anim.states = std::move(temp);
    (*a_state) = &ag->anim.states[ag->anim.states.size() - 1];
    (*a_state)->id = id;
    (*a_state)->clip_id = a_clip->id;
  }

  ddTerminal::f_post("agent animation:: %llu :: %llu(clip id)",
                     (long long unsigned)(*a_state)->id,
                     (long long unsigned)(*a_state)->clip_id);

  // set metatable
  luaL_getmetatable(L, ddAnimState_meta_name());
  lua_setmetatable(L, -2);

  return 1;
}

ddBodyFuncs::AABB oobb_to_aabb(dd_array<OOBoundingBox> &boxes) {
  ddBodyFuncs::AABB out;
  const glm::mat4 iden = glm::mat4();

  BoundingBox temp;
  // per oobb: update output aabb to set min & max
  DD_FOREACH(OOBoundingBox, box, boxes) {
    temp = box.ptr->get_bbox();
    out.update(temp.min);
    out.update(temp.max);
  }

  return out;
}

static int add_oobb(lua_State *L) {
  ddAgent *ag = *(ddAgent **)lua_touserdata(L, 1);
  dd_array<OOBoundingBox> boxes;

  // open and parse .ddx file
  int top = lua_gettop(L);
  if (top == 2) {
    const char *str = luaL_checkstring(L, 2);
    boxes = load_ddx(str);
  } else {
    ddTerminal::post("[error]ddAgent::add_oobb::invalid arguments provided");
    lua_pushboolean(L, false);
    return 1;
  }

  // set oobb array in agent ddBody
  if (boxes.isValid()) {
    ag->body.oobb_data.oobbs = std::move(boxes);

    // calculate current agent's bounding box min and max
    ddBodyFuncs::AABB new_aabb = oobb_to_aabb(ag->body.oobb_data.oobbs);

    // set
    ag->body.oobb_data.max_vert = new_aabb.max;
    ag->body.oobb_data.min_vert = new_aabb.min;
    ddBodyFuncs::update_aabb(&ag->body, new_aabb);
  }

  // return bool flag on sucess
  lua_pushboolean(L, true);
  return 1;
}

// ddAgent library
static const struct luaL_Reg agent_m2[] = {
    {"add_mesh", add_mesh},         {"__gc", ddAgent_gc},
    {"set_skeleton", set_skeleton}, {"add_animation", add_animation},
    {"add_oobb", add_oobb},         {NULL, NULL}};

static const struct luaL_Reg agent_lib[] = {{"new", new_ddAgent}, {NULL, NULL}};

int luaopen_ddAgent(lua_State *L) {
  // get and log functions in metatable
  log_meta_ddAgent(L);
  luaL_setfuncs(L, agent_m2, 0);

  // library functions
  luaL_newlib(L, agent_lib);
  return 1;
}
