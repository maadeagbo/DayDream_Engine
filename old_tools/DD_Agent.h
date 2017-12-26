#pragma once

/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

#include "DD_Input.h"
#include "DD_Model.h"
#include "DD_ModelSK.h"
#include "DD_ParticleTypes.h"
#include "DD_Types.h"

/**
        Tracks near and far distance clip range for identified model
*/
struct ModelLOD {
  float nearLOD = 0.f, farLOD = 0.f;
  std::string model = "";
};

/**
        Represents agents w/ in engine. Implements handler interface methods for
        callback functionality. Has structures and function for setting
   instances,
        modifying scene graph, and updating mesh information
*/
class DD_Agent {
 public:
  DD_Agent(const char* ID = "", const char* model = "",
           const char* parent = "");
  // dtor
  ~DD_Agent() {}
  DD_Agent(const DD_Agent& other) = default;
  DD_Agent(DD_Agent&& other) = default;
  DD_Agent& operator=(DD_Agent& other) = default;
  DD_Agent& operator=(DD_Agent&& other) = default;

  /** \brief Sets parent and triggers flags for updating scene graph */
  inline void SetParent(const char* parentID) {
    _parent_id = parentID;
    _parent_idx = -1;
    _flag_parent = true;
  }
  /** \brief Unsets parent and triggers flags for updating scene graph */
  inline void unParent() {
    _parent_id = "";
    _parent_idx = -1;
    _flag_parent = false;
  }
  inline void SetParentIndex(const int index) { _parent_idx = index; }

  void SetMaterial(const size_t index);
  void SetInstances(dd_array<glm::mat4>& matrices);
  void SetInstances(const dd_array<glm::mat4>& matrices);
  void SetColorInstances(dd_array<glm::vec3>& vects);
  void AddCallback(const char* ticket, EventHandler handler);
  void AddModel(const char* model_ID, const float _near, const float _far);
  void AddModelSK(const char* model_ID, const float _near, const float _far);

  glm::vec3 ForwardDir();
  glm::vec3 RightDir();
  glm::vec3 UpDir();
  inline void UpdatePosition(const glm::vec3& pos) { _position = pos; }
  inline void UpdateScale(const glm::vec3& scale) { _size = scale; }
  inline void UpdateRotation(const glm::quat& rot) {
    _rot_q = glm::normalize(rot);
  }
  /** \brief Overrides default instance update ( RST--> final transform )*/
  inline void override_inst_update() { _default_update = false; }
  inline void default_inst_update() { _default_update = true; }
  inline bool multiInst() { return _flag_inst; }
  inline glm::vec3 pos() const { return _position; }
  inline glm::vec3 size() const { return _size; }
  inline glm::quat rot() const { return _rot_q; }
  inline int parentIndex() const { return _parent_idx; }
  inline std::string parentID() const { return _parent_id; }
  inline bool isChild() const { return _flag_parent; }
  inline void frameFlag(const GLfloat millisec) { _flag_ftime = millisec; }
  inline float checkFrame() const { return (float)_flag_ftime; }
  void cleanInst();

  size_t num_handlers;
  dd_array<std::string> tickets;   /**< describes the handler method type */
  dd_array<EventHandler> handlers; /**< handles for callback methods */
  glm::mat4 parent_transform;      /**< updated per frame (recuresively) */
  BoundingBox BBox;                /**< set if LOD 0 mesh exists */

  size_t inst_sink_size;             /**< reset per frame (cpu culled meshes) */
  dd_array<glm::mat4> inst_m4x4;     /**< full number of instance matrices */
  dd_array<glm::mat4> f_inst_m4x4;   /**< reset per frame (cpu culled meshes) */
  dd_array<glm::vec3> inst_colors;   /**< full number of color matrices */
  dd_array<glm::vec3> f_inst_colors; /**< reset per frame (cpu culled meshes) */
  dd_array<size_t> mat_buffer;       /**< indices for material */
  dd_array<ModelLOD> mesh_buffer; /**< model information (contains LOD info) */
  dd_array<GLint> flag_cull_inst; /**< flag set to index into mesh_buffer */
  std::string m_ID;

  GLboolean flag_color; /**< flag set to show if color instances are present */
  GLboolean flag_model; /**< flag set to show if meshes are present */
  GLboolean flag_modelsk; /**< flag set to show if skinned meshes are present */
  GLboolean flag_render;  /**< True = render, false = no render */
 private:
  glm::vec3 _position, _size;
  glm::quat _rot_q;
  std::string _parent_id;
  int _parent_idx;
  GLfloat _flag_ftime;
  GLboolean _flag_parent, _flag_inst, _default_update;
};
