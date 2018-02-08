#include "ddParticleSystem.h"
#include "ddTimer.h"

#define PARTICLE_Q_MAX 200

namespace {
// screen information
unsigned scr_width = 0;
unsigned scr_height = 0;

// particle queue attributes
unsigned head = 0;
unsigned tail = 0;
unsigned num_tasks = 0;
dd_array<ddPTask *> particle_q = dd_array<ddPTask *>(PARTICLE_Q_MAX);
};  // namespace

namespace ddParticleSys {

/** \brief Push task id on queue */
void push_task(ddPTask &pt);
/** \brief Remove task id from queue */
void pop_task(ddPTask *&pt);
/** \brief Process task queue (deletes task gpu memory if task is complete) */
void process_queue();

void initialization(const unsigned width, const unsigned height) {
  scr_width = width;
  scr_height = height;
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
  process_queue();

  // reset buffer
  ddGPUFrontEnd::bind_framebuffer(ddBufferType::DEFAULT);
  ddGPUFrontEnd::toggle_additive_blend(false);
}

void cleanup() {
  // delete storage buffers of tasks still in queue (maybe?)
}

bool add_task(ddPTask &new_task, const unsigned buffer_size_bytes) {
  if (num_tasks == PARTICLE_Q_MAX) {
    return false;
  }
  push_task(new_task);

  return true;
}

void push_task(ddPTask &pt) {
  if (num_tasks == PARTICLE_Q_MAX) {
    return;
  }
  // setup new task
  particle_q[tail] = &pt;
  tail = (tail + 1) % PARTICLE_Q_MAX;
  num_tasks++;
}

void pop_task(ddPTask *&pt) {
  if (num_tasks == 0) {
    return;
  }
  // retrieve task
  pt = particle_q[head];
  head = (head + 1) % PARTICLE_Q_MAX;
  num_tasks--;
}

void process_queue() {
  ddPTask *pt = nullptr;
  unsigned tasks_on_q = 0;
  const float ftime = ddTime::get_avg_frame_time();

  // loop thru available tasks
  if (tail < head) {
    tasks_on_q = (PARTICLE_Q_MAX - head) + tail;
  } else {
    tasks_on_q = tail - head;
  }

  for (unsigned i = 0; i < tasks_on_q; i++) {
    // get next task
    pop_task(pt);

    // check perform render function if task exists (skip null tasks and funcs)
    if (pt != nullptr) {
      // assign buffer space if necessary
      if (!pt->buff) {
        ddGPUFrontEnd::create_storage_buffer(pt->buff, pt->buff_size);
      }

      if (pt->rfunc != nullptr) pt->rfunc();

      // decrement life span (if tasks isn't permanent)
      if (!pt->remain_on_q) {
        pt->lifespan -= ftime;

        // add back to queue if task not complete
        if (pt->lifespan > 0.f) push_task(*pt);
      } else {
        push_task(*pt);
      }
    }
  }
}
}  // namespace ddParticleSys
