#pragma once

#include "GPUFrontEnd.h"
#include "ddIncludes.h"

/** function pointer for particle shader initialization */
typedef std::function<void()> RenderFunc;

/** \brief Contains particle task information */
struct ddPTask {
  /** \brief Time left on queue */
  float lifespan = 0.f;
  /** \brief Buffer to be assigned to task */
  unsigned buff_size = 0;
  /** \brief Flag to tell queue not to remove task */
  bool remain_on_q = false;
  /** \brief User-implemented shader setup function (called before particle
   * render) */
  RenderFunc rfunc;
};

/** \brief Particle system interface for ddRenderer */
namespace ddParticleSys {

/** \brief ONLY CALLED INTERNALLY BY ddRenderEngine. DO NOT USE */
void initialization(const unsigned width, const unsigned height);

/** \brief ONLY CALLED INTERNALLY BY ddRenderEngine. DO NOT USE */
void render_tasks();

/** \brief ONLY CALLED INTERNALLY BY ddEngine. DO NOT USE */
void cleanup();

/**
 * \brief Add task to queue
 * \return Newly created particle task (if queue still has space)
 */
bool add_task(ddPTask& new_task);
};
