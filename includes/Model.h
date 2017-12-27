#pragma once

/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	DD_Model:
*		- Store 3D mesh information for rendering
*			- OpenGL VAO info
*			- Material info
*			- Level of detail meshes
*			- Bounding Box
*	Model(namespace):
*		- loadModel
*			-
*		- CalculateBoundingBox
*			-
*		- OpenGLBindMesh
*			-
*	TODO:
*
-----------------------------------------------------------------------------*/

#include "Material.h"
#include "MeshTypes.h"
#include "Types.h"

// struct DD_Model {
//   std::string m_ID;
//   dd_array<DDM_Data> meshes;
//   dd_array<GLuint> VAO, VBO, EBO, instVBO, instColorVBO;
//   dd_array<size_t> materials;  // index
//   std::string directory;
//   bool m_loaded_to_GPU = false;
// };

// namespace ModelSpace {
// // BoundingBox CalculateBBox(const DD_Model& model);
// void OpenGLBindMesh(const int index, DD_Model& model, const size_t inst_size,
//                     const size_t inst_c_size);
// void OpenGLUnBindMesh(const int index, DD_Model& model);
// // void PrintInfo(const DD_Model& mod);
// }

/// \brief Container for agent mesh information
struct ModelIDs {
  /// \brief LOD bounds
  float _near = 0.f, _far = 100.f;
  /// \brief Mesh id
  size_t model = -1;
  /// \brief Marks whether the mesh is skinned for animation
  bool sk_flag = false;
};

/// \brief Container for mesh data stored on GPU
struct GPUInfo {
  /// \brief Handles to GPU buffers
  unsigned vao = 0, inst_vbo = 0, inst_cvbo = 0;
  /// \brief Flag that marks if the mesh is back on the gpu
  bool loaded_gpu = false;
};

/// \brief Data used by render engine for drawing
struct ddModelData {
  /// \brief Engine identifier assigned at initialization
  size_t id;
  /// \brief Mesh information
  dd_array<DDM_Data> mesh_info;
  /// \brief GPU buffer handles
  unsigned vbo = 0, ebo = 0;
  /// \brief Flag that marks if the mesh is back on the gpu
  bool loaded_gpu = false;
};