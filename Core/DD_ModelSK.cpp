#include "DD_ModelSK.h"

/// \brief Setup debug buffers for analyzing skeleton output
void DD_ModelSK::debugOn() {
  m_flagDebug = true;
  m_debugSkeleton.resize(m_finalPose.m_globalPose.size());
}

/// \brief Destroy debug buffers for analyzing skeleton output
void DD_ModelSK::debugOff() {
  m_flagDebug = false;
  m_debugSkeleton.resize(0);
}

namespace ModelSKSpace {

GLenum err;

BoundingBox CalculateBBox(const DD_ModelSK& model) {
  BoundingBox bbox;

  bool firstMesh = true;
  for (size_t i = 0; i < model.meshes.size(); i++) {
    if (firstMesh) {
      bbox.min =
          glm::vec3(model.meshes[i].bbox_min.x(), model.meshes[i].bbox_min.y(),
                    model.meshes[i].bbox_min.z());
      bbox.max =
          glm::vec3(model.meshes[i].bbox_max.x(), model.meshes[i].bbox_max.y(),
                    model.meshes[i].bbox_max.z());
      firstMesh = false;
    } else {
      glm::vec3 vectorMin =
          glm::vec3(model.meshes[i].bbox_min.x(), model.meshes[i].bbox_min.y(),
                    model.meshes[i].bbox_min.z());
      glm::vec3 vectorMax =
          glm::vec3(model.meshes[i].bbox_max.x(), model.meshes[i].bbox_max.y(),
                    model.meshes[i].bbox_max.z());
      bbox.min.x = (vectorMin.x < bbox.min.x) ? vectorMin.x : bbox.min.x;
      bbox.min.y = (vectorMin.y < bbox.min.y) ? vectorMin.y : bbox.min.y;
      bbox.min.z = (vectorMin.z < bbox.min.z) ? vectorMin.z : bbox.min.z;

      bbox.max.x = (vectorMax.x > bbox.max.x) ? vectorMax.x : bbox.max.x;
      bbox.max.y = (vectorMax.y > bbox.max.y) ? vectorMax.y : bbox.max.y;
      bbox.max.z = (vectorMax.z > bbox.max.z) ? vectorMax.z : bbox.max.z;
    }
  }

  // set corners
  bbox.SetCorners();

  return bbox;
}

void OpenGLBindMesh(const unsigned index, DD_ModelSK& model,
                    const unsigned inst_size, const unsigned inst_c_size) {
  glGenVertexArrays(1, &model.VAO[index]);
  glGenBuffers(1, &model.VBO[index]);
  glGenBuffers(1, &model.EBO[index]);

  glBindVertexArray(model.VAO[index]);
  glBindBuffer(GL_ARRAY_BUFFER, model.VBO[index]);

  glBufferData(GL_ARRAY_BUFFER, model.meshes[index].data.sizeInBytes(),
               &model.meshes[index].data[0], GL_STATIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.EBO[index]);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               model.meshes[index].indices.sizeInBytes(),
               &model.meshes[index].indices[0], GL_STATIC_DRAW);

  // position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
  // normal
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (GLvoid*)offsetof(Vertex, normal));
  // texcoords
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (GLvoid*)offsetof(Vertex, texCoords));
  // tangent normals
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (GLvoid*)offsetof(Vertex, tangent));
  // blend weights
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (GLvoid*)offsetof(Vertex, blendweight));
  // joint indices
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (GLvoid*)offsetof(Vertex, joints));

  // instancing
  glBindVertexArray(model.VAO[index]);
  glGenBuffers(1, &(model.instVBO[index]));
  glBindBuffer(GL_ARRAY_BUFFER, model.instVBO[index]);
  glBufferData(GL_ARRAY_BUFFER, inst_size * sizeof(glm::mat4), NULL,
               GL_DYNAMIC_DRAW);

  // Set attribute pointers for matrix (4 times vec4)
  glEnableVertexAttribArray(6);
  glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                        (GLvoid*)0);
  glEnableVertexAttribArray(7);
  glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                        (GLvoid*)(sizeof(glm::vec4)));
  glEnableVertexAttribArray(8);
  glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                        (GLvoid*)(2 * sizeof(glm::vec4)));
  glEnableVertexAttribArray(9);
  glVertexAttribPointer(9, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                        (GLvoid*)(3 * sizeof(glm::vec4)));
  // instance color vectors
  glGenBuffers(1, &(model.instColorVBO[index]));
  glBindBuffer(GL_ARRAY_BUFFER, model.instColorVBO[index]);
  glBufferData(GL_ARRAY_BUFFER, inst_c_size * sizeof(glm::vec3), NULL,
               GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(10);
  glVertexAttribPointer(10, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                        (GLvoid*)0);

  glVertexAttribDivisor(6, 1);
  glVertexAttribDivisor(7, 1);
  glVertexAttribDivisor(8, 1);
  glVertexAttribDivisor(9, 1);
  glVertexAttribDivisor(10, 1);

  glBindVertexArray(0);
}

void OpenGLUnBindMesh(const unsigned index, DD_ModelSK& model) {
  if (model.VBO.isValid()) {
    glDeleteBuffers((GLsizei)model.VBO.size(), &model.VBO[0]);
  }
  if (model.EBO.isValid()) {
    glDeleteBuffers((GLsizei)model.EBO.size(), &model.EBO[0]);
  }
  if (model.instVBO.isValid()) {
    glDeleteBuffers((GLsizei)model.instVBO.size(), &model.instVBO[0]);
  }
  if (model.instColorVBO.isValid()) {
    glDeleteBuffers((GLsizei)model.instColorVBO.size(), &model.instColorVBO[0]);
  }
  if (model.VAO.isValid()) {
    glDeleteVertexArrays((GLsizei)model.VAO.size(), &model.VAO[0]);
  }
}
}
