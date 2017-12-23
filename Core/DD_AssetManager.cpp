#include "DD_AssetManager.h"

namespace {
// function callback buffer
DD_FuncBuff fb;
// containers only exist in this translation unit
btDiscreteDynamicsWorld *p_world = nullptr;

// DD_BaseAgents
ASSET_CREATE(DD_BaseAgent, b_agents, ASSETS_CONTAINER_MAX_SIZE)
// DD_Cam
ASSET_CREATE(DD_Cam, cams, ASSETS_CONTAINER_MIN_SIZE)
// DD_Bulb
ASSET_CREATE(DD_Bulb, lights, ASSETS_CONTAINER_MIN_SIZE)
// DD_MeshData
ASSET_CREATE(DD_MeshData, meshes, ASSETS_CONTAINER_MAX_SIZE)
// DD_Skeleton
ASSET_CREATE(DD_Skeleton, skeletons, ASSETS_CONTAINER_MIN_SIZE)
// DD_SkeletonPose
ASSET_CREATE(DD_SkeletonPose, poses, ASSETS_CONTAINER_MAX_SIZE)
// DD_Tex2D
ASSET_CREATE(DD_Tex2D, textures, ASSETS_CONTAINER_MAX_SIZE)
}  // namespace

/// \brief Parse ddm file and load to ram
/// \param filename Path to .ddm file
/// \return MeshData of input ddm file
dd_array<MeshData> load_ddm(const char *filename);

namespace DD_Assets {
void initialize(btDiscreteDynamicsWorld *physics_world) {
  p_world = physics_world;
  // set free lists
  fl_b_agents.initialize(b_agents.size());
  fl_cams.initialize(cams.size());
  fl_lights.initialize(lights.size());
  fl_meshes.initialize(meshes.size());
  fl_skeletons.initialize(skeletons.size());
  fl_poses.initialize(poses.size());
  fl_textures.initialize(textures.size());
}

int create_agent(lua_State *L) {
  parse_lua_events(L, fb);
  // get arguments and use them to create DD_BaseAgent
  return 0;
}

int create_mesh(lua_State *L) {
  parse_lua_events(L, fb);
  // get arguments and use them to create DD_MeshData
  return 0;
}

int create_cam(lua_State *L) {
  parse_lua_events(L, fb);
  // get arguments and use them to create DD_Cam
  return 0;
}

int create_light(lua_State *L) {
  parse_lua_events(L, fb);
  // get arguments and use them to create DD_Bulb
  return 0;
}

ASSET_DEF(DD_BaseAgent, b_agents)
ASSET_DEF(DD_Cam, cams)
ASSET_DEF(DD_Bulb, lights)
ASSET_DEF(DD_MeshData, meshes)
ASSET_DEF(DD_Skeleton, skeletons)
ASSET_DEF(DD_SkeletonPose, poses)
ASSET_DEF(DD_Tex2D, textures)
}  // namespace DD_Assets

dd_array<MeshData> load_ddm(const char *filename) {
  return dd_array<MeshData>();
}
