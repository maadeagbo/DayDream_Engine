#pragma once

#include "ddBaseAgent.h"
#include "ddCamera.h"
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
  TYPE *spawn_##TYPE(const size_t id);  \
  bool destroy_##TYPE(const size_t id); \
  dd_array<TYPE *> get_all_##TYPE();    \
  TYPE *find_##TYPE(const size_t id);

#define ASSET_DECL_PTR(TYPE)            \
  TYPE *spawn_##TYPE(const size_t id);  \
  bool destroy_##TYPE(const size_t id); \
  TYPE *find_##TYPE(const size_t id);

#define ASSET_DEF(TYPE, CONTAINER)                 \
  TYPE *spawn_##TYPE(const size_t id) {            \
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
      CONTAINER[idx].id = 0;                       \
      fl_##CONTAINER.release_slot(idx);            \
      map_##CONTAINER.erase(id);                   \
      return true;                                 \
    }                                              \
  }                                                \
  dd_array<TYPE *> get_all_##TYPE() {              \
    dd_array<TYPE *> ids(map_##CONTAINER.size());  \
    unsigned i = 0;                                \
    for (auto &idx : map_##CONTAINER) {            \
      ids[i] = &CONTAINER[idx.second];             \
      i++;                                         \
    }                                              \
    return ids;                                    \
  }                                                \
  TYPE *find_##TYPE(const size_t id) {             \
    if (map_##CONTAINER.count(id) == 0) {          \
      return nullptr;                              \
    } else {                                       \
      return &CONTAINER[map_##CONTAINER[id]];      \
    }                                              \
  }

#define ASSET_DEF_PTR(TYPE, CONTAINER)             \
  TYPE *spawn_##TYPE(const size_t id) {            \
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
  TYPE *find_##TYPE(const size_t id) {             \
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

ASSET_DECL(ddAgent)
ASSET_DECL(ddCam)
ASSET_DECL(ddLBulb)
ASSET_DECL(ddModelData)
ASSET_DECL(ddSkeleton)
ASSET_DECL(ddSkeletonPose)
ASSET_DECL(ddTex2D)
ASSET_DECL(ddAnimClip)
ASSET_DECL(ddMat)

/** \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddAssetManager */
void init_assets();

/** \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddAssetManager */
void cleanup_all_assets();
