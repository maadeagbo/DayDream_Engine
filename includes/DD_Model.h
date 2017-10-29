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

#include <DD_Types.h>
#include <DD_MeshTypes.h>
#include <DD_Material.h>



struct DD_Model
{
	std::string m_ID;
	dd_array<MeshData> meshes;
	dd_array<GLuint> VAO, VBO, EBO, instVBO, instColorVBO;
	dd_array<size_t> materials; // index
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
