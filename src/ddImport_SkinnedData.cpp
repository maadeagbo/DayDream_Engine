#include "ddImport_SkinnedData.h"
#include "ddTerminal.h"

ddSkeleton *load_skeleton(const char *ddb_file, const char *id) {
  ddIO io_handle;
  ddSkeleton *skele = nullptr;

  // check if already exists
  size_t hashed_id = getCharHash(id);
  skele = find_ddSkeleton(hashed_id);
  if (skele) {
    ddTerminal::f_post("load_skeleton::<%s> already exists", id);
    return skele;
  }
  // check that path is .ddb
  cbuff<512> path_check(ddb_file);
  if (!path_check.contains(".ddb")) {
    ddTerminal::f_post("[error] Invalid ddb file: %s", ddb_file);
    return skele;
  }

  // parse file
  if (io_handle.open(ddb_file, ddIOflag::READ)) {
    skele = spawn_ddSkeleton(hashed_id);
    cbuff<64> mybuff;
    const char *nxtLine = io_handle.readNextLine();
    while (nxtLine && *nxtLine) {
      mybuff.set(nxtLine);
      // number of bones
      if (mybuff.compare("<size>") == 0) {
        nxtLine = io_handle.readNextLine();
        unsigned size = std::strtoul(nxtLine, nullptr, 10);
        skele->bones.resize(size);
      }
      // global array
      if (mybuff.compare("<global>") == 0) {
        nxtLine = io_handle.readNextLine();
        nxtLine += 2;  // position
        glm::vec3 pos = getVec3f(nxtLine);
        nxtLine = io_handle.readNextLine();
        nxtLine += 2;  // rotation
        glm::vec3 rot = getVec3f(nxtLine);
        nxtLine = io_handle.readNextLine();
        nxtLine += 2;  // scale
        glm::vec3 scale = getVec3f(nxtLine);

        glm::mat4 global_mat = createMatrix(pos, rot, scale);
        skele->global_mat = global_mat;
        // printGlmMat(global_mat);
      }
      // joints
      if (mybuff.compare("<joint>") == 0) {
        nxtLine = io_handle.readNextLine();
        auto tkns = StrSpace::tokenize1024<64>(nxtLine, " ");

        // set id and parent index
        unsigned idx = strtoul(tkns[1].str(), nullptr, 10);
        unsigned parent_idx = strtoul(tkns[2].str(), nullptr, 10);
        skele->bones[idx].id.set(tkns[0].str());
        skele->bones[idx].parent = (uint8_t)parent_idx;

        // get inverse bind pose matrix (object to joint space)
        nxtLine = io_handle.readNextLine();
        nxtLine += 2;  // position
        glm::vec3 pos = getVec3f(nxtLine);
        nxtLine = io_handle.readNextLine();
        nxtLine += 2;  // rotation
        glm::vec3 rot = getVec3f(nxtLine);
        nxtLine = io_handle.readNextLine();
        nxtLine += 2;  // scale
        glm::vec3 scale = getVec3f(nxtLine);

        glm::mat4 jnt_mat = createMatrix(pos, rot, scale);
        skele->bones[idx].inv_bp = glm::inverse(jnt_mat);
      }
      nxtLine = io_handle.readNextLine();
    }
    // calculate delta bone transform of skeleton
    bool v_bin[MAX_JOINTS];
    for (unsigned i = 0; i < skele->bones.size(); i++) {
      v_bin[i] = false;
    }
    for (unsigned i = 0; i < skele->bones.size(); i++) {
      calc_delta_bone_trans(skele, v_bin, i);
    }
  }
  return skele;
}

void calc_delta_bone_trans(ddSkeleton *sk, bool visited[], const unsigned idx) {
  if (!visited[idx]) {
    visited[idx] = true;

    ddJoint &jnt = sk->bones[idx];
    const unsigned p_idx = jnt.parent;
    // check parent bone
    if (!visited[p_idx]) {
      calc_delta_bone_trans(sk, visited, p_idx);
    }
    if (idx == 0) {
      jnt.p_delta = glm::mat4();
    } else {
      jnt.p_delta = sk->bones[p_idx].inv_bp * glm::inverse(jnt.inv_bp);
    }
  }
}

ddAnimClip *load_animation(const char *dda_file, const char *id) {
  ddIO io_handle;
  ddAnimClip *a_clip = nullptr;

  // check if already exists
  size_t hashed_id = getCharHash(id);
  a_clip = find_ddAnimClip(hashed_id);
  if (a_clip) {
    ddTerminal::f_post("load_animation::<%s> already exists", id);
    return a_clip;
  }
  // make sure file is .dda
  cbuff<512> path_check(dda_file);
  if (!path_check.contains(".dda")) {
    ddTerminal::f_post("[error] Not valid dda file: %s", dda_file);
    return a_clip;
  }

  // parse animation from file
  if (io_handle.open(dda_file, ddIOflag::READ)) {
    a_clip = spawn_ddAnimClip(hashed_id);
    cbuff<128> mybuff;
    const char *val = nullptr;

    const char *nxtLine = io_handle.readNextLine();
    while (nxtLine) {
      mybuff.set(nxtLine);

      // framerate of clip
      if (mybuff.compare("<framerate>") == 0) {
        nxtLine = io_handle.readNextLine();
        float fr = std::strtof(nxtLine, nullptr);
        a_clip->fps = fr;
      }

      // buffer sizes
      if (mybuff.compare("<buffer>") == 0) {
        nxtLine = io_handle.readNextLine();

        if (*nxtLine == 'j') {  // total joints
          val = nxtLine + 2;
          unsigned long j = std::strtoul(val, nullptr, 10);
          a_clip->num_joints = (unsigned)j;
          nxtLine = io_handle.readNextLine();
        }
        if (*nxtLine == 'f') {  // total frames
          val = nxtLine + 2;
          unsigned long tf = std::strtoul(val, nullptr, 10);
          a_clip->num_frames = (unsigned)tf;
          a_clip->samples.resize(a_clip->num_frames);

          // calculate clip length
          a_clip->step_size = 1.f / a_clip->fps;
          a_clip->length = (float)a_clip->num_frames / a_clip->fps;
          a_clip->length -= a_clip->step_size;
          for (unsigned i = 0; i < tf; i++) {  // resize storage bin
            a_clip->samples[i].pose.resize(a_clip->num_joints);
          }
          nxtLine = io_handle.readNextLine();
        }
      }

      // animation data
      if (mybuff.compare("<animation>") == 0) {
        nxtLine = io_handle.readNextLine();
        unsigned idx = 0;
        unsigned f_idx = 0;
        dd_array<ddJointPose> output(a_clip->num_frames);

        if (*nxtLine == '-') {  // joint index
          val = nxtLine + 2;
          idx = (unsigned)std::strtoul(val, nullptr, 10);
          nxtLine = io_handle.readNextLine();
        }
        while (*nxtLine != '<') {  // keep reading till </animation>
          if (*nxtLine == 'r') {   // joint rotation
            val = nxtLine + 2;
            // printf("r %u:%u %s\n", f_idx, idx, val);
            a_clip->samples[f_idx].pose[idx].rot = getQuat(val);
            f_idx += 1;  // increment frame index
          }
          if (*nxtLine == 'p') {  // joint translation
            val = nxtLine + 2;
            // printf("t %u:%u %s\n", f_idx, idx, val);
            a_clip->samples[f_idx - 1].pose[idx].trans = getVec3f(val);
          }
          nxtLine = io_handle.readNextLine();
        }
      }
      nxtLine = io_handle.readNextLine();
    }
  }

  return a_clip;
}
