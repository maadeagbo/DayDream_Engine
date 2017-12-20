#include "DD_AssetManager.h"

namespace {
// containers only exist in this translation unit
btDiscreteDynamicsWorld *p_world = nullptr;

// DD_BaseAgents
std::map<size_t, unsigned>	map_b_agents;
dd_array<bool>							freelist_b_agents(ASSETS_CONTAINER_MAX_SIZE);
dd_array<DD_BaseAgent>			b_agents(ASSETS_CONTAINER_MAX_SIZE);
}

namespace DD_Assets {
void initialize(btDiscreteDynamicsWorld *physics_world) {
  p_world = physics_world;

	// set free lists
	for (unsigned i = 0; i < freelist_b_agents.size(); i++) {
		freelist_b_agents[i] = true;
	}
}

ASSET_DEF(DD_BaseAgent, b_agents)
}  // namespace DD_Asset
