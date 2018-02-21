#include "ddAssetManager.h"
#include "ddFileIO.h"
#include "ddImport_Model.h"
#include "ddTerminal.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace {

// function callback buffer
DD_FuncBuff fb;
// containers only exist in this translation unit
btDiscreteDynamicsWorld *p_world = nullptr;

// default parameters for object initialization
unsigned native_scr_width = 0, native_scr_height = 0;

// flag to check if game is in load screen
bool load_screen_flag = false;
}  // namespace

/// \brief Add agent to physics world
/// \param agent to add rigid body
/// \param mesh data containing bounding box information
/// \return True if rigid body is successfully added
bool add_rigid_body(ddAgent *agent, ddModelData *mdata, glm::vec3 pos,
                    glm::vec3 rot, const float mass = 0.f,
                    RBType rb_type = RBType::BOX);
/**
 * \brief Create ddAgent from lua scripts
 * \param L lua state
 * \return Number of returned values to lua
 */
int dd_assets_create_agent(lua_State *L);
/**
 * \brief Create ddCam from lua scripts
 * \param L lua state
 * \return Number of returned values to lua
 */
int dd_assets_create_cam(lua_State *L);
/**
 * \brief Create ddLBulb from lua scripts
 * \param L lua state
 * \return Number of returned values to lua
 */
int dd_assets_create_light(lua_State *L);
/**
 * \brief Create ddTex2D from lua scripts
 * \param L lua state
 * \return Number of returned values to lua
 */
int dd_assets_create_texture(lua_State *L);

// agent monipulation
int get_agent_pos_ls(lua_State *L);
int get_agent_pos_ws(lua_State *L);
int get_agent_forward_dir(lua_State *L);
int set_agent_pos(lua_State *L);
int set_agent_scale(lua_State *L);
int set_agent_vel(lua_State *L);
int rotate_agent(lua_State *L);
int get_agent_vel(lua_State *L);
int get_cam_dir(lua_State *L);
int get_agent_ang_vel(lua_State *L);
int set_agent_friction(lua_State *L);
int set_agent_damping(lua_State *L);

// camera manipulation
int rotate_camera(lua_State *L);
/**
 * \brief Add ghost agent to physics world for parenting/scene graph
 */
btRigidBody &create_ghost(btTransform _transform);
/**
 * \brief Removes rigidbody from physics world
 * \param agent contains rigid body
 */
void delete_rigid_body(ddAgent *agent);

// source <Muhammad Mobeen Movania::OpenGL Development Cookbook>
/// \brief Invert image along Y-axis
/// \param image Pointer to image on RAM
/// \param width
/// \param height
/// \param channels
void flip_image(unsigned char *image, const int width, const int height,
                const int channels);

void ddAssets::initialize(btDiscreteDynamicsWorld *physics_world) {
  p_world = physics_world;

  // initialize asset buffers
  init_assets();

  // default assets
  obj_mat default_mat;
  default_mat.diff_raw = glm::vec4(0.5, 0.5, 0.5f, 1.f);
  create_material(default_mat);
}

void ddAssets::set_load_screen_flag(const bool flag) {
  load_screen_flag = flag;
}

void ddAssets::cleanup_assets() {
  // clean up bullet physics bodies
  dd_array<ddAgent *> ag_array = get_all_ddAgent();
  DD_FOREACH(ddAgent *, ag_id, ag_array) {
    ddAgent *ag = *ag_id.ptr;
    delete_rigid_body(ag);
  }

  // close asset buffers
  cleanup_all_assets();
}

void ddAssets::default_params(const unsigned scr_width,
                              const unsigned scr_height) {
  native_scr_width = scr_width;
  native_scr_height = scr_height;
}

void ddAssets::log_lua_func(lua_State *L) {
  // mesh creation
  add_func_to_scripts(L, dd_assets_create_mesh, "dd_load_ddm");
  // camera creation
  add_func_to_scripts(L, dd_assets_create_cam, "dd_create_camera");
  // light creation
  add_func_to_scripts(L, dd_assets_create_light, "dd_create_light");
  // agent creation
  add_func_to_scripts(L, dd_assets_create_agent, "dd_create_agent");
  // texture creation
  add_func_to_scripts(L, dd_assets_create_texture, "dd_create_texture");

  // get agent information
  add_func_to_scripts(L, get_agent_pos_ws, "ddAgent_world_pos");
  add_func_to_scripts(L, get_agent_pos_ls, "ddAgent_local_pos");
  add_func_to_scripts(L, get_agent_forward_dir, "ddAgent_get_forward");
  add_func_to_scripts(L, get_agent_vel, "ddAgent_get_velocity");
  add_func_to_scripts(L, get_agent_ang_vel, "ddAgent_get_ang_velocity");
  // manipulate agent information
  add_func_to_scripts(L, set_agent_pos, "ddAgent_set_position");
  add_func_to_scripts(L, set_agent_vel, "ddAgent_set_velocity");
  add_func_to_scripts(L, rotate_agent, "ddAgent_set_rotation");
  add_func_to_scripts(L, set_agent_scale, "ddAgent_set_scale");
  add_func_to_scripts(L, set_agent_friction, "ddAgent_set_friction");
  add_func_to_scripts(L, set_agent_damping, "ddAgent_set_damping");
  // manipulate camera
  add_func_to_scripts(L, rotate_camera, "ddCam_rotate");
}

void ddAssets::load_to_gpu() {
  // load textures (skip load_screen id)
  size_t load_screen_tex = getCharHash("load_screen");

  dd_array<ddTex2D *> t_array = get_all_ddTex2D();
  DD_FOREACH(ddTex2D *, t_id, t_array) {
    ddTex2D *tex = *t_id.ptr;

    if (tex->id == load_screen_tex) continue;

    bool success = ddGPUFrontEnd::generate_texture2D_RGBA8_LR(tex->image_info);
    POW2_VERIFY_MSG(success == true, "Texture not generated: %s",
                    tex->image_info.path[0].str());
    // cleanup images on ram
    tex->image_info.image_data[0].resize(0);
  }

  // load meshes
  dd_array<ddModelData *> md_array = get_all_ddModelData();
  DD_FOREACH(ddModelData *, mh_id, md_array) {
    ddModelData *mdl = *mh_id.ptr;
    load_model_to_gpu(mdl);
  }

  // load agents
  dd_array<ddAgent *> ag_array = get_all_ddAgent();
  DD_FOREACH(ddAgent *, ag_id, ag_array) {
    ddAgent *ag = *ag_id.ptr;
    load_agent_to_gpu(ag);
  }
}

void ddAssets::load_agent_to_gpu(ddAgent *ag) {
  // create instance buffer
  bool success = ddGPUFrontEnd::load_instance_data(ag->inst.inst_buff,
                                                   ag->inst.m4x4.size());
  POW2_VERIFY_MSG(success == true, "Instance data not loaded to GPU", 0);

  // create & bind vao
  DD_FOREACH(ModelIDs, mdl_id, ag->mesh) {
    ddModelData *mdl = find_ddModelData(mdl_id.ptr->model);
    POW2_VERIFY_MSG(success == true, "Mesh data not found", 0);

    mdl_id.ptr->vao_handles.resize(mdl->buffers.size());
    DD_FOREACH(ddVAOData *, vao, mdl_id.ptr->vao_handles) {
      success = ddGPUFrontEnd::create_vao(*vao.ptr);
      POW2_VERIFY_MSG(success == true, "VAO data not generated", 0);
      success = ddGPUFrontEnd::bind_object(*vao.ptr, ag->inst.inst_buff,
                                           mdl->buffers[vao.i]);
      POW2_VERIFY_MSG(success == true, "Object not bound to VAO", 0);
    }
  }
}

void ddAssets::load_model_to_gpu(ddModelData *mdl) {
  mdl->buffers.resize(mdl->mesh_info.size());

  DD_FOREACH(DDM_Data, md, mdl->mesh_info) {
    // create mesh data on gpu
    mdl->buffers[md.i] = nullptr;
    bool success = ddGPUFrontEnd::load_buffer_data(mdl->buffers[md.i], md.ptr);
    POW2_VERIFY_MSG(success == true, "Mesh data not loaded to GPU", 0);

    // cleanup on ram?
    md.ptr->data.resize(0);
  }
}

void ddAssets::remove_rigid_body(ddAgent *ag) { delete_rigid_body(ag); }

bool ddAssets::add_body(ddAgent *agent, ddModelData *mdata, glm::vec3 pos,
                        glm::vec3 rot, const float mass, RBType rb_type) {
  if (!agent) return false;

  // set up bounding box
  glm::vec3 bb_max = glm::vec3(0.5f, 0.5f, 0.5f);
  glm::vec3 bb_min = glm::vec3(-.5f, -0.5f, -0.5f);
  if (mdata) {
    bb_max = mdata->mesh_info[0].bb_max * agent->body.scale;
    bb_min = mdata->mesh_info[0].bb_min * agent->body.scale;
  }
  float h_width = (bb_max.x - bb_min.x) * 0.5f;
  float h_height = (bb_max.y - bb_min.y) * 0.5f;
  float h_depth = (bb_max.z - bb_min.z) * 0.5f;

  btCollisionShape *bt_shape = nullptr;
  if (rb_type == RBType::SPHERE) {
    float diameter = glm::length(glm::vec3(h_width, h_height, h_depth));
    bt_shape = new btSphereShape(diameter * 0.5f);
  } else {
    bt_shape = new btBoxShape(
        btVector3(btScalar(h_width), btScalar(h_height), btScalar(h_depth)));
  }

  // set up rigid body constructor
  btTransform transform;
  transform.setIdentity();
  transform.setOrigin(btVector3(pos.x, pos.y, pos.z));
  btQuaternion _q;
  _q.setEuler(rot.y, rot.x, rot.z);
  transform.setRotation(_q);

  // rigidbody is dynamic if and only if mass is non zero, otherwise static
  btScalar _mass = (rb_type != RBType::KIN) ? mass : 0.f;
  bool isDynamic = (_mass != 0.f);
  btVector3 localInertia(0, 0, 0);
  if (isDynamic) {
    bt_shape->calculateLocalInertia(_mass, localInertia);
  }

  // set up rigid body
  // using motionstate is optional, it provides interpolation capabilities, and
  // only synchronizes 'active' objects
  btDefaultMotionState *bt_motion = new btDefaultMotionState(transform);
  btRigidBody::btRigidBodyConstructionInfo rbInfo(_mass, bt_motion, bt_shape,
                                                  localInertia);
  agent->body.bt_bod = new btRigidBody(rbInfo);

  // add to world
  btCollisionObject::CollisionFlags cf = btCollisionObject::CollisionFlags(
      agent->body.bt_bod->getCollisionFlags());
  switch (rb_type) {
    case RBType::BOX:
      p_world->addRigidBody(agent->body.bt_bod, COL_AGENTS,
                            COL_AGENTS | COL_WEAPONS);
      break;
    case RBType::SPHERE:
      p_world->addRigidBody(agent->body.bt_bod, COL_AGENTS,
                            COL_AGENTS | COL_WEAPONS);
      break;
    case RBType::FREE_FORM:
      agent->body.bt_bod->setGravity(btVector3(0.f, 0.f, 0.f));
      agent->body.bt_bod->setAngularFactor(btVector3(0, 0, 0));
      p_world->addRigidBody(agent->body.bt_bod, COL_AGENTS,
                            COL_AGENTS | COL_WEAPONS);
      agent->body.bt_bod->setActivationState(DISABLE_DEACTIVATION);
      break;
    case RBType::KIN:
      agent->body.bt_bod->setCollisionFlags(
          cf | btCollisionObject::CF_KINEMATIC_OBJECT);
      p_world->addRigidBody(agent->body.bt_bod, COL_NOTHING, COL_NOTHING);
      break;
    case RBType::GHOST: {
      agent->body.bt_bod->setGravity(btVector3(0.f, 0.f, 0.f));
      agent->body.bt_bod->setLinearFactor(btVector3(0, 0, 0));
      agent->body.bt_bod->setAngularFactor(btVector3(0, 0, 0));
      /*agent->body.bt_bod->setCollisionFlags(
      cf | btCollisionObject::CF_NO_CONTACT_RESPONSE);
      agent->body.bt_bod->setActivationState(DISABLE_DEACTIVATION);*/
      p_world->addRigidBody(agent->body.bt_bod, COL_NOTHING, COL_NOTHING);

      // constraint
      btTransform frameInA;
      frameInA = btTransform::getIdentity();
      btRigidBody &ghost =
          create_ghost(agent->body.bt_bod->getWorldTransform());
      // set parent id / set physics system constraint
      agent->body.bt_constraint = new btGeneric6DofSpring2Constraint(
          *agent->body.bt_bod, ghost, frameInA, frameInA);

      p_world->addConstraint(agent->body.bt_constraint);
      break;
    }
    default:
      break;
  }
  return true;
}

bool ddAssets::load_screen_check() { return load_screen_flag; }

//*****************************************************************************
//*****************************************************************************

int dd_assets_create_agent(lua_State *L) {
  parse_lua_events(L, fb);
  // get arguments and use them to create ddAgent
  const char *agent_id = fb.get_func_val<const char>("id");
  int64_t *mesh_id = fb.get_func_val<int64_t>("mesh");
  int64_t *sk_id = fb.get_func_val<int64_t>("skeleton");
  int64_t *p_id = fb.get_func_val<int64_t>("parent");
  int64_t *shape = fb.get_func_val<int64_t>("type");
  float *agent_mass = fb.get_func_val<float>("mass");
  float *pos_x = fb.get_func_val<float>("pos_x");
  float *pos_y = fb.get_func_val<float>("pos_y");
  float *pos_z = fb.get_func_val<float>("pos_z");
  float *sc_x = fb.get_func_val<float>("scale_x");
  float *sc_y = fb.get_func_val<float>("scale_y");
  float *sc_z = fb.get_func_val<float>("scale_z");
  float *rot_p = fb.get_func_val<float>("pitch");
  float *rot_y = fb.get_func_val<float>("yaw");
  float *rot_r = fb.get_func_val<float>("roll");

  ddAgent *new_agent = nullptr;
  if (agent_id) {
    size_t out_id = getCharHash(agent_id);

    // check if agent already exists
    new_agent = find_ddAgent(out_id);
    if (new_agent) {
      ddTerminal::f_post("Duplicate agent <%s>", agent_id);
      // return agent id
      lua_pushinteger(L, new_agent->id);
      return 1;
    }

    new_agent = spawn_ddAgent(out_id);
    if (new_agent) {
      // add mesh for render
      ddModelData *mdata = nullptr;
      if (mesh_id) {
        mdata = find_ddModelData((size_t)(*mesh_id));
        if (mdata) {
          // modify ModelIDs struct
          new_agent->mesh.resize(1);
          new_agent->mesh[0].model = mdata->id;
          // initialize material buffer
          new_agent->mesh[0].material.resize(mdata->mesh_info.size());
          DD_FOREACH(DDM_Data, data, mdata->mesh_info) {
            new_agent->mesh[0].material[data.i] = data.ptr->mat_id;
          }
        } else {
          ddTerminal::f_post("[error]  Failed to find mesh <%ld>", *mesh_id);
        }
      }
      // add skeleton for animation
      if (sk_id) {
        ddSkeleton *sk = find_ddSkeleton((size_t)*sk_id);
        if (sk) {
          // create skeleton pose struct and log in agent
          ddSkeletonPose *skpose = spawn_ddSkeletonPose(new_agent->id);
          if (skpose) {
            skpose->sk_id = sk->id;
            skpose->global_mat = sk->global_mat;
            skpose->local_pose.resize(sk->bones.size());
            skpose->global_pose.resize(sk->bones.size());
            new_agent->mesh[0].sk_flag = true;
          } else {
            ddTerminal::f_post("[error]  Failed to find skeleton <%ld>",
                               *sk_id);
          }
        }
      }
      // add ddBody to agent and then agent to world
      float mass = 0.f;
      RBType type = RBType::BOX;
      if (agent_mass) mass = *agent_mass;
      if (shape && *shape == 1) type = RBType::SPHERE;
      if (shape && *shape == -1) type = RBType::FREE_FORM;
      if (shape && *shape == -2) type = RBType::KIN;

      // set position
      glm::vec3 pos;
      if (pos_x) pos.x = *pos_x;
      if (pos_y) pos.y = *pos_y;
      if (pos_z) pos.z = *pos_z;

      // set scale
      glm::vec3 _scale = glm::vec3(1.f);
      if (sc_x) _scale.x = *sc_x;
      if (sc_y) _scale.y = *sc_y;
      if (sc_z) _scale.z = *sc_z;
      new_agent->body.scale = _scale;

      // set rotation
      glm::vec3 _rot = glm::vec3(0.f);
      if (rot_p) _rot.x = glm::radians(*rot_p);
      if (rot_y) _rot.y = glm::radians(*rot_y);
      if (rot_r) _rot.z = glm::radians(*rot_r);

      add_rigid_body(new_agent, mdata, pos, _rot, mass, type);

      // add parent object
      if (p_id) {
        ddAgent *p_agent = find_ddAgent((size_t)*p_id);
        if (p_agent) {
          new_agent->body.parent = p_agent->id;
          new_agent->body.offset = glm::vec3(0.1f, 0.1f, -2.f);
        } else {
          ddTerminal::f_post("[error]  Failed to find parent <%ld>", *p_id);
        }
      }

      // return agent id
      lua_pushinteger(L, new_agent->id);
      return 1;
    }
    ddTerminal::f_post("[error]Failed to create new agent <%s>", agent_id);
  }
  return 0;
}

int dd_assets_create_mesh(lua_State *L) {
  parse_lua_events(L, fb);
  ddModelData *new_mdata = nullptr;
  // get arguments and use them to create ddModelData
  const char *file = fb.get_func_val<const char>("path");
  if (file) {
    cbuff<256> f = file;
    // check file extension
    if (f.contains(".ddm")) {
      new_mdata = load_ddm(file);
      // return key of new mesh back up to script if created
      if (new_mdata) {
        lua_pushinteger(L, new_mdata->id);
        return 1;
      }
    }
    ddTerminal::f_post("[error]Failed to create new mesh <%s>", file);
  }
  ddTerminal::f_post("[error]Failed to create new mesh");
  return 0;
}

int dd_assets_create_cam(lua_State *L) {
  parse_lua_events(L, fb);
  // get arguments and use them to create ddCam
  ddCam *new_cam = nullptr;
  // get arguments
  const char *id = fb.get_func_val<const char>("id");
  int64_t *parent = fb.get_func_val<int64_t>("parent");

  if (id && parent) {
    // check if camera already exists otherwise allocate
    size_t cam_id = getCharHash(id);
    new_cam = find_ddCam(cam_id);
    ddAgent *ag = find_ddAgent((size_t)(*parent));
    if (new_cam && ag) {
      ddTerminal::f_post("Duplicate camera <%s>", id);
    } else {
      new_cam = spawn_ddCam(cam_id);
      // set parent
      new_cam->parent = (size_t)(*parent);
      // set default width, height, focal horiz, near & far
      new_cam->width = native_scr_width;
      new_cam->height = native_scr_height;
      new_cam->fovh = glm::radians(60.f);
      new_cam->n_plane = 0.1f;
      new_cam->f_plane = 50.f;
      new_cam->active = true;
      // rotate agent to face towards the negative z-axis
      // ddBodyFuncs::rotate(
      //&ag->body, glm::vec3(glm::radians(90.f), glm::radians(180.f), 0.f));
    }

    if (new_cam) {
      lua_pushinteger(L, new_cam->id);
      return 1;
    }
    ddTerminal::f_post("[error]Failed to create new camera <%s>", id);
  }
  ddTerminal::f_post("[error]Failed to create new camera");
  return 0;
}

int dd_assets_create_light(lua_State *L) {
  parse_lua_events(L, fb);
  // get arguments and use them to create ddLBulb
  ddLBulb *new_bulb = nullptr;
  // get arguments
  const char *id = fb.get_func_val<const char>("id");
  if (id) {
    // check if light already exist otherwise create new light
    size_t light_id = getCharHash(id);
    new_bulb = find_ddLBulb(light_id);
    if (new_bulb) {
      ddTerminal::f_post("Duplicate light <%s>", id);
    } else {
      new_bulb = spawn_ddLBulb(light_id);
      // initialize
      new_bulb->active = true;
    }

    if (new_bulb) {
      lua_pushinteger(L, new_bulb->id);
      return 1;
    }
    ddTerminal::f_post("[error]Failed to create new light <%s>", id);
  }
  ddTerminal::f_post("[error]Failed to create new light");
  return 0;
}

int dd_assets_create_texture(lua_State *L) {
  parse_lua_events(L, fb);
  // get arguments and use them to create ddLTex2D
  ddTex2D *tex = nullptr;
  // get arguments
  const char *t_id = fb.get_func_val<const char>("id");
  const char *path = fb.get_func_val<const char>("path");

  if (t_id && path) {
    tex = create_tex2D(path, t_id);

    if (tex) {
      // return generated texture id
      lua_pushinteger(L, tex->id);
      return 1;
    }
    ddTerminal::f_post("[error]Failed to create new texture <%s>", path);
  }
  ddTerminal::post("[error]Failed to create new texture");
  return 0;
}

int get_agent_pos_ls(lua_State *L) {
  parse_lua_events(L, fb);

  int64_t *id = fb.get_func_val<int64_t>("id");

  if (id) {
    ddAgent *ag = find_ddAgent((size_t)(*id));
    if (ag) {
      // push vec3 to stack
      glm::vec3 pos = ddBodyFuncs::pos(&ag->body);
      push_vec3_to_lua(L, pos.x, pos.y, pos.z);
      return 1;
    }
  }
  ddTerminal::post("[error]Failed to get agent local position");
  lua_pushnil(L);  // push nil to stack
  return 1;
}

int get_agent_pos_ws(lua_State *L) {
  parse_lua_events(L, fb);

  int64_t *id = fb.get_func_val<int64_t>("id");

  if (id) {
    ddAgent *ag = find_ddAgent((size_t)(*id));
    if (ag) {
      // push vec3 to stack
      glm::vec3 pos = ddBodyFuncs::pos_ws(&ag->body);
      push_vec3_to_lua(L, pos.x, pos.y, pos.z);
      return 1;
    }
  }
  ddTerminal::post("[error]Failed to get agent world position");
  lua_pushnil(L);  // push nil to stack
  return 1;
}

int get_agent_forward_dir(lua_State *L) {
  parse_lua_events(L, fb);

  int64_t *id = fb.get_func_val<int64_t>("id");

  if (id) {
    ddAgent *ag = find_ddAgent((size_t)(*id));
    if (ag) {
      // push vec3 to stack
      glm::vec3 dir = ddBodyFuncs::forward_dir(&ag->body);
      push_vec3_to_lua(L, dir.x, dir.y, dir.z);
      return 1;
    }
  }
  ddTerminal::post("[error]Failed to get agent world position");
  lua_pushnil(L);  // push nil to stack
  return 1;
}

int set_agent_pos(lua_State *L) {
  parse_lua_events(L, fb);

  int64_t *id = fb.get_func_val<int64_t>("id");
  float *_x = fb.get_func_val<float>("x");
  float *_y = fb.get_func_val<float>("y");
  float *_z = fb.get_func_val<float>("z");

  if (id) {
    ddAgent *ag = find_ddAgent((size_t)(*id));
    if (ag) {
      glm::vec3 pos = ddBodyFuncs::pos_ws(&ag->body);

      // set arguments based on availability
      const float x_ = _x ? *_x : pos.x;
      const float y_ = _y ? *_y : pos.y;
      const float z_ = _z ? *_z : pos.z;
      ddBodyFuncs::update_pos(&ag->body, glm::vec3(x_, y_, z_));
      return 0;
    }
  }
  ddTerminal::post("[error]Failed to set agent position");
  return 0;
}

int set_agent_scale(lua_State *L) {
  parse_lua_events(L, fb);

  int64_t *id = fb.get_func_val<int64_t>("id");
  float *_x = fb.get_func_val<float>("x");
  float *_y = fb.get_func_val<float>("y");
  float *_z = fb.get_func_val<float>("z");

  if (id && _x && _y && _z) {
    ddAgent *ag = find_ddAgent((size_t)(*id));
    if (ag) {
      // set agent scale based on arguments
      glm::vec3 new_scale = glm::vec3(*_x, *_y, *_z);
      ddBodyFuncs::update_scale(&ag->body, new_scale);
      p_world->updateSingleAabb(ag->body.bt_bod);

      return 0;
    }
  }
  ddTerminal::post("[error]Failed to set agent scale");
  return 0;
}

int set_agent_vel(lua_State *L) {
  parse_lua_events(L, fb);

  int64_t *id = fb.get_func_val<int64_t>("id");
  float *_x = fb.get_func_val<float>("x");
  float *_y = fb.get_func_val<float>("y");
  float *_z = fb.get_func_val<float>("z");

  if (id) {
    ddAgent *ag = find_ddAgent((size_t)(*id));
    if (ag) {
      // set arguments based on availability
      const float x_ = _x ? *_x : 0;
      const float y_ = _y ? *_y : 0;
      const float z_ = _z ? *_z : 0;

      ddBodyFuncs::update_velocity(&ag->body, glm::vec3(x_, y_, z_));
      return 0;
    }
  }
  ddTerminal::post("[error]Failed to set agent velocity");
  return 0;
}

int rotate_agent(lua_State *L) {
  parse_lua_events(L, fb);

  int64_t *id = fb.get_func_val<int64_t>("id");
  float *_p = fb.get_func_val<float>("pitch");
  float *_y = fb.get_func_val<float>("yaw");
  float *_r = fb.get_func_val<float>("roll");

  if (id) {
    ddAgent *ag = find_ddAgent((size_t)(*id));
    if (ag) {
      // set arguments based on availability
      const float p_ = _p ? *_p : 0;
      const float y_ = _y ? *_y : 0;
      const float r_ = _r ? *_r : 0;

      ddBodyFuncs::rotate(&ag->body, glm::radians(y_), glm::radians(p_),
                          glm::radians(r_));
      return 0;
    }
  }
  ddTerminal::post("[error]Failed to set agent torque");
  return 0;
}

int get_agent_vel(lua_State *L) {
  parse_lua_events(L, fb);

  int64_t *id = fb.get_func_val<int64_t>("id");

  if (id) {
    ddAgent *ag = find_ddAgent((size_t)(*id));
    if (ag) {
      // push vec3 to stack
      const btVector3 vel = ag->body.bt_bod->getLinearVelocity();
      push_vec3_to_lua(L, vel.x(), vel.y(), vel.z());
      return 1;
    }
  }
  ddTerminal::post("[error]Failed to get agent velocity");
  lua_pushnil(L);  // push nil to stack
  return 1;
}

int get_agent_ang_vel(lua_State *L) {
  parse_lua_events(L, fb);

  int64_t *id = fb.get_func_val<int64_t>("id");

  if (id) {
    ddAgent *ag = find_ddAgent((size_t)(*id));
    if (ag) {
      // push vec3 to stack
      const btVector3 ang_vel = ag->body.bt_bod->getAngularVelocity();
      push_vec3_to_lua(L, ang_vel.x(), ang_vel.y(), ang_vel.z());
      return 1;
    }
  }
  ddTerminal::post("[error]Failed to get agent angular velocity");
  lua_pushnil(L);  // push nil to stack
  return 1;
}

int set_agent_friction(lua_State *L) {
  parse_lua_events(L, fb);

  int64_t *id = fb.get_func_val<int64_t>("id");
  float *fr = fb.get_func_val<float>("friction");

  if (id && fr) {
    ddAgent *ag = find_ddAgent((size_t)(*id));
    if (ag) {
      // set agent friction
      ag->body.bt_bod->setFriction(*fr);
      return 0;
    }
  }
  ddTerminal::post("[error]Failed to set agent friction");
  return 0;
}

int set_agent_damping(lua_State *L) {
  parse_lua_events(L, fb);

  int64_t *id = fb.get_func_val<int64_t>("id");
  float *vel = fb.get_func_val<float>("velocity");
  float *ang = fb.get_func_val<float>("angular");

  if (id) {
    ddAgent *ag = find_ddAgent((size_t)(*id));
    if (ag) {
      // set agent damping based on arguments
      float _v = vel ? *vel : (float)ag->body.bt_bod->getAngularDamping();
      float _a = ang ? *ang : (float)ag->body.bt_bod->getLinearDamping();

      ag->body.bt_bod->setDamping(_v, _a);
      return 0;
    }
  }
  ddTerminal::post("[error]Failed to set agent damping");
  return 0;
}

int rotate_camera(lua_State *L) {
  parse_lua_events(L, fb);

  int64_t *id = fb.get_func_val<int64_t>("id");
  float *_p = fb.get_func_val<float>("pitch");
  float *_y = fb.get_func_val<float>("yaw");
  float *_r = fb.get_func_val<float>("roll");

  if (id) {
    ddCam *cam = find_ddCam((size_t)(*id));
    if (cam) {  // set arguments based on availability
      if (_p) cam->pitch = *_p;
      if (_y) cam->yaw = *_y;
      if (_r) cam->roll = *_r;

      return 0;
    }
  }
  ddTerminal::post("[error]Failed to set camera euler angles");
  return 0;
}

//*****************************************************************************

bool add_rigid_body(ddAgent *agent, ddModelData *mdata, glm::vec3 pos,
                    glm::vec3 rot, const float mass, RBType rb_type) {
  if (!agent) return false;

  // set up bounding box
  glm::vec3 bb_max = glm::vec3(0.5f, 0.5f, 0.5f);
  glm::vec3 bb_min = glm::vec3(-.5f, -0.5f, -0.5f);
  if (mdata) {
    bb_max = mdata->mesh_info[0].bb_max * agent->body.scale;
    bb_min = mdata->mesh_info[0].bb_min * agent->body.scale;
  }
  float h_width = (bb_max.x - bb_min.x) * 0.5f;
  float h_height = (bb_max.y - bb_min.y) * 0.5f;
  float h_depth = (bb_max.z - bb_min.z) * 0.5f;

  btCollisionShape *bt_shape = nullptr;
  if (rb_type == RBType::SPHERE) {
    float diameter = glm::length(glm::vec3(h_width, h_height, h_depth));
    bt_shape = new btSphereShape(diameter * 0.5f);
  } else {
    bt_shape = new btBoxShape(
        btVector3(btScalar(h_width), btScalar(h_height), btScalar(h_depth)));
  }

  // set up rigid body constructor
  btTransform transform;
  transform.setIdentity();
  transform.setOrigin(btVector3(pos.x, pos.y, pos.z));
  btQuaternion _q;
  _q.setEuler(rot.y, rot.x, rot.z);
  transform.setRotation(_q);

  // rigidbody is dynamic if and only if mass is non zero, otherwise static
  btScalar _mass = (rb_type != RBType::KIN) ? mass : 0.f;
  bool isDynamic = (_mass != 0.f);
  btVector3 localInertia(0, 0, 0);
  if (isDynamic) {
    bt_shape->calculateLocalInertia(_mass, localInertia);
  }

  // set up rigid body
  // using motionstate is optional, it provides interpolation capabilities, and
  // only synchronizes 'active' objects
  btDefaultMotionState *bt_motion = new btDefaultMotionState(transform);
  btRigidBody::btRigidBodyConstructionInfo rbInfo(_mass, bt_motion, bt_shape,
                                                  localInertia);
  agent->body.bt_bod = new btRigidBody(rbInfo);

  // add to world
  btCollisionObject::CollisionFlags cf = btCollisionObject::CollisionFlags(
      agent->body.bt_bod->getCollisionFlags());
  switch (rb_type) {
    case RBType::BOX:
      p_world->addRigidBody(agent->body.bt_bod, COL_AGENTS,
                            COL_AGENTS | COL_WEAPONS);
      break;
    case RBType::SPHERE:
      p_world->addRigidBody(agent->body.bt_bod, COL_AGENTS,
                            COL_AGENTS | COL_WEAPONS);
      break;
    case RBType::FREE_FORM:
      agent->body.bt_bod->setGravity(btVector3(0.f, 0.f, 0.f));
      agent->body.bt_bod->setAngularFactor(btVector3(0, 0, 0));
      p_world->addRigidBody(agent->body.bt_bod, COL_AGENTS,
                            COL_AGENTS | COL_WEAPONS);
      agent->body.bt_bod->setActivationState(DISABLE_DEACTIVATION);
      break;
    case RBType::KIN:
      agent->body.bt_bod->setCollisionFlags(
          cf | btCollisionObject::CF_KINEMATIC_OBJECT);
      p_world->addRigidBody(agent->body.bt_bod, COL_NOTHING, COL_NOTHING);
      break;
    case RBType::GHOST: {
      agent->body.bt_bod->setGravity(btVector3(0.f, 0.f, 0.f));
      agent->body.bt_bod->setLinearFactor(btVector3(0, 0, 0));
      agent->body.bt_bod->setAngularFactor(btVector3(0, 0, 0));
      /*agent->body.bt_bod->setCollisionFlags(
                      cf | btCollisionObject::CF_NO_CONTACT_RESPONSE);
      agent->body.bt_bod->setActivationState(DISABLE_DEACTIVATION);*/
      p_world->addRigidBody(agent->body.bt_bod, COL_NOTHING, COL_NOTHING);

      // constraint
      btTransform frameInA;
      frameInA = btTransform::getIdentity();
      btRigidBody &ghost =
          create_ghost(agent->body.bt_bod->getWorldTransform());
      // set parent id / set physics system constraint
      agent->body.bt_constraint = new btGeneric6DofSpring2Constraint(
          *agent->body.bt_bod, ghost, frameInA, frameInA);

      p_world->addConstraint(agent->body.bt_constraint);
      break;
    }
    default:
      break;
  }
  return true;
}

btRigidBody &create_ghost(btTransform _transform) {
  btRigidBody *bt_bod = nullptr;
  // set up bounding sphere
  btCollisionShape *bt_shape = new btSphereShape(0.5f);
  ;

  // set up dynamic rigid body constructor
  btScalar _mass(0.1f);
  // bool isDynamic = (_mass != 0.f);
  btVector3 localInertia(0, 0, 0);
  bt_shape->calculateLocalInertia(_mass, localInertia);

  // motion state
  btDefaultMotionState *bt_motion = new btDefaultMotionState(_transform);
  btRigidBody::btRigidBodyConstructionInfo rbInfo(_mass, bt_motion, bt_shape,
                                                  localInertia);
  bt_bod = new btRigidBody(rbInfo);

  // add to world
  p_world->addRigidBody(bt_bod, COL_AGENTS, COL_AGENTS | COL_WEAPONS);

  POW2_VERIFY(bt_bod != nullptr);
  return *bt_bod;
}

void delete_rigid_body(ddAgent *agent) {
  if (!agent) return;
  if (!agent->body.bt_bod) return;

  // remove constraints
  if (agent->body.bt_constraint) {
    // rigidbodyb contains ghost body. Delete
    btRigidBody &ghost = agent->body.bt_constraint->getRigidBodyB();
    p_world->removeRigidBody(&ghost);
    delete ghost.getMotionState();
    delete ghost.getCollisionShape();

    // delete constraint
    p_world->removeConstraint(agent->body.bt_constraint);
    delete agent->body.bt_constraint;
    agent->body.bt_constraint = nullptr;
  }

  // remove body and delete allocated CollisionShape and MotionState
  p_world->removeRigidBody(agent->body.bt_bod);
  delete agent->body.bt_bod->getMotionState();
  delete agent->body.bt_bod->getCollisionShape();
  agent->body.bt_bod = nullptr;
}

void flip_image(unsigned char *image, const int width, const int height,
                const int channels) {
  // validate parameters
  if (width < 0 || height < 0 || channels < 0) {
    fprintf(stderr, "flip_image::Invalid paramters <%d::%d::%d>", width, height,
            channels);
    return;
  }
  // flip
  for (int j = 0; j * 2 < height; j++) {
    int idx_1 = width * channels * j;
    int idx_2 = width * channels * (height - 1 - j);
    for (int i = width * channels; i > 0; i--) {
      unsigned char temp = image[idx_1];
      image[idx_1] = image[idx_2];
      image[idx_2] = temp;
      // img_ptr.get()[idx_2] = image[idx_1];
      idx_1++;
      idx_2++;
    }
  }
}
