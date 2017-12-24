#pragma once

#include "DD_BaseAgent.h"
#include "DD_Camera.h"
#include "DD_Light.h"
#include "DD_LuaHooks.h"
#include "DD_Model.h"
#include "DD_Skeleton.h"
#include "DD_Texture2D.h"
#include "DD_Types.h"
#include "DD_Material.h"
#include "freelist.h"

#ifndef ASSETS_CONTAINER_MAX_SIZE
#define ASSETS_CONTAINER_MAX_SIZE 200
#endif  // !ASSETS_CONTAINER_MAX_SIZE

#ifndef ASSETS_CONTAINER_MIN_SIZE
#define ASSETS_CONTAINER_MIN_SIZE 50
#endif  // !ASSETS_CONTAINER_MIN_SIZE

#define ASSET_DECL(TYPE)               \
  TYPE* spawn##TYPE(const size_t id);  \
  bool destroy##TYPE(const size_t id); \
  TYPE* find##TYPE(const size_t id);

#define ASSET_DECL_PTR(TYPE)           \
  TYPE* spawn##TYPE(const size_t id);  \
  bool destroy##TYPE(const size_t id); \
  TYPE* find##TYPE(const size_t id);

#define ASSET_DEF(TYPE, CONTAINER)                 \
  TYPE* spawn##TYPE(const size_t id) {             \
    uint32_t free_idx = 0;                         \
    if (!fl_##CONTAINER.nxt_free_slot(free_idx)) { \
      return nullptr;                              \
    }                                              \
    map_##CONTAINER[id] = free_idx;                \
    CONTAINER[free_idx] = TYPE();                  \
    CONTAINER[free_idx].id = id;                   \
    return &CONTAINER[free_idx];                   \
  }                                                \
  bool destroy##TYPE(const size_t id) {            \
    if (map_##CONTAINER.count(id) == 0) {          \
      return false;                                \
    } else {                                       \
      uint32_t idx = map_##CONTAINER[id];          \
      fl_##CONTAINER.release_slot(idx);            \
      map_##CONTAINER.erase(id);                   \
      return true;                                 \
    }                                              \
  }                                                \
  TYPE* find##TYPE(const size_t id) {              \
    if (map_##CONTAINER.count(id) == 0) {          \
      return nullptr;                              \
    } else {                                       \
      return &CONTAINER[map_##CONTAINER[id]];      \
    }                                              \
  }

#define ASSET_DEF_PTR(TYPE, CONTAINER)             \
  TYPE* spawn##TYPE(const size_t id) {             \
    uint32_t free_idx = 0;                         \
    if (!fl_##CONTAINER.nxt_free_slot(free_idx)) { \
      return nullptr;                              \
    }                                              \
    map_##CONTAINER[id] = free_idx;                \
    CONTAINER[free_idx] = new TYPE();              \
    CONTAINER[free_idx].id = id;                   \
    return CONTAINER[free_idx];                    \
  }                                                \
  bool destroy##TYPE(const size_t id) {            \
    if (map_##CONTAINER.count(id) == 0) {          \
      return false;                                \
    } else {                                       \
      uint32_t idx = map_##CONTAINER[id];          \
      delete CONTAINER[idx];                       \
      fl_##CONTAINER.release_slot(idx);            \
      map_##CONTAINER.erase(id);                   \
      return true;                                 \
    }                                              \
  }                                                \
  TYPE* find##TYPE(const size_t id) {              \
    if (map_##CONTAINER.count(id) == 0) {          \
      return nullptr;                              \
    } else {                                       \
      return CONTAINER[map_##CONTAINER[id]];       \
    }                                              \
  }

#define ASSET_CREATE(TYPE, CONTAINER, C_SIZE) \
  std::map<size_t, uint32_t> map_##CONTAINER; \
  freelist fl_##CONTAINER;                    \
  dd_array<TYPE> CONTAINER(C_SIZE);

/// \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY DD_ENGINE
void dd_assets_initialize(btDiscreteDynamicsWorld* physics_world);

/// \brief Create DD_BaseAgent from lua scripts
/// \param L lua state
/// \return Number of returned values to lua
int dd_assets_create_agent(lua_State* L);
/// \brief Create DD_MeshData from lua scripts
/// \param L lua state
/// \return Number of returned values to lua
int dd_assets_create_mesh(lua_State* L);
/// \brief Create DD_Cam from lua scripts
/// \param L lua state
/// \return Number of returned values to lua
int dd_assets_create_cam(lua_State* L);
/// \brief Create DD_Bulb from lua scripts
/// \param L lua state
/// \return Number of returned values to lua
int dd_assets_create_light(lua_State* L);

ASSET_DECL(DD_BaseAgent)
ASSET_DECL(DD_Cam)
ASSET_DECL(DD_Bulb)
ASSET_DECL(DD_MeshData)
ASSET_DECL(DD_Skeleton)
ASSET_DECL(DD_SkeletonPose)
ASSET_DECL(DD_Tex2D)
ASSET_DECL(DD_Mat)
