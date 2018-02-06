#include "ddParticleSystem.h"
#include "ddAssetManager.h"

namespace {
// screen information
unsigned scr_width = 0;
unsigned scr_height = 0;

/** \brief VAO for rendering particles */
ddVAOData *p_vao = nullptr;
};

namespace ddParticleSys {

ASSET_CREATE(ddPTask, p_tasks, ASSETS_CONTAINER_MAX_SIZE)
ASSET_DEF(ddPTask, p_tasks)

void initialization(const unsigned width, const unsigned height) {
  scr_width = width;
  scr_height = height;

  // initialize objects for rendering
  ddGPUFrontEnd::create_vao(p_vao);

  // initialize tasks bin
  fl_p_tasks.initialize(p_tasks.size());
}

void render_tasks(const unsigned width, const unsigned height) {
  // setup particle buffer

  // loop thru tasks
  for (auto &idx : map_p_tasks) {
    // call shader functions

    // render based on flags
  }
}

void cleanup() {
  // delete objects for rendering
  ddGPUFrontEnd::destroy_vao(p_vao);

  // delete storage buffers
  for (auto &idx : map_p_tasks) {
    ddGPUFrontEnd::destroy_storage_buffer(p_tasks[idx.second].buff);
  }
}

ddPTask* add_task(const char* task_name) {
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