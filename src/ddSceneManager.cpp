#include "ddSceneManager.h"
#include "ddTerminal.h"

namespace {
unsigned native_scr_width = 0;
unsigned native_scr_height = 0;

// visibility list
dd_array<ddVisList> visibility_lists;

DD_FuncBuff fb;
}  // namespace

void ddSceneManager::initialize(const unsigned width, const unsigned height) {
  native_scr_width = width;
  native_scr_height = height;
}

glm::uvec2 ddSceneManager::get_screen_dimensions() {
  return glm::uvec2(native_scr_width, native_scr_height);
}

ddCam *ddSceneManager::get_active_cam() {
  dd_array<ddCam *> cam_array = get_all_ddCam();
  DD_FOREACH(ddCam *, cam_id, cam_array) {
    ddCam *cam = *cam_id.ptr;
    if (cam->active) return cam;
  }
  return nullptr;
}

glm::mat4 ddSceneManager::calc_view_matrix(const ddCam *cam) {
  if (!cam) {
    ddTerminal::f_post("[error] calc_view_matrix::Camera is null");
    return glm::mat4();
  }
  // get parent object
  ddAgent *parent_ag = find_ddAgent(cam->parent);
  if (!parent_ag) {
    ddTerminal::f_post("[error] calc_view_matrix::Parent agent not found <%u>",
                       cam->parent);
    return glm::mat4();
  }
  ddBody *bod = &parent_ag->body;

  // camera world position
  const glm::vec3 pos = ddBodyFuncs::pos_ws(bod);

  // camera front direction
  const glm::vec3 front = ddSceneManager::cam_forward_dir(cam, bod);

  // camera up direction
  glm::vec3 right = glm::normalize(glm::cross(front, global_Yv3));
  glm::vec3 up = glm::normalize(glm::cross(right, front));

  return glm::lookAt(pos, pos + front, up);
}

glm::mat4 ddSceneManager::calc_p_proj_matrix(const ddCam *cam) {
  if (!cam) {
    ddTerminal::f_post("[error] calc_p_proj_matrix::Camera is null");
    return glm::mat4();
  }
  return glm::perspectiveFov(cam->fovh, (float)cam->width, (float)cam->height,
                             cam->n_plane, cam->f_plane);
}

FrustumBox ddSceneManager::get_current_frustum(const ddCam *cam) {
  FrustumBox frustum;
  if (!cam) {
    ddTerminal::f_post("[error] get_current_frustum::Camera is null");
    return frustum;
  }
  // get parent object
  ddAgent *parent_ag = find_ddAgent(cam->parent);
  if (!parent_ag) {
    ddTerminal::f_post(
        "[error] get_current_frustum::Parent agent not found <%u>",
        cam->parent);
    return frustum;
  }
  ddBody *bod = &parent_ag->body;

  // recalculate front, right, and up for frustum calculation
  const glm::vec3 cam_pos = ddBodyFuncs::pos_ws(bod);
  const glm::vec3 front = ddSceneManager::cam_forward_dir(cam, bod);
  const glm::vec3 right = glm::normalize(glm::cross(front, global_Yv3));
  const glm::vec3 up = glm::normalize(glm::cross(right, front));

  // calculate new frustum
  glm::vec3 n_center = glm::vec3(glm::vec3(cam_pos) + front * cam->n_plane);
  glm::vec3 f_center = glm::vec3(glm::vec3(cam_pos) + front * cam->f_plane);
  float tang = tan(cam->fovh / 2);
  float ratio = ((float)cam->height / (float)cam->width);
  float w_near = tang * cam->n_plane * 2.0f;
  float h_near = w_near * ratio;
  float w_far = tang * cam->f_plane * 2.0f;
  float h_far = w_far * ratio;

  // corners
  frustum.corners[0] = n_center + (up * h_near) + (right * w_near);
  frustum.corners[1] = n_center - (up * h_near) + (right * w_near);
  frustum.corners[2] = n_center + (up * h_near) - (right * w_near);
  frustum.corners[3] = n_center - (up * h_near) - (right * w_near);
  frustum.corners[4] = f_center + (up * h_far) + (right * w_far);
  frustum.corners[5] = f_center - (up * h_far) + (right * w_far);
  frustum.corners[6] = f_center + (up * h_far) - (right * w_far);
  frustum.corners[7] = f_center - (up * h_far) - (right * w_far);

  // frustum top
  glm::vec3 top_pos = n_center + (up * h_near);
  glm::vec3 temp_vec = glm::normalize(top_pos - glm::vec3(cam_pos));
  glm::vec3 up_norm = glm::cross(right, temp_vec);

  // frustum bottom
  glm::vec3 bot_pos = n_center - (up * h_near);
  temp_vec = glm::normalize(bot_pos - glm::vec3(cam_pos));
  glm::vec3 down_norm = glm::cross(temp_vec, right);

  // frustum left
  glm::vec3 left_pos = n_center - (right * w_near);
  temp_vec = glm::normalize(left_pos - glm::vec3(cam_pos));
  glm::vec3 left_norm = glm::cross(up, temp_vec);

  // frustum right normal ---> (nc + left * w_near / 2) - p
  glm::vec3 right_pos = n_center + (right * w_near);
  temp_vec = glm::normalize(right_pos - glm::vec3(cam_pos));
  glm::vec3 right_norm = glm::cross(temp_vec, up);

  // points
  frustum.points[0] = n_center;
  frustum.points[1] = f_center;
  frustum.points[2] = right_pos;
  frustum.points[3] = left_pos;
  frustum.points[4] = top_pos;
  frustum.points[5] = bot_pos;

  // normals
  frustum.normals[0] = -front;
  frustum.normals[1] = front;
  frustum.normals[2] = right_norm;
  frustum.normals[3] = left_norm;
  frustum.normals[4] = up_norm;
  frustum.normals[5] = down_norm;

  // D
  frustum.d[0] = -glm::dot(frustum.normals[0], frustum.points[0]);
  frustum.d[1] = -glm::dot(frustum.normals[1], frustum.points[1]);
  frustum.d[2] = -glm::dot(frustum.normals[2], frustum.points[2]);
  frustum.d[3] = -glm::dot(frustum.normals[3], frustum.points[3]);
  frustum.d[4] = -glm::dot(frustum.normals[4], frustum.points[4]);
  frustum.d[5] = -glm::dot(frustum.normals[5], frustum.points[5]);

  return frustum;
}

float ddSceneManager::calc_lightvolume_radius(const ddLBulb *blb) {
  if (!blb) {
    ddTerminal::f_post("[error] calc_lightvolume_radius::Light is null");
    return 0.f;
  }

  const float radius = 1.0f;
  const float min_lumin = 0.01f;  // may need to be scaled inversely by exposure

  // luminance using Rec 709 luminance formula
  double light_lumin = glm::dot(blb->color, glm::vec3(0.2126, 0.7152, 0.0722));
  const double cutoff = min_lumin / light_lumin;

  // double kc = constant - (light_lumin / min_lumin);
  // 1.0/(kc + blb->linear * blb->linear + (blb->quadratic * dist^2))

  // max value set as light intensity (approximation subject to change)
  float light_max =
      std::fmaxf(std::fmaxf(blb->color.r, blb->color.g), blb->color.b);
  return radius * (glm::sqrt(light_max / cutoff) - 1.0);
}

void ddSceneManager::get_active_lights(dd_array<ddLBulb *> &_lights) {
  POW2_VERIFY(_lights.size() == ASSETS_CONTAINER_MIN_SIZE);

  // null the array
  DD_FOREACH(ddLBulb *, blb, _lights) { *blb.ptr = nullptr; }

  unsigned blb_tracker = 0;
  dd_array<ddLBulb *> l_array = get_all_ddLBulb();
  DD_FOREACH(ddLBulb *, l_id, l_array) {
    ddLBulb *blb = *l_id.ptr;

    if (blb->active) {
      _lights[blb_tracker] = blb;
      blb_tracker++;
    }
  }
}

ddLBulb *ddSceneManager::get_shadow_light() {
  dd_array<ddLBulb *> l_array = get_all_ddLBulb();
  DD_FOREACH(ddLBulb *, l_id, l_array) {
    ddLBulb *blb = *l_id.ptr;

    if (blb->active && blb->shadow) {
      return blb;
    }
  }
  return nullptr;
}

glm::vec3 ddSceneManager::cam_forward_dir(const ddCam *cam,
                                          const ddBody *cam_parent_body) {
  glm::quat cam_internal_rot =
      glm::quat(glm::vec3(glm::radians(cam->pitch), glm::radians(cam->yaw),
                          glm::radians(cam->roll)));
  btTransform tr = cam_parent_body->bt_bod->getWorldTransform();
  glm::mat4 body_rot;
  tr.getBasis().getOpenGLSubMatrix(&body_rot[0][0]);
  body_rot *= glm::mat4_cast(cam_internal_rot);
  glm::vec4 _f = body_rot * glm::vec4(world_front, 1.f);

  return glm::normalize(glm::vec3(_f));
}

void ddSceneManager::update_scene_graph() {
  // loop through agents & update constraints
  dd_array<ddAgent *> ag_array = get_all_ddAgent();
  DD_FOREACH(ddAgent *, ag_id, ag_array) {
    ddAgent *top_ag = *ag_id.ptr;
    if (top_ag->body.bt_constraint) {
      // get parent & update constraints
      ddAgent *ag = find_ddAgent(top_ag->body.parent);

      // create tranformation for kinematic object
      btTransform og_tr = ag->body.bt_bod->getWorldTransform();
      btVector3 offset(top_ag->body.offset.x, top_ag->body.offset.y,
                       top_ag->body.offset.z);

      btTransform tr;
      tr.setIdentity();
      tr.setOrigin(offset);  // apply offset vector
      tr = og_tr * tr;

      top_ag->body.bt_bod->activate(true);
      top_ag->body.bt_bod->setWorldTransform(tr);
    }
  }
}

/** \brief Get min corner of AAABB based on frustum face normal */
glm::vec3 get_min_corner(ddBodyFuncs::AABB &bbox, const glm::vec3 &normal) {
  glm::vec3 new_min = bbox.max;
  if (normal.x >= 0) {
    new_min.x = bbox.min.x;
  }
  if (normal.y >= 0) {
    new_min.y = bbox.min.y;
  }
  if (normal.z >= 0) {
    new_min.z = bbox.min.z;
  }
  return new_min;
}

/** \brief Get max corner of AAABB based on frustum face normal */
glm::vec3 get_max_corner(ddBodyFuncs::AABB &bbox, const glm::vec3 &normal) {
  glm::vec3 new_max = bbox.min;
  if (normal.x >= 0) {
    new_max.x = bbox.max.x;
  }
  if (normal.y >= 0) {
    new_max.y = bbox.max.y;
  }
  if (normal.z >= 0) {
    new_max.z = bbox.max.z;
  }
  return new_max;
}

/** \brief Frustum cull function */
bool frustum_cull(ddBodyFuncs::AABB &bbox, const FrustumBox &fr) {
  for (unsigned i = 0; i < 6; i++) {
    glm::vec3 fr_norm = fr.normals[i];
    float fr_dist = fr.d[i];
    // check if negative vertex is outside (depends on normal of the plane)
    glm::vec3 min_vert = get_min_corner(bbox, fr_norm);
    // if _dist is positive, point is located behind frustum plane (reject)
    float _dist = glm::dot(fr_norm, min_vert) + fr_dist;
    if (_dist > 0.000001f) {
      return false;  // must not fail any plane test
    }
  }
  return true;
}

bool ddSceneManager::reload_visibility_list(const size_t cam_id,
                                            const glm::vec3 &cam_pos,
                                            const FrustumBox &fr) {
  // check if camera already exists in list
  ddVisList *vlist = nullptr;
  DD_FOREACH(ddVisList, item, visibility_lists) {
    if (item.ptr->cam_id == cam_id) vlist = item.ptr;
  }

  // create new entry if necessary
  if (!vlist) {
    dd_array<ddVisList> temp(visibility_lists.size() + 1);
    temp = visibility_lists;
    visibility_lists = std::move(temp);
    vlist = &visibility_lists[visibility_lists.size() - 1];
    vlist->cam_id = cam_id;
  }

  if (vlist) {
    dd_array<ddAgent *> ag_array = get_all_ddAgent();

    // resize lists (dist from cam & pointers) for worst case
    vlist->sq_dist.resize(ag_array.size());
    vlist->visible_agents.resize(ag_array.size());

    // perform frustum check and calculate squared distanced from camera
    unsigned ag_tracker = 0;
    DD_FOREACH(ddAgent *, ag_id, ag_array) {
      ddAgent *ag = *ag_id.ptr;

      // check if agent has mesh
      if (ag->mesh.size() > 0 && ag->body.bt_bod) {
        // get agent position (center of bounding box)
        ddBodyFuncs::AABB bbox = ddBodyFuncs::get_aabb(&ag->body);
        glm::vec3 ag_pos = (bbox.max + bbox.min) / 2.f;

        if (frustum_cull(bbox, fr)) {
          // distance vector
          glm::vec3 dist_vec = cam_pos - ag_pos;

          // add agent to current list
          vlist->visible_agents[ag_tracker] = ag;
          vlist->sq_dist[ag_tracker] = glm::dot(dist_vec, dist_vec);
          ag_tracker++;
        }
      } else {  // agents w/out models get automatic pass

        vlist->visible_agents[ag_tracker] = ag;
        vlist->sq_dist[ag_tracker] = 0.f;
        ag_tracker++;
      }
    }

    // resize lists (dist from cam & pointers) to be compact
    dd_array<ddAgent *> temp_1(ag_tracker);
    temp_1 = vlist->visible_agents;
    dd_array<float> temp_2(ag_tracker);
    temp_2 = vlist->sq_dist;

    vlist->visible_agents = std::move(temp_1);
    vlist->sq_dist = std::move(temp_2);

    return true;
  }
  return false;
}

const ddVisList *ddSceneManager::get_visibility_list(const size_t cam_id) {
  // check if camera has had a visibilty list calculated
  DD_FOREACH(ddVisList, item, visibility_lists) {
    if (item.ptr->cam_id == cam_id) return item.ptr;
  }

  return nullptr;
}

bool ddSceneManager::ray_bbox_intersect(const glm::vec3 origin,
                                        const glm::vec3 dir,
                                        const size_t ag_id) {
  ddAgent *ag = find_ddAgent(ag_id);

  if (ag) {
    // get bounding box
    ddBodyFuncs::AABB bbox = ddBodyFuncs::get_aabb(&ag->body);

    const double epsilon = 0.0000000001f;

    // uses slab method of intersection (Kay and Kayjia siggraph)
    // furthest near t vs closest far t (t = time to intersect)
    double t_min = std::numeric_limits<double>::lowest();
    double t_max = std::numeric_limits<double>::max();

    // vectors from origin of ray to bbox corners
    const glm::vec3 min_d = bbox.min - origin;
    const glm::vec3 max_d = bbox.max - origin;

    // inverse of direction and abs direction
    const glm::vec3 inv_dir = 1.f / dir;
    const glm::vec3 abs_dir = glm::abs(dir);

    // check x (intersection of x-component)
    if (abs_dir.x > epsilon) {
      const double t1 = (min_d.x) * inv_dir.x;
      const double t2 = (max_d.x) * inv_dir.x;

      t_min = std::max(t_min, std::min(t1, t2));
      t_max = std::min(t_max, std::max(t1, t2));
    }
    // check y (intersection of y-component)
    if (abs_dir.y > epsilon) {
      const double t1 = (min_d.y) * inv_dir.y;
      const double t2 = (max_d.y) * inv_dir.y;

      t_min = std::max(t_min, std::min(t1, t2));
      t_max = std::min(t_max, std::max(t1, t2));
    }
    // check z (intersection of z-component)
    if (abs_dir.z > epsilon) {
      const double t1 = (min_d.z) * inv_dir.z;
      const double t2 = (max_d.z) * inv_dir.z;

      t_min = std::max(t_min, std::min(t1, t2));
      t_max = std::min(t_max, std::max(t1, t2));
    }

    return t_max >= t_min && t_max > 0.0;
  }
  return false;
}
