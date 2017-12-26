#include <DD_AnimSystem.h>

void localToGlobalPose(DD_SkeletonPose& pose, const DD_Skeleton* sk,
                       bool* visited, const unsigned idx,
                       bool globalflag = false);

DD_Event DD_AnimSystem::update(DD_Event& event) {
  // update all DD_ModelSK animations registered
  // may need to change this to active agents and not active skinned models
  unsigned idx = 0;
  DD_ModelSK* mdlsk = ResSpace::findDD_ModelSK(res_ptr, idx);
  // can be parallelized
  for (unsigned mdl_idx = 0; mdl_idx < res_ptr->sk_mdl_counter; mdl_idx++) {
    mdlsk = &res_ptr->sk_models[mdl_idx];

    DD_Skeleton* sk = ResSpace::findDD_Skeleton(
        res_ptr, mdlsk->m_finalPose.m_skeletonID.str());
    DD_SkeletonPose& skpose = mdlsk->m_finalPose;

    if (sk) {
      // loop thru animation states and calculate final pose matrix
      processAnimState(mdlsk, sk, event.m_time);

      // create flag bin
      bool flag_bin[MAX_JOINTS];
      for (unsigned i = 0; i < sk->m_bones.size(); i++) {
        flag_bin[i] = false;
      }
      // calculate global pose
      for (unsigned i = 0; i < sk->m_bones.size(); i++) {
        localToGlobalPose(skpose, sk, flag_bin, i, mdlsk->m_daltonFlag);
      }
      if (mdlsk->debugStatus()) {  // debug buffer
        mdlsk->m_debugSkeleton = skpose.m_globalPose;
      }
      // finalize for vertex shader
      if (!mdlsk->m_daltonFlag) {
        for (unsigned i = 0; i < sk->m_bones.size(); i++) {
          skpose.m_globalPose[i] =
              skpose.m_globalPose[i] * sk->m_bones[i].m_invBP;
        }
      }
    }
  }
  return DD_Event();
}

void DD_AnimSystem::anim_update(DD_LEvent& _event) {}

void DD_AnimSystem::processAnimState(DD_ModelSK* mdlsk, const DD_Skeleton* sk,
                                     const float time) {
  bool first_pass = true;
  for (unsigned i = 0; i < mdlsk->m_animStates.size(); i++) {
    DD_AnimState& a_state = mdlsk->m_animStates[i];
    if (!a_state.active) {
      continue;
    }  // skip inactive animations

    DD_AnimClip* a_clip =
        ResSpace::findDD_AnimClip(res_ptr, a_state.clip_id.str());
    dd_array<DD_JointPose>& localp = mdlsk->m_finalPose.m_localPose;

    // calculate time
    float anim_speed = a_state.play_back;
    if (a_state.pause) {
      anim_speed = 0.f;
    }  // pause

    a_state.local_time += time * anim_speed;
    if (a_state.local_time >= a_clip->length) {  // loop forwards
      if (a_state.flag_loop) {
        const float x = a_state.local_time / a_clip->length;
        const unsigned x_int = (unsigned)x;
        a_state.local_time = x - (float)x_int;
      } else {
        a_state.local_time = 0.f;
        a_state.active = false;
      }

    } else if (a_state.local_time < 0) {  // loop backwards
      if (a_state.flag_loop) {
        const float x = (a_state.local_time * -1) / a_clip->length;
        const unsigned x_int = (unsigned)x;
        a_state.local_time = a_clip->length - (x - (float)x_int);
      } else {
        a_state.local_time = 0.f;
        a_state.active = false;
      }
    }
    const unsigned idx_a = (unsigned)(a_state.local_time / a_clip->step_size);
    // clamp interpolating frame
    const unsigned idx_b =
        (idx_a + 1) < a_clip->num_frames ? (idx_a + 1) : a_clip->num_frames - 1;
    const float alpha =
        (a_state.local_time - (idx_a * a_clip->step_size)) / a_clip->step_size;

    // set m_final pose
    for (unsigned j = 0; j < localp.size(); j++) {
      glm::quat iden;
      const glm::quat qa = glm::slerp(
          iden, a_clip->samples[idx_a].m_pose[j].m_rot, a_state.weight);
      const glm::quat qb = glm::slerp(
          iden, a_clip->samples[idx_b].m_pose[j].m_rot, a_state.weight);

      const glm::vec3 ta =
          a_clip->samples[idx_a].m_pose[j].m_trans * a_state.weight;
      const glm::vec3 tb =
          a_clip->samples[idx_b].m_pose[j].m_trans * a_state.weight;

      if (first_pass) {  // reset local pose
        if (a_state.interpolate) {
          localp[j].m_rot = glm::slerp(qa, qb, alpha);
          localp[j].m_trans = ta * (1 - alpha) + (tb * alpha);
        } else {
          localp[j].m_rot = qa;
          localp[j].m_trans = ta;
        }
      } else {
        if (a_state.interpolate) {
          localp[j].m_rot = glm::lerp(qa, qb, alpha) * localp[j].m_rot;
          localp[j].m_trans =
              (ta * (1 - alpha) + (tb * alpha)) + localp[j].m_trans;
        } else {
          localp[j].m_rot = qa * localp[j].m_rot;
          localp[j].m_trans = ta + localp[j].m_trans;
        }
      }
    }
    first_pass = false;
  }
}

/// \brief Calculate the global pose matrix from local pose data
void localToGlobalPose(DD_SkeletonPose& pose, const DD_Skeleton* sk,
                       bool* visited, const unsigned idx, bool globalflag) {
  if (!visited[idx]) {
    visited[idx] = true;

    DD_JointPose& jntp = pose.m_localPose[idx];
    const unsigned p_idx = sk->m_bones[idx].m_parent;

    // check parent node before calculating matrix
    if (!visited[p_idx]) {
      localToGlobalPose(pose, sk, visited, p_idx, globalflag);
    }
    // calculate matrix
    glm::mat4 trans = glm::translate(glm::mat4(), jntp.m_trans);
    glm::mat4 rot = glm::mat4_cast(jntp.m_rot);
    // trans = rot = glm::mat4();

    if (idx == 0) {
      pose.m_globalPose[0] = sk->m_globalMat * trans * rot;
    } else {
      if (!globalflag) {
        pose.m_globalPose[idx] =
            pose.m_globalPose[p_idx] * sk->m_bones[idx].m_pDelta * trans * rot;
      } else {
        pose.m_globalPose[idx] = trans * rot;
      }
    }
  }
}
