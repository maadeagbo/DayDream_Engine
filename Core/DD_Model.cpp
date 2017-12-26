#include "DD_Model.h"

/*
BoundingBox ModelSpace::CalculateBBox(const DD_Model& model) {
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
//*/

void ModelSpace::OpenGLBindMesh(const int index, DD_Model& model,
                                const size_t inst_size,
                                const size_t inst_c_size) {
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
  // instancing
  glBindVertexArray(model.VAO[index]);
  glGenBuffers(1, &(model.instVBO[index]));
  glBindBuffer(GL_ARRAY_BUFFER, model.instVBO[index]);
  glBufferData(GL_ARRAY_BUFFER, inst_size * sizeof(glm::mat4), NULL,
               GL_DYNAMIC_DRAW);

  // Set attribute pointers for matrix (4 times vec4)
  glEnableVertexAttribArray(4);
  glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                        (GLvoid*)0);
  glEnableVertexAttribArray(5);
  glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                        (GLvoid*)(sizeof(glm::vec4)));
  glEnableVertexAttribArray(6);
  glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                        (GLvoid*)(2 * sizeof(glm::vec4)));
  glEnableVertexAttribArray(7);
  glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4),
                        (GLvoid*)(3 * sizeof(glm::vec4)));
  // instance color vectors
  glGenBuffers(1, &(model.instColorVBO[index]));
  glBindBuffer(GL_ARRAY_BUFFER, model.instColorVBO[index]);
  glBufferData(GL_ARRAY_BUFFER, inst_c_size * sizeof(glm::vec3), NULL,
               GL_DYNAMIC_DRAW);

  glEnableVertexAttribArray(8);
  glVertexAttribPointer(8, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3),
                        (GLvoid*)0);

  glVertexAttribDivisor(4, 1);
  glVertexAttribDivisor(5, 1);
  glVertexAttribDivisor(6, 1);
  glVertexAttribDivisor(7, 1);
  glVertexAttribDivisor(8, 1);

  glBindVertexArray(0);
}

void ModelSpace::OpenGLUnBindMesh(const int index, DD_Model& model) {
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

/*
void ModelSpace::PrintInfo(const DD_Model& mod) {
  printf("\nModel ID: %s\n", mod.m_ID.c_str());
  for (size_t i = 0; i < mod.meshes.size(); i++) {
    printf("Mesh %zd:\n", i + 1);
    printf("\tNum vertices: %zd\n", mod.meshes[i].data.size());
    printf("\tBBox min: %.3f, %.3f, %.3f\n", mod.meshes[i].bbox_min.x(),
           mod.meshes[i].bbox_min.y(), mod.meshes[i].bbox_min.z());
    printf("\tBBox max: %.3f, %.3f, %.3f\n", mod.meshes[i].bbox_max.x(),
           mod.meshes[i].bbox_max.y(), mod.meshes[i].bbox_max.z());
    printf("\tMaterial ID: %s\n", mod.meshes[i].material_ID.c_str());
    if (mod.meshes[i].material_info.albedo_flag) {
      printf("\t\talbedo: %s\n",
             mod.meshes[i].material_info.albedo_tex.c_str());
    }
    if (mod.meshes[i].material_info.spec_flag) {
      printf("\t\tspecular: %s\n",
             mod.meshes[i].material_info.specular_tex.c_str());
    }
    if (mod.meshes[i].material_info.norm_flag) {
      printf("\t\tnormal: %s\n",
             mod.meshes[i].material_info.normal_tex.c_str());
    }
  }
}
//*/
