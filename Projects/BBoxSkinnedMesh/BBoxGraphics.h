#pragma once

#include <map>
#include "ddParticleSystem.h"
#include "ddSceneManager.h"
#include "ddShader.h"

// generate grid line points and indices
void generate_grid();

// render grid
void render_grid();

// fill in buffer
void fill_buffer(const BoundingBox &bbox);

// render bbox
void render_bbox();

// gpu 
int init_gpu_stuff(lua_State *L);
