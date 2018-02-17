#pragma once

#include "GPUFrontEnd.h"
#include "LuaHooks.h"
#include "ddAssets.h"
#include "ddIncludes.h"
#include "ddPhysicsEngine.h"

/** \brief Masks for physics object interactions */
enum collisiontypes {
  COL_NOTHING = 0,         //< Collide with nothing
  COL_AGENTS = DD_BIT(0),  //< Collide with ddAgent
  COL_WEAPONS = DD_BIT(1)  //< Collide with walls
};

/** \brief Rigid body types */
enum class RBType : unsigned { BOX, SPHERE, FREE_FORM, KIN, GHOST };

/** \brief Interace for manipulating and creating assets for in-engine use */
namespace ddAssets {
/**
 * \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine
 */
void initialize(btDiscreteDynamicsWorld* physics_world);
/**
 * \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine
 */
void set_load_screen_flag(const bool flag);
/**
 * \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine
 */
void cleanup();
/**
 * \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine
 */
void default_params(const unsigned scr_width, const unsigned scr_height);
/**
 * \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine
 */
void log_lua_func(lua_State* L);

/**
 * \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine
 */
void load_to_gpu();

/**
 * \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine
 */
void remove_rigid_body(ddAgent* ag);

/**
 * \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddLuaLib_ddAgent
 */
bool add_body(ddAgent* agent, ddModelData* mdata, glm::vec3 pos,
                    glm::vec3 rot, const float mass, RBType rb_type);

/**
 * \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddLuaLib_ddAgent
 */
bool load_screen_check();                    
};  // namespace ddAssets
