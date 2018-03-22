#include "ddAnimation.h"
#include "ddAssetManager.h"
#include "ddSceneManager.h"

void state_to_local_pose(ddAnimInfo& a_info, const ddSkeleton* sk,
                         ddAnimState* a_state, const bool first_pass);

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
      const bool flag = state.i == 0 ? true : false;  // first state
      if (state.ptr->active) state_to_local_pose(ag->anim, sk, state.ptr, flag);
    }
  }
}

void state_to_local_pose(ddAnimInfo& a_info, const ddSkeleton* sk,
                         ddAnimState* a_state, const bool first_pass) {
  // calculate local time (if looped, use modulus to get corrected time)

  // use local time and step-size to get frame of animation

  // get interpolating frame (i.e. next frame of animation)

  // set local pose for each joint (compound succesive frames after first pass)
}