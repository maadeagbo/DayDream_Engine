#pragma once

#include "ddIncludes.h"
#include "LuaHooks.h"

#ifndef MAX_SHADERS
#define MAX_SHADERS 50
#endif  // !MAX_SHADERS

/** \brief Render engine interface */
namespace ddRenderer {
/**
* \brief Initialize lua globals for scripts
*/
void init_lua_globals(lua_State *L);

/**
 * \brief Initialize rendering engine
 */
void initialize(const unsigned width, const unsigned height);
/*
 * \brief Clean up resources
 */
void shutdown();

/*
 * \brief Render load screen
 */
void render_load_screen();

/*
* \brief Render 3D meshes in world
*/
void draw_world();
};
