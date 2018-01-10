#pragma once

#include "ddIncludes.h"

#ifndef MAX_SHADERS
#define MAX_SHADERS 50
#endif  // !MAX_SHADERS

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
};
