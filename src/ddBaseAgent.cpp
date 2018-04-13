#include "ddBaseAgent.h"
#include <string>
#include "ddTerminal.h"

ddAgent::ddAgent() {}

ddAgent::~ddAgent() {}

namespace {
DD_FuncBuff fb;
}

namespace ddBodyFuncs {

glm::vec3 pos(const ddBody* bod) {
  // local position
  btVector3 center = bod->bt_bod->getCenterOfMassPosition();
  return glm::vec3(center.x(), center.y(), center.z());
}

glm::vec3 pos_ws(const ddBody* bod) {
  // world position
  btTransform tr = bod->bt_bod->getWorldTransform();
  btVector3 ws = tr.getOrigin();
  return glm::vec3(ws.x(), ws.y(), ws.z());
}

glm::quat rot(const ddBody* bod) {
  // local transform
  glm::mat4 _r;
  bod->bt_bod->getCenterOfMassTransform().getBasis().getOpenGLSubMatrix(
      &_r[0][0]);
  return glm::quat_cast(_r);
}

glm::quat rot_ws(const ddBody* bod) {
  // world transform
  glm::mat4 _r;
  bod->bt_bod->getWorldTransform().getBasis().getOpenGLSubMatrix(&_r[0][0]);
  return glm::quat_cast(_r);
}

glm::vec3 forward_dir(const ddBody* bod) {
  glm::quat q = ddBodyFuncs::rot_ws(bod);
  glm::vec4 _f =
      q * glm::vec4(world_front.x, world_front.y, world_front.z, 1.f);
  return glm::normalize(glm::vec3(_f));
}

void update_velocity(ddBody* bod, const glm::vec3& vel) {
  // bod->bt_bod->setLinearVelocity(btVector3(vel.x, vel.y, vel.z));

  // btMatrix3x3& boxRot = bod->bt_bod->getWorldTransform().getBasis();
  // btVector3 correctedForce = boxRot * btVector3(vel.x, vel.y, vel.z);

  bod->bt_bod->activate(true);
  bod->bt_bod->setLinearVelocity(btVector3(vel.x, vel.y, vel.z));
}

void update_pos(ddBody* bod, const glm::vec3& pos) {
  // local rotation
  const btQuaternion q1 = bod->bt_bod->getCenterOfMassTransform().getRotation();

  // set tranform
  btTransform tr;
  tr.setOrigin(btVector3(pos.x, pos.y, pos.z));
  tr.setRotation(q1);
  bod->bt_bod->setWorldTransform(tr);
  bod->bt_bod->getMotionState()->setWorldTransform(tr);

  // clear forces to remove "portal-like" motion
  // bod->bt_bod->clearForces();
  // bod->bt_bod->setLinearVelocity(btVector3(0, 0, 0));
  // bod->bt_bod->setAngularVelocity(btVector3(0, 0, 0));
}

void rotate(ddBody* bod, const float yaw, const float pitch, const float roll) {
  btTransform tr;
  // local rotation
  // const btQuaternion q1 =
  // bod->bt_bod->getCenterOfMassTransform().getRotation();

  // set new transform
  tr.setIdentity();
  const glm::vec3 p1 = ddBodyFuncs::pos_ws(bod);
  tr.setOrigin(btVector3(p1.x, p1.y, p1.z));

  // new rotation
  btQuaternion q2;
  q2.setEuler(yaw, pitch, roll);
  tr.setRotation(q2);

  bod->bt_bod->setWorldTransform(tr);
  // bod->bt_bod->getMotionState()->setWorldTransform(tr);

  // bod->bt_bod->applyTorque(btVector3(torque.x, torque.y, torque.z));
  // bod->bt_bod->setAngularVelocity(btVector3(torque.x, torque.y, torque.z));
}

void update_scale(ddBody* bod, const glm::vec3& _scale) {
  bod->scale = _scale;

  glm::vec3 s_val =
      bod->oobb_data.oobbs.isValid() ? _scale * bod->oobbs_scale : _scale;
  // set scale in collision shape for physics
  bod->bt_bod->getCollisionShape()->setLocalScaling(
      btVector3((btScalar)s_val.x, (btScalar)s_val.y, (btScalar)s_val.z));
  // btCollisionShape::setLocalScaling()
  // btCollisionWorld::updateSingleAABB( rigidbody )
}

glm::mat4 get_model_mat(ddBody* bod) {
  const glm::vec3 sc = bod->scale;
  glm::mat4 _s = glm::scale(glm::mat4(), sc);
  glm::mat4 tr;
  bod->bt_bod->getWorldTransform().getOpenGLMatrix(&tr[0][0]);

  return tr * _s;
}

AABB get_aabb(ddBody* bod) {
  AABB bbox;

  btVector3 min, max;
  bod->bt_bod->getAabb(min, max);
  bbox.min = glm::vec3(min.x(), min.y(), min.z());
  bbox.max = glm::vec3(max.x(), max.y(), max.z());

  return bbox;
}

void update_aabb(ddBody* bod, AABB& bbox) {
  // default box is a 1x1x1 cube centered at (0,0,0)
  // This method relates the position & scale to that reference box, then sets

  // get the new scale in the x,y,z dimension
  bod->oobbs_scale = glm::abs(bbox.max - bbox.min);

  // get the new center position
  glm::vec3 center = (bod->oobbs_scale * 0.5f) + bbox.min;
  center += bod->oobb_offset;

  // get compound shape and rotation
  btCompoundShape* _shape =
      static_cast<btCompoundShape*>(bod->bt_bod->getCollisionShape());
  btQuaternion q = bod->bt_bod->getWorldTransform().getRotation();

  // un-transform bbox
  _shape->updateChildTransform(
      0, btTransform(btQuaternion(0, 0, 0), btVector3(0, 0, 0)));

  // set scale
  update_scale(bod, bod->scale);

  // set final rotation & translation
  _shape->updateChildTransform(
      0, btTransform(q, btVector3(center.x, center.y, center.z)));
}

}  // namespace ddBodyFuncs

//******************************************************************************
//******************************************************************************

#define DDANIMSTATE_META_NAME "LuaClass.ddAnimState"
#define check_ddAnimState(L) \
  (ddAnimState**)luaL_checkudata(L, 1, DDANIMSTATE_META_NAME)

const char* ddAnimState_meta_name() { return DDANIMSTATE_META_NAME; }

static int set_val(lua_State* L);
static int get_val(lua_State* L);
static int anim_to_string(lua_State* L);

static const struct luaL_Reg animstate_methods[] = {
    {"__index", get_val},
    {"__newindex", set_val},
    {"__tostring", anim_to_string},
    {NULL, NULL}};

void log_meta_ddAnimState(lua_State* L) {
  luaL_newmetatable(L, DDANIMSTATE_META_NAME);  // create meta table
  luaL_setfuncs(L, animstate_methods, 0);       /* register metamethods */
}

int get_val(lua_State* L) {
  ddAnimState* a_state = *check_ddAnimState(L);

  // match input enum to ddAnimState variable
  int idx = (int)luaL_checkinteger(L, 2);
  switch (idx) {
    case 1:
      // weight
      lua_pushnumber(L, a_state->weight);
      break;
    case 2:
      // local time
      lua_pushnumber(L, a_state->local_time);
      break;
    case 3:
      // play speed
      lua_pushnumber(L, a_state->play_back);
      break;
    case 4:
      // interpolate
      lua_pushnumber(L, a_state->interpolate);
      break;
    case 5:
      // active
      lua_pushboolean(L, a_state->active);
      break;
    case 6:
      // loop
      lua_pushboolean(L, a_state->flag_loop);
      break;
    default:
      ddTerminal::f_post("[error]ddAnimState::invalid indexing enum (%d)", idx);
      // couldn't find indexed object/ignoring
      lua_pushnil(L);
      break;
  }

  return 1;
}

int set_val(lua_State* L) {
  ddAnimState* a_state = *check_ddAnimState(L);

  // match input enum to ddAnimState variable
  int idx = (int)luaL_checkinteger(L, 2);
  switch (idx) {
    case 1:
      // weight
      a_state->weight = (float)luaL_checknumber(L, 3);
      break;
    case 2:
      // local time
      a_state->local_time = (float)luaL_checknumber(L, 3);
      break;
    case 3:
      // play speed
      a_state->play_back = (float)luaL_checknumber(L, 3);
      break;
    case 4:
      // interpolate
      luaL_checktype(L, 3, LUA_TBOOLEAN);
      a_state->interpolate = (bool)lua_toboolean(L, 3);
      break;
    case 5:
      // active
      luaL_checktype(L, 3, LUA_TBOOLEAN);
      a_state->active = (bool)lua_toboolean(L, 3);
      break;
    case 6:
      // loop
      luaL_checktype(L, 3, LUA_TBOOLEAN);
      a_state->flag_loop = (bool)lua_toboolean(L, 3);
      break;
    default:
      ddTerminal::f_post("[error]ddAnimState::invalid indexing enum (%d)", idx);
      break;
  }

  // couldn't find indexed object/ignoring
  return 0;
}

static int anim_to_string(lua_State* L) {
  // ddAnimState* a_state = *check_ddAnimState(L);

  // print ddAnimState information

  return 0;
}

//******************************************************************************
//******************************************************************************

#define DDAGENT_META_NAME "LuaClass.ddAgent"
#define check_ddAgent(L) (ddAgent**)luaL_checkudata(L, 1, DDAGENT_META_NAME)

const char* ddAgent_meta_name() { return DDAGENT_META_NAME; }

static int get_id(lua_State* L);
static int get_pos(lua_State* L);
static int get_rot(lua_State* L);
static int get_scale(lua_State* L);
static int get_vel(lua_State* L);
static int get_forward(lua_State* L);
static int get_mat_count(lua_State* L);
static int get_mat_at_idx(lua_State* L);

static int set_pos(lua_State* L);
static int set_rot(lua_State* L);
static int set_scale(lua_State* L);
static int set_vel(lua_State* L);
static int set_friction(lua_State* L);
static int set_damping(lua_State* L);
static int set_mat_at_idx(lua_State* L);

static int to_string(lua_State* L);

// method list
static const struct luaL_Reg agent_methods[] = {
    {"id", get_id},
    {"pos", get_pos},
    {"set_pos", set_pos},
    {"eulerPYR", get_rot},
    {"set_eulerPYR", set_rot},
    {"scale", get_scale},
    {"set_scale", set_scale},
    {"vel", get_vel},
    {"set_vel", set_vel},
    {"set_friction", set_friction},
    {"set_damping", set_damping},
    {"forward_dir", get_forward},
    {"mat_count", get_mat_count},
    {"mat_at_idx", get_mat_at_idx},
    {"set_mat_at_idx", set_mat_at_idx},
    {"__tostring", to_string},
    {NULL, NULL}};

void log_meta_ddAgent(lua_State* L) {
  log_meta_ddAnimState(L);  // ddAnimState

  luaL_newmetatable(L, DDAGENT_META_NAME);  // create meta table
  lua_pushvalue(L, -1);                     /* duplicate the metatable */
  lua_setfield(L, -2, "__index");           /* mt.__index = mt */
  luaL_setfuncs(L, agent_methods, 0);       /* register metamethods */
}

static int get_id(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);
  lua_pushinteger(L, ag->id);
  return 1;
}

int get_pos(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);

  // return world space position
  glm::vec3 wp = ddBodyFuncs::pos_ws(&ag->body);
  push_vec3_to_lua(L, wp.x, wp.y, wp.z);

  return 1;
}

int get_rot(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);

  // return world space rotation
  glm::vec3 temp = glm::eulerAngles(ddBodyFuncs::rot_ws(&ag->body));
  glm::vec3 rot = glm::degrees(temp);
  push_vec3_to_lua(L, rot.x, rot.y, rot.z);

  return 1;
}

int get_scale(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);

  // return world space scale
  glm::vec3 sc = ag->body.scale;
  push_vec3_to_lua(L, sc.x, sc.y, sc.z);

  return 1;
}

int get_vel(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);

  // return velocity
  btVector3 vel = ag->body.bt_bod->getLinearVelocity();
  push_vec3_to_lua(L, vel.x(), vel.y(), vel.z());

  return 1;
}

int get_forward(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);

  // return forward direction
  glm::vec3 fwd = ddBodyFuncs::forward_dir(&ag->body);
  push_vec3_to_lua(L, fwd.x, fwd.y, fwd.z);

  return 1;
}

int get_mat_count(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);
  int top = lua_gettop(L);
  size_t lod_lvl = 0;

  if (top >= 2) {
    if (lua_isinteger(L, 2)) {
      lod_lvl = (size_t)lua_tointeger(L, 2);  // get lod level
    } else {
      ddTerminal::post(
          "[error]ddAgent::mat_count::Invalid 2nd arg (LOD level : int)");
      lua_pushinteger(L, -1);
      return 1;
    }
  }

  if (lod_lvl >= ag->mesh.size()) {  // check if lod level is out of bounds
    ddTerminal::post(
        "[error]ddAgent::mat_count::Invalid 2nd arg (LOD level : int)");
    lua_pushinteger(L, -1);
    return 1;
  }

  lua_pushinteger(L, ag->mesh[lod_lvl].material.size());
  return 1;
}

static bool check_mat_args(lua_State* L, ddAgent* ag, size_t& lod_arg,
                           size_t& mat_arg, const char* func_id) {
  int top = lua_gettop(L);
  if (top >= 3) {
    if (lua_isinteger(L, 2)) {
      lod_arg = (size_t)lua_tointeger(L, 2);  // get lod level
    } else {
      ddTerminal::f_post(
          "[error]ddAgent::%s::Invalid 2nd arg (LOD level : int)", func_id);
      return false;
    }

    if (lua_isinteger(L, 3)) {
      mat_arg = (size_t)lua_tointeger(L, 3);  // get material index
    } else {
      ddTerminal::f_post(
          "[error]ddAgent::%s::Invalid 3rd arg (material index : int)",
          func_id);
      return false;
    }
  } else {  // not enough arguments provided
    ddTerminal::f_post(
        "[error]ddAgent::%s::Must provide 2 args (LOD level, material index)",
        func_id);
    return false;
  }

  if (lod_arg >= ag->mesh.size()) {  // check if lod level is out of bounds
    ddTerminal::f_post(
        "[error]ddAgent::%s::Out-of-bounds 2nd arg (LOD level : int)", func_id);
    return false;
  }
  // check if material index is out of bounds
  if (mat_arg >= ag->mesh[lod_arg].material.size()) {
    ddTerminal::f_post(
        "[error]ddAgent::%s::Out-of-bounds 3rd arg (material index : int)",
        func_id);
    return false;
  }

  return true;
}

int get_mat_at_idx(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);
  size_t lod_lvl = 0;
  size_t mat_idx = 0;

  bool valid_idxs = check_mat_args(L, ag, lod_lvl, mat_idx, "mat_at_idx");
  if (valid_idxs) {
    lua_pushinteger(L, ag->mesh[lod_lvl].material[mat_idx]);
  } else {
    lua_pushnil(L);
  }

  return 1;
}

static void get_v3_lua(lua_State* L, glm::vec3& in) {
  int top = lua_gettop(L);
  for (unsigned i = 2; i <= (unsigned)top; i++) {
    if (lua_isnumber(L, i)) {
      switch (i) {
        case 2:
          in.x = (float)lua_tonumber(L, i);
          break;
        case 3:
          in.y = (float)lua_tonumber(L, i);
          break;
        case 4:
          in.z = (float)lua_tonumber(L, i);
          break;
        default:
          break;
      }
    }
  }
}

int set_pos(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);
  glm::vec3 pos = ddBodyFuncs::pos_ws(&ag->body);

  get_v3_lua(L, pos);
  ddBodyFuncs::update_pos(&ag->body, pos);

  return 0;
}

int set_rot(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);
  glm::vec3 temp = glm::eulerAngles(ddBodyFuncs::rot_ws(&ag->body));
  glm::vec3 rot = glm::degrees(temp);

  get_v3_lua(L, rot);
  rot = glm::radians(rot);
  ddBodyFuncs::rotate(&ag->body, rot.y, rot.x, rot.z);

  return 0;
}

int set_scale(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);
  glm::vec3 sc = ag->body.scale;

  get_v3_lua(L, sc);
  ddBodyFuncs::update_scale(&ag->body, sc);

  return 0;
}

int set_vel(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);
  btVector3 vel = ag->body.bt_bod->getLinearVelocity();
  glm::vec3 vel3(vel.x(), vel.y(), vel.z());

  get_v3_lua(L, vel3);
  ddBodyFuncs::update_velocity(&ag->body, vel3);
  return 0;
}

int set_friction(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);

  int top = lua_gettop(L);
  if (top == 2) {
    float fr = (float)lua_tonumber(L, 2);
    if (fr >= 0.f && fr <= 1.f) {
      ag->body.bt_bod->setFriction(fr);
    }
  }

  return 0;
}

int set_damping(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);
  float l_damp = (float)ag->body.bt_bod->getLinearDamping();
  float a_damp = (float)ag->body.bt_bod->getAngularDamping();
  glm::vec3 damp(l_damp, a_damp, 0.f);

  get_v3_lua(L, damp);
  ag->body.bt_bod->setDamping(damp.x, damp.y);

  return 0;
}

int set_mat_at_idx(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);
  size_t lod_lvl = 0;
  size_t mat_idx = 0;

  bool valid_idxs = check_mat_args(L, ag, lod_lvl, mat_idx, "set_mat_at_idx");
  if (valid_idxs) {
    // get 4th argument (material ID to assign)
    int top = lua_gettop(L);
    if (top >= 4 && lua_isinteger(L, 4)) {
      size_t id = (size_t)lua_tointeger(L, 4);
      ag->mesh[lod_lvl].material[mat_idx] = id;
    }
  }
  return 0;
}

static int to_string(lua_State* L) {
  ddAgent* ag = *check_ddAgent(L);
  std::string buff;

  cbuff<128> out;
  out.format("ddAgent(%llu):", (unsigned long long)ag->id);
  buff += out.str();

  out.format("\n  Mesh: %d", (int)ag->mesh.size());
  buff += out.str();

  DD_FOREACH(ModelIDs, mdl, ag->mesh) {
    out.format("\n    %u:: near %.3f, far %.3f, model %llu", mdl.i,
               mdl.ptr->_near, mdl.ptr->_far,
               (unsigned long long)mdl.ptr->model);
    buff += out.str();

    out.format("\n    Materials: %d", (int)mdl.ptr->material.size());
    buff += out.str();
    DD_FOREACH(size_t, mat_id, mdl.ptr->material) {
      out.format("\n      %u:: id %llu", mat_id.i,
                 (unsigned long long)(*mat_id.ptr));
      buff += out.str();
    }
  }

  lua_pushstring(L, buff.c_str());
  return 1;
}
