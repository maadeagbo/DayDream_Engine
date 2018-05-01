#include "ddAnimation.h"
#include "ddAssetManager.h"
#include "ddSceneManager.h"
#include "ddTerminal.h"

namespace {
bool flag_bin[MAX_JOINTS];
}

/** \brief Translate animation state to current frame's local pose data */
void state_to_local_pose(ddAnimInfo& a_info, ddAnimState* a_state,
                         const bool first_pass);

/** \brief Calculate the global pose matrix from local pose data */
void local_to_global_pose(ddAnimInfo& a_info, const ddSkeleton* sk,
                          bool* visited, const unsigned idx);

void ddAnimation::process_animations() {
  // Get list of visible agents from active camera
  ddCam* cam = ddSceneManager::get_active_cam();
  POW2_VERIFY_MSG(cam != nullptr, "No active camera", 0);

  ddAgent* cam_p = find_ddAgent(cam->parent);
  POW2_VERIFY_MSG(cam_p != nullptr, "Active camera has no parent", 0);

  const ddVisList* vlist = ddSceneManager::get_visibility_list(cam->id);

  // for agents w/ animation states, build up final local & global pose
  DD_FOREACH(ddAgent*, item, vlist->visible_agents) {
    ddAgent* ag = *item.ptr;

    // determine if agent has all pieces necessary for animation
    if (!ag->anim.states.isValid()) continue;

    ddSkeleton* sk = find_ddSkeleton(ag->anim.sk_id);
    if (!sk) continue;

    // loop thru animations and perform recursize build up of joint pose data
    DD_FOREACH(ddAnimState, state, ag->anim.states) {
      const bool f_flag = state.i == 0 ? true : false;  // first state
      if (state.ptr->active) state_to_local_pose(ag->anim, state.ptr, f_flag);

      // clear flag bin
      for (unsigned i = 0; i < sk->bones.size(); i++) {
        flag_bin[i] = false;
      }

      // calculate final global pose
      for (unsigned i = 0; i < sk->bones.size(); i++) {
        local_to_global_pose(ag->anim, sk, flag_bin, i);
      }

      // finalize for vertex shader
      if (!ag->anim.global_calc) {
        for (unsigned i = 0; i < sk->bones.size(); i++) {
          ag->anim.global_pose[i] =
              ag->anim.global_pose[i] * sk->bones[i].inv_bp;
        }
      }

      // update bounding box if necessary
      if (ag->body.oobb_data.oobbs.isValid()) {
        ddBodyFuncs::AABB aabb;
        DD_FOREACH(OOBoundingBox, oobb, ag->body.oobb_data.oobbs) {
          // get oobb and tranform it using final delta pose for joint
          BoundingBox bbox = oobb.ptr->get_bbox();
          const unsigned j_idx = (oobb.ptr->joint_idx >= 0 &&
                                  oobb.ptr->joint_idx < sk->bones.size())
                                     ? oobb.ptr->joint_idx
                                     : 0;

          // works best w/ an inverse bind pose present
          bbox = bbox.transformCorners(ag->anim.global_pose[j_idx]);
          aabb.update(bbox.min);
          aabb.update(bbox.max);
        }
        // log min and max vertex then update
        ag->body.oobb_data.max_vert = aabb.max;
        ag->body.oobb_data.min_vert = aabb.min;
        ddBodyFuncs::update_aabb(&ag->body, aabb);
      }
    }
  }
}

void state_to_local_pose(ddAnimInfo& a_info, ddAnimState* a_state,
                         const bool first_pass) {
  // get clip
  ddAnimClip* a_clip = find_ddAnimClip(a_state->clip_id);
  if (!a_clip) {
    ddTerminal::f_post("state_to_local_pose::Invalid ddAnimClip %llu",
                       (long long unsigned)a_state->clip_id);
  }

  // correct local time (if looped, use modulus to get corrected time)
  if (a_state->local_time >= (a_clip->length)) {
    // loop forwards
    if (a_state->flag_loop) {
      a_state->local_time = std::fmodf(a_state->local_time, a_clip->length);
    } else {
      a_state->local_time = 0.f;
      a_state->active = false;
    }
  } else if (a_state->local_time < 0) {  // loop backwards
    if (a_state->flag_loop) {
      /*const float x = (a_state->local_time * -1) / a_clip->length;
      const unsigned x_int = (unsigned)x;
      a_state->local_time = a_clip->length - (x - (float)x_int);*/
      const float x = std::fmodf(a_state->local_time, a_clip->length);
      a_state->local_time = a_clip->length + x;
    } else {
      a_state->local_time = 0.f;
      a_state->active = false;
    }
  }

  // use local time and step-size to get frame of animation
  const unsigned idx_a = (unsigned)(a_state->local_time / a_clip->step_size);

  // get interpolating frame (i.e. next frame of animation)
  unsigned idx_b =
      (idx_a + 1) < a_clip->num_frames ? (idx_a + 1) : a_clip->num_frames - 1;
  idx_b = (idx_b == (a_clip->num_frames - 1)) && a_state->flag_loop ? 0 : idx_b;

  const float alpha =
      (a_state->local_time - (idx_a * a_clip->step_size)) / a_clip->step_size;

  // set local pose for each joint (compound succesive frames after first pass)
  for (unsigned j = 0; j < a_info.local_pose.size(); j++) {
    glm::quat iden;
    const glm::quat qa = a_clip->samples[idx_a].pose[j].rot;
    const glm::quat qb = a_clip->samples[idx_b].pose[j].rot;

    const glm::vec3 ta = a_clip->samples[idx_a].pose[j].trans * a_state->weight;
    const glm::vec3 tb = a_clip->samples[idx_b].pose[j].trans * a_state->weight;

    if (first_pass) {
      // reset local pose to new animation
      if (a_state->interpolate) {
        const glm::quat temp = glm::slerp(qa, qb, alpha);
        // apply weight
        a_info.local_pose[j].rot = glm::slerp(iden, temp, a_state->weight);

        a_info.local_pose[j].trans = ta * (1 - alpha) + (tb * alpha);
      } else {
        a_info.local_pose[j].rot = qa;
        a_info.local_pose[j].trans = ta;
      }
    } else {
      // compounded animations
      if (a_state->interpolate) {
        glm::quat temp = glm::slerp(qa, qb, alpha);
        // apply weight
        temp = glm::slerp(iden, temp, a_state->weight);
        a_info.local_pose[j].rot = temp * a_info.local_pose[j].rot;

        a_info.local_pose[j].trans =
            (ta * (1 - alpha) + (tb * alpha)) + a_info.local_pose[j].trans;
      } else {
        a_info.local_pose[j].rot = qa * a_info.local_pose[j].rot;
        a_info.local_pose[j].trans = ta + a_info.local_pose[j].trans;
      }
    }
  }
}

void local_to_global_pose(ddAnimInfo& a_info, const ddSkeleton* sk,
                          bool* visited, const unsigned idx) {
  if (!visited[idx]) {
    visited[idx] = true;

    ddJointPose& jntp = a_info.local_pose[idx];
    const unsigned p_idx = sk->bones[idx].parent;

    // check parent node before calculating matrix
    if (!visited[p_idx]) {
      local_to_global_pose(a_info, sk, visited, p_idx);
    }
    // calculate matrix
    glm::mat4 trans = glm::translate(glm::mat4(), jntp.trans);
    glm::mat4 rot = glm::mat4_cast(jntp.rot);
    // trans = rot = glm::mat4();

    if (idx == 0) {
      a_info.global_pose[0] = sk->global_mat * trans * rot;
    } else {
      if (!a_info.global_calc) {
        a_info.global_pose[idx] =
            a_info.global_pose[p_idx] * sk->bones[idx].p_delta * trans * rot;
      } else {
        a_info.global_pose[idx] = trans * rot;
      }
    }
  }
}
