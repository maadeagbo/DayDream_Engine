#pragma once

#include "LuaHooks.h"
#include "ddAssets.h"
#include "ddIncludes.h"

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
 * \brief Cull objects outside of camera frustum
 * \param _agents must be allocated to size ASSETS_CONTAINER_MAX_SIZE
 */
void cull_objects(const FrustumBox fr, const glm::mat4 view_m,
                  dd_array<ddAgent*>& _agents);

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
 * \brief Get camera's forward direction
 */
void update_scene_graph();

/**
 * \brief Check if ray intersects agent
 */

bool ray_bbox_intersect(const glm::vec3 origin, const glm::vec3 dir,
                        const size_t ag_id);
}  // namespace ddSceneManager
