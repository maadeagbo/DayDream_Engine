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

#include <DD_Material.h>
#include <DD_MeshTypes.h>
#include <DD_Types.h>

struct DD_Model {
  std::string m_ID;
  dd_array<MeshData> meshes;
  dd_array<GLuint> VAO, VBO, EBO, instVBO, instColorVBO;
  dd_array<size_t> materials;  // index
  std::string directory;
  bool m_loaded_to_GPU = false;
};

namespace ModelSpace {
BoundingBox CalculateBBox(const DD_Model& model);
void OpenGLBindMesh(const int index, DD_Model& model, const size_t inst_size,
                    const size_t inst_c_size);
void OpenGLUnBindMesh(const int index, DD_Model& model);
void PrintInfo(const DD_Model& mod);
}

/// \brief Data used by render engine for drawing
struct DD_MeshData {
  /// \brief Engine identifier assigned at initialization
  size_t id;
  /// \brief Mesh information
  dd_array<MeshInfo> mesh_info;
  /// \brief DD_SkeletonPose identification
  size_t skinnedpose_id;
  /// \brief Marks if mesh has skinned data
  bool skinned = false;
};