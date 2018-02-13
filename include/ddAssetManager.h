#pragma once

#include "GPUFrontEnd.h"
#include "ddIncludes.h"
#include "LuaHooks.h"
#include "ddPhysicsEngine.h"
#include "ddAssets.h"

/** \brief Interace for manipulating and creating assets for in-engine use */
namespace ddAssets {
/**
 * \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine
 */
void initialize(btDiscreteDynamicsWorld* physics_world);
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
};  // namespace ddAssets