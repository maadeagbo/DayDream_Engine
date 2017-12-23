#pragma once

#include "DD_BaseAgent.h"
#include "DD_Camera.h"
#include "DD_Light.h"
#include "DD_LuaHooks.h"
#include "DD_Model.h"
#include "DD_Skeleton.h"
#include "DD_Texture2D.h"
#include "DD_Types.h"
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

#define ASSET_DEF(TYPE, CONTAINER)                                    \
  TYPE* spawn##TYPE(const size_t id) {                                \
    int free_idx = -1;                                                \
    for (unsigned i = 0; i < CONTAINER.size() && free_idx < 0; i++) { \
      if (freelist_##CONTAINER[i]) {                                  \
        free_idx = (int)i;                                            \
      }                                                               \
    }                                                                 \
    if (free_idx < 0) {                                               \
      return nullptr;                                                 \
    }                                                                 \
    freelist_##CONTAINER[free_idx] = false;                           \
    map_##CONTAINER[id] = (unsigned)free_idx;                         \
    CONTAINER[(unsigned)free_idx] = TYPE();                           \
    CONTAINER[(unsigned)free_idx].id = id;                            \
    return &CONTAINER[(unsigned)free_idx];                            \
  }                                                                   \
  bool destroy##TYPE(const size_t id) {                               \
    if (map_##CONTAINER.count(id) == 0) {                             \
      return false;                                                   \
    } else {                                                          \
      unsigned idx = map_##CONTAINER[id];                             \
      freelist_##CONTAINER[idx] = true;                               \
      map_##CONTAINER.erase(id);                                      \
      return true;                                                    \
    }                                                                 \
  }                                                                   \
  TYPE* find##TYPE(const size_t id) {                                 \
    if (map_##CONTAINER.count(id) == 0) {                             \
      return nullptr;                                                 \
    } else {                                                          \
      return &CONTAINER[map_##CONTAINER[id]];                         \
    }                                                                 \
  }

#define ASSET_DEF_PTR(TYPE, CONTAINER)                                \
  TYPE* spawn##TYPE(const size_t id) {                                \
    int free_idx = -1;                                                \
    for (unsigned i = 0; i < CONTAINER.size() && free_idx < 0; i++) { \
      if (freelist_##CONTAINER[i]) {                                  \
        free_idx = (int)i;                                            \
      }                                                               \
    }                                                                 \
    if (free_idx < 0) {                                               \
      return nullptr;                                                 \
    }                                                                 \
    freelist_##CONTAINER[free_idx] = false;                           \
    map_##CONTAINER[id] = (unsigned)free_idx;                         \
    CONTAINER[(unsigned)free_idx] = new TYPE();                       \
    CONTAINER[(unsigned)free_idx].id = id;                            \
    return CONTAINER[(unsigned)free_idx];                             \
  }                                                                   \
  bool destroy##TYPE(const size_t id) {                               \
    if (map_##CONTAINER.count(id) == 0) {                             \
      return false;                                                   \
    } else {                                                          \
      unsigned idx = map_##CONTAINER[id];                             \
      delete CONTAINER[idx];                                          \
      freelist_##CONTAINER[idx] = true;                               \
      map_##CONTAINER.erase(id);                                      \
      return true;                                                    \
    }                                                                 \
  }                                                                   \
  TYPE* find##TYPE(const size_t id) {                                 \
    if (map_##CONTAINER.count(id) == 0) {                             \
      return nullptr;                                                 \
    } else {                                                          \
      return CONTAINER[map_##CONTAINER[id]];                          \
    }                                                                 \
  }

#define ASSET_CREATE(TYPE, CONTAINER, C_SIZE)  \
  std::map<size_t, unsigned> map_##CONTAINER;  \
  dd_array<bool> freelist_##CONTAINER(C_SIZE); \
  dd_array<TYPE> CONTAINER(C_SIZE);

namespace DD_Assets {
/// \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY DD_ENGINE
void initialize(btDiscreteDynamicsWorld* physics_world);

/// \brief Create DD_BaseAgent from lua scripts
/// \param L lua state
/// \return Number of returned values to lua
int create_agent(lua_State* L);
/// \brief Create DD_MeshData from lua scripts
/// \param L lua state
/// \return Number of returned values to lua
int create_mesh(lua_State* L);
/// \brief Create DD_Cam from lua scripts
/// \param L lua state
/// \return Number of returned values to lua
int create_cam(lua_State* L);
/// \brief Create DD_Bulb from lua scripts
/// \param L lua state
/// \return Number of returned values to lua
int create_light(lua_State* L);

ASSET_DECL(DD_BaseAgent)
ASSET_DECL(DD_Cam)
ASSET_DECL(DD_Bulb)
ASSET_DECL(DD_MeshData)
ASSET_DECL(DD_Skeleton)
ASSET_DECL(DD_SkeletonPose)
ASSET_DECL(DD_Tex2D)
}  // namespace DD_Assets
