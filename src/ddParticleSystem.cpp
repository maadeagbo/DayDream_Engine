#include "ddParticleSystem.h"
#include "ddAssetManager.h"

namespace {
// screen information
unsigned scr_width = 0;
unsigned scr_height = 0;
};

namespace ddParticleSys {

// Buffer of tasks
ASSET_DECL(ddPTask)
ASSET_CREATE(ddPTask, p_tasks, ASSETS_CONTAINER_MAX_SIZE)
ASSET_DEF(ddPTask, p_tasks)

/** \brief Destroy created task */
void remove_task(const size_t task_id);

void initialization(const unsigned width, const unsigned height) {
  scr_width = width;
  scr_height = height;

  // initialize tasks bin
  fl_p_tasks.initialize(p_tasks.size());
}

void render_tasks() {
  // copy gbuffer depth
  ddGPUFrontEnd::blit_depth_buffer(ddBufferType::GEOM, ddBufferType::PARTICLE,
                                   scr_width, scr_height);

  // setup particle buffer
  ddGPUFrontEnd::bind_framebuffer(ddBufferType::PARTICLE);
  ddGPUFrontEnd::clear_color_buffer();
  ddGPUFrontEnd::toggle_additive_blend(true);

  // loop thru tasks
  for (auto &idx : map_p_tasks) {
    ddPTask *pt = &p_tasks[idx.second];
    // call shader/render function
    pt->sfunc();
  }

  // reset buffer
  ddGPUFrontEnd::bind_framebuffer(ddBufferType::DEFAULT);
  ddGPUFrontEnd::toggle_additive_blend(false);
}

void cleanup() {
  // delete storage buffers
  for (auto &idx : map_p_tasks) {
    ddGPUFrontEnd::destroy_storage_buffer(p_tasks[idx.second].buff);
  }
}

ddPTask *add_task(const char *task_name) {
  ddPTask *pt = nullptr;
  // get new task
  pt = spawn_ddPTask(getCharHash(task_name));

  return pt;
}

void remove_task(const size_t task_id) {
  // delete task if it exists
  if (map_p_tasks.count(task_id) > 0) {
    uint32_t idx = map_p_tasks[task_id];
    ddGPUFrontEnd::destroy_storage_buffer(p_tasks[idx].buff);
    bool success = destroy_ddPTask(task_id);
    POW2_VERIFY(success == true);
  }
}
}