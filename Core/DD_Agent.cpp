#include "DD_Agent.h"

/**
        \brief Agent Constructor
        \param ID Searchable descriptor for agent
        \param model Model Model ID that can be set at LOD level 0
        \param parent Searchable Descriptor to set parent agent (scene graph)
*/
DD_Agent::DD_Agent(const char* ID, const char* model, const char* parent)
    : num_handlers(0),
      tickets(5),
      handlers(5),
      inst_m4x4(1),
      f_inst_m4x4(1),
      inst_colors(1),
      f_inst_colors(1),
      flag_cull_inst(1),
      flag_color(false),
      flag_model(false),
      flag_modelsk(false),
      flag_render(true),
      _position(glm::vec3(0.0f)),
      _size(glm::vec3(1.0f)),
      _rot_q(glm::quat()),
      _flag_parent(false),
      _flag_inst(false),
      _default_update(true) {
  m_ID = ID;
  std::string model_ID = model;
  if (model_ID != "") {
    AddModel(model, 0.f, 10000.f);
  }
  _parent_id = parent;
}

/**
        \brief Sets material of m_mesh[0] only if m_material container is empty
*/
void DD_Agent::SetMaterial(const size_t index) {
  if (mat_buffer.size() == 0) {
    mat_buffer.resize(1);
    mat_buffer[0] = index;
  } else {
    mat_buffer[0] = index;
  }
}

/**
        \brief Load internal instance array
*/
void DD_Agent::SetInstances(dd_array<glm::mat4>& matrices) {
  inst_m4x4.resize(matrices.size());
  f_inst_m4x4.resize(matrices.size());
  flag_cull_inst.resize(matrices.size());
  inst_m4x4 = std::move(matrices);
  _flag_inst = true;
}

void DD_Agent::SetInstances(const dd_array<glm::mat4>& matrices) {
  inst_m4x4.resize(matrices.size());
  f_inst_m4x4.resize(matrices.size());
  flag_cull_inst.resize(matrices.size());
  inst_m4x4 = matrices;
  _flag_inst = true;
}

/**
        \brief Load internal color instance array
*/
void DD_Agent::SetColorInstances(dd_array<glm::vec3>& vects) {
  inst_colors.resize(vects.size());
  f_inst_colors.resize(vects.size());
  flag_color = true;
  inst_colors = std::move(vects);
}

/**
        \brief Appends new callback method and ticket for handler classification
*/
void DD_Agent::AddCallback(const char* ticket, EventHandler handler) {
  // resize if too small
  if (num_handlers >= handlers.size()) {
    // functions
    dd_array<EventHandler> temp(num_handlers + 5);
    temp = std::move(handlers);
    handlers.resize(temp.size());
    handlers = std::move(temp);
    // tickets
    dd_array<std::string> temp2(num_handlers + 5);
    temp2 = std::move(tickets);
    tickets.resize(temp2.size());
    tickets = std::move(temp2);
  }
  handlers[num_handlers] = handler;
  tickets[num_handlers] = ticket;
  num_handlers += 1;
}

/**
        \brief Appends new model/mesh to Agent mesh container
*/
void DD_Agent::AddModel(const char* model_ID, const float _near,
                        const float _far) {
  if (mesh_buffer.size() == 0) {
    mesh_buffer.resize(1);
    mesh_buffer[0].farLOD = _far;
    mesh_buffer[0].nearLOD = _near;
    mesh_buffer[0].model = model_ID;

    flag_model = true;
  } else {
    size_t pre_size = mesh_buffer.size();
    dd_array<ModelLOD> temp = dd_array<ModelLOD>(pre_size + 1);
    temp = mesh_buffer;
    temp[pre_size].farLOD = _far;
    temp[pre_size].nearLOD = _near;
    temp[pre_size].model = model_ID;

    temp[pre_size - 1].farLOD = _near - 0.0001f;  // set previous LOD

    mesh_buffer = std::move(temp);
  }
}

/**
        \brief Appends new model/mesh to Agent mesh container
*/
void DD_Agent::AddModelSK(const char* model_ID, const float _near,
                          const float _far) {
  if (mesh_buffer.size() == 0) {
    mesh_buffer.resize(1);
    mesh_buffer[0].farLOD = _far;
    mesh_buffer[0].nearLOD = _near;
    mesh_buffer[0].model = model_ID;

    flag_modelsk = true;
  } else {
    size_t pre_size = mesh_buffer.size();
    dd_array<ModelLOD> temp = dd_array<ModelLOD>(pre_size + 1);
    temp = mesh_buffer;
    temp[pre_size].farLOD = _far;
    temp[pre_size].nearLOD = _near;
    temp[pre_size].model = model_ID;

    temp[pre_size - 1].farLOD = _near - 0.0001f;  // set previous LOD

    mesh_buffer = std::move(temp);
  }
}

glm::vec3 DD_Agent::ForwardDir() {
  glm::vec4 forward = rot() * glm::vec4(0.0f, 0.0f, -1.0f, 1.0f);
  return glm::normalize(glm::vec3(forward));
}

glm::vec3 DD_Agent::RightDir() {
  glm::vec3 forward = ForwardDir();
  glm::vec3 right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));
  return right;
}

glm::vec3 DD_Agent::UpDir() {
  glm::vec4 up = rot() * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
  return glm::vec3(up);
}

/**
        \brief Traditionally updates inst_m4x4[0] w/ RST matrix multiply order
*/
void DD_Agent::cleanInst() {
  if (!multiInst() && _default_update) {
    glm::mat4 transM = glm::translate(glm::mat4(), pos());
    transM = transM * glm::mat4_cast(rot());
    transM = glm::scale(transM, size());
    inst_m4x4[0] = transM;
  }
}
