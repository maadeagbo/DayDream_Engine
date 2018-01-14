#pragma once

#include "GPUFrontEnd.h"
#include "LuaHooks.h"
#include "ddBaseAgent.h"
#include "ddCamera.h"
#include "ddIncludes.h"
#include "ddLight.h"
#include "ddMaterial.h"
#include "ddModel.h"
#include "ddSkeleton.h"
#include "ddTexture2D.h"
#include "freelist.h"

#ifndef ASSETS_CONTAINER_MAX_SIZE
#define ASSETS_CONTAINER_MAX_SIZE 200
#endif  // !ASSETS_CONTAINER_MAX_SIZE

#ifndef ASSETS_CONTAINER_MIN_SIZE
#define ASSETS_CONTAINER_MIN_SIZE 50
#endif  // !ASSETS_CONTAINER_MIN_SIZE

#define ASSET_DECL(TYPE)                \
  TYPE* spawn_##TYPE(const size_t id);  \
  bool destroy_##TYPE(const size_t id); \
  TYPE* find_##TYPE(const size_t id);

#define ASSET_DECL_PTR(TYPE)            \
  TYPE* spawn_##TYPE(const size_t id);  \
  bool destroy_##TYPE(const size_t id); \
  TYPE* find_##TYPE(const size_t id);

#define ASSET_DEF(TYPE, CONTAINER)                 \
  TYPE* spawn_##TYPE(const size_t id) {            \
    uint32_t free_idx = 0;                         \
    if (!fl_##CONTAINER.nxt_free_slot(free_idx)) { \
      return nullptr;                              \
    }                                              \
    map_##CONTAINER[id] = free_idx;                \
    CONTAINER[free_idx] = TYPE();                  \
    CONTAINER[free_idx].id = id;                   \
    return &CONTAINER[free_idx];                   \
  }                                                \
  bool destroy_##TYPE(const size_t id) {           \
    if (map_##CONTAINER.count(id) == 0) {          \
      return false;                                \
    } else {                                       \
      uint32_t idx = map_##CONTAINER[id];          \
      CONTAINER[idx].~TYPE();                      \
      fl_##CONTAINER.release_slot(idx);            \
      map_##CONTAINER.erase(id);                   \
      return true;                                 \
    }                                              \
  }                                                \
  TYPE* find_##TYPE(const size_t id) {             \
    if (map_##CONTAINER.count(id) == 0) {          \
      return nullptr;                              \
    } else {                                       \
      return &CONTAINER[map_##CONTAINER[id]];      \
    }                                              \
  }

#define ASSET_DEF_PTR(TYPE, CONTAINER)             \
  TYPE* spawn_##TYPE(const size_t id) {            \
    uint32_t free_idx = 0;                         \
    if (!fl_##CONTAINER.nxt_free_slot(free_idx)) { \
      return nullptr;                              \
    }                                              \
    map_##CONTAINER[id] = free_idx;                \
    CONTAINER[free_idx] = new TYPE();              \
    CONTAINER[free_idx].id = id;                   \
    return CONTAINER[free_idx];                    \
  }                                                \
  bool destroy_##TYPE(const size_t id) {           \
    if (map_##CONTAINER.count(id) == 0) {          \
      return false;                                \
    } else {                                       \
      uint32_t idx = map_##CONTAINER[id];          \
      delete CONTAINER[idx];                       \
      CONTAINER[idx] = nullptr;                    \
      fl_##CONTAINER.release_slot(idx);            \
      map_##CONTAINER.erase(id);                   \
      return true;                                 \
    }                                              \
  }                                                \
  TYPE* find_##TYPE(const size_t id) {             \
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
 * \brief Get currently active camera (selects to 1st active camera it finds)
 */
ddCam* get_active_cam();

};  // namespace ddAssets

namespace ddSceneManager {

/**
 * \brief Cull objects outside of camera frustum 
 * \param _agents must be allocated to size ASSETS_CONTAINER_MAX_SIZE
 */
void cull_objects(const FrustumBox fr, const glm::mat4 view_m,
                  dd_array<ddAgent*>& _agents);

/**
 * \brief Get all active lights in scene
 * \param _lights must be allocated to size ASSETS_CONTAINER_MIN_SIZE
 */
void get_active_lights(dd_array<ddLBulb*>& _lights);

/**
 * \brief Get currently active directional light
 */
ddLBulb *get_shadow_light();

}  // namespace ddSceneManager

ASSET_DECL(ddAgent)
ASSET_DECL(ddCam)
ASSET_DECL(ddLBulb)
ASSET_DECL(ddModelData)
ASSET_DECL(ddSkeleton)
ASSET_DECL(ddSkeletonPose)
ASSET_DECL(ddTex2D)
ASSET_DECL(ddMat)
