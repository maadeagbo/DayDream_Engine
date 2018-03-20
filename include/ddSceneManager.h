#pragma once

#include "LuaHooks.h"
#include "ddAssets.h"
#include "ddIncludes.h"

/** \brief List off agents visible from a selected camera's frustum */
struct ddVisList {
  size_t cam_id = 0;
  dd_array<ddAgent*> visible_agents;
  dd_array<float> sq_dist;
};

/** \brief Tools for retrieving assets and scene information */
namespace ddSceneManager {
/** \brief Initialize */
void initialize(const unsigned width, const unsigned height);

/** \brief Get screen size information */
glm::uvec2 get_screen_dimensions();

/**
 * \brief Get currently active camera (selects to 1st active camera it finds)
 */
ddCam* get_active_cam();

/**
 * \brief Calculate camera view matrix
 */
glm::mat4 calc_view_matrix(const ddCam* cam);

/**
 * \brief Calculate perspective projection matrix
 */
glm::mat4 calc_p_proj_matrix(const ddCam* cam);

/**
 * \brief Calculate camera frustum
 */
FrustumBox get_current_frustum(const ddCam* cam);

/**
 * \brief Calculate light volume radius
 */
float calc_lightvolume_radius(const ddLBulb* blb);

/**
 * \brief Get all active lights in scene
 * \param _lights must be allocated to size ASSETS_CONTAINER_MIN_SIZE
 */
void get_active_lights(dd_array<ddLBulb*>& _lights);

/**
 * \brief Get currently active directional light
 */
ddLBulb* get_shadow_light();

/**
 * \brief Get camera's forward direction
 */
glm::vec3 cam_forward_dir(const ddCam* cam, const ddBody* cam_parent_body);

/**
 * \brief Update positions of constraints in bullet physics
 */
void update_scene_graph();

/**
 * \brief Create list of visible objects in the scene w/ dist from camera
 */
bool reload_visibility_list(const size_t cam_id, const glm::vec3& cam_pos,
                            const FrustumBox& fr);

/**
 * \brief Returns list of visible objects in the scene
 */
const ddVisList* get_visibility_list(const size_t cam_id);

/**
 * \brief Check if ray intersects agent
 */

bool ray_bbox_intersect(const glm::vec3 origin, const glm::vec3 dir,
                        const size_t ag_id);
}  // namespace ddSceneManager
