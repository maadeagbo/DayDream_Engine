#pragma once

#include "ddAssetManager.h"

/** function pointer for particle shader initialization */
typedef std::function<void()> ShaderSetupFunc;

enum class ddPType : unsigned { 
  POINTS = DD_BIT(0), 
  TRIANGLES = DD_BIT(1),
  NULL_ = 0 
};
ENABLE_BITMASK_OPERATORS(ddPType)

/** \brief Contains particle task information */
struct ddPTask {
  /** \brief in-engine object id (set by engine) */ 
  size_t id = 0;
  /** \brief User-implemented shader setup function (called before particle render) */
  ShaderSetupFunc sfunc;
  /** \brief Particle position buffer (vec3 positions) */
  ddStorageBufferData *buff = nullptr;
  /** \brief Render type for particles */
  ddPType type = ddPType::NULL_;
};

/** \brief Particle system interface for ddRenderer */
namespace ddParticleSys {

// Buffer of tasks
ASSET_DECL(ddPTask)

/** \brief ONLY CALLED INTERNALLY BY ddRenderEngine. DO NOT USE */
void initialization(const unsigned width, const unsigned height);

/** \brief ONLY CALLED INTERNALLY BY ddRenderEngine. DO NOT USE */
void render_tasks(const unsigned width, const unsigned height);

/** \brief ONLY CALLED INTERNALLY BY ddEngine. DO NOT USE */
void cleanup();

/** 
 * \brief Add task to queue 
 * \return Newly created particle task (if queue still has space)
 */
ddPTask* add_task(const char* task_name);

/** \brief Destroy created task */
void remove_task(const size_t task_id);
};
