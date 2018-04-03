#include "ddAssets.h"

// ddAgents
ASSET_CREATE(ddAgent, b_agents, ASSETS_CONTAINER_MAX_SIZE)
dd_array<size_t> culled_agents(ASSETS_CONTAINER_MAX_SIZE);
// ddCam
ASSET_CREATE(ddCam, cams, ASSETS_CONTAINER_MIN_SIZE)
// ddLBulb
ASSET_CREATE(ddLBulb, lights, ASSETS_CONTAINER_MIN_SIZE)
// ddModelData
ASSET_CREATE(ddModelData, meshes, ASSETS_CONTAINER_MAX_SIZE)
// ddSkeleton
ASSET_CREATE(ddSkeleton, skeletons, ASSETS_CONTAINER_MIN_SIZE)
// ddSkeletonPose
ASSET_CREATE(ddSkeletonPose, poses, ASSETS_CONTAINER_MAX_SIZE)
// ddTex2D
ASSET_CREATE(ddTex2D, textures, ASSETS_CONTAINER_MAX_SIZE)
// ddMat
ASSET_CREATE(ddMat, mats, ASSETS_CONTAINER_MAX_SIZE)
// ddAnimClip
ASSET_CREATE(ddAnimClip, clips, ASSETS_CONTAINER_MAX_SIZE)

ASSET_DEF(ddAgent, b_agents)
ASSET_DEF(ddCam, cams)
ASSET_DEF(ddLBulb, lights)
ASSET_DEF(ddModelData, meshes)
ASSET_DEF(ddSkeleton, skeletons)
ASSET_DEF(ddSkeletonPose, poses)
ASSET_DEF(ddTex2D, textures)
ASSET_DEF(ddMat, mats)
ASSET_DEF(ddAnimClip, clips)

void init_assets() {
  // set free lists
  fl_b_agents.initialize((unsigned)b_agents.size());
  fl_cams.initialize((unsigned)cams.size());
  fl_lights.initialize((unsigned)lights.size());
  fl_meshes.initialize((unsigned)meshes.size());
  fl_skeletons.initialize((unsigned)skeletons.size());
  fl_poses.initialize((unsigned)poses.size());
  fl_textures.initialize((unsigned)textures.size());
  fl_mats.initialize((unsigned)mats.size());
  fl_clips.initialize((unsigned)clips.size());
}

void cleanup_all_assets() {
  // cleanup agents
  for (auto &idx : map_b_agents) {
    // vao's
    DD_FOREACH(ModelIDs, mdl_id, b_agents[idx.second].mesh) {
      DD_FOREACH(ddVAOData *, vao, mdl_id.ptr->vao_handles) {
        ddGPUFrontEnd::destroy_vao(*vao.ptr);
      }
      // resize container
      mdl_id.ptr->vao_handles.resize(0);
    }
    // instance buffers
    ddGPUFrontEnd::destroy_instance_data(b_agents[idx.second].inst.inst_buff);
    b_agents[idx.second].inst.inst_buff = nullptr;
  }
  // cleanup meshes
  for (auto &idx : map_meshes) {
    // mesh buffers
    DD_FOREACH(ddMeshBufferData *, mdl, meshes[idx.second].buffers) {
      ddGPUFrontEnd::destroy_buffer_data(*mdl.ptr);
    }
    // resize to 0
    meshes[idx.second].buffers.resize(0);
  }
}
