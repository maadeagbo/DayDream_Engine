#pragma once

#include "ddIncludes.h"
#include "LuaHooks.h"
#include "ddFileIO.h"
#include "ddAssets.h"

/// \brief Object to store vertex data from DDM
struct FbxVData {
	dd_array<glm::vec3> v;
	dd_array<glm::vec3> n;
	dd_array<glm::vec3> t;
	dd_array<glm::vec2> u;
	dd_array<glm::vec4> j;
	dd_array<glm::vec4> b;
	BoundingBox bbox;
	void setSize(const unsigned size) {
		v.resize(size);
		n.resize(size);
		t.resize(size);
		u.resize(size);
		j.resize(size);
		b.resize(size);
	}
};

/// \brief Object to store ebo data from DDM
struct FbxEData {
	dd_array<unsigned> idxs;
	unsigned mat_idx;
};

/**
* \brief Create ddModelData from lua scripts
* \param L lua state
* \return Number of returned values to lua
*/
int dd_assets_create_mesh(lua_State *L);
/// \brief Parse ddm file and load to ram
/// \param filename Path to .ddm file
/// \return ddModelData of input ddm file
ddModelData *load_ddm(const char *filename);
/// \brief Read file in io handle and grab vertex data
/// \param _size Number of vertices
/// \param _io file reader handle
/// \return Parsed fbx vertex data
FbxVData get_fbx_vdata(const unsigned _size, ddIO &_io);
/// \brief Read file in io handle and grab index buffer data
/// \param _size Number of indices
/// \param _io file reader handle
/// \return Parsed fbx element buffer data
FbxEData get_fbx_edata(ddIO &_io);
/// \brief Translate ebo & vertex data into MeshData object
/// \param vdata Vertices
/// \param edata Indices
/// \return Parsed mesh data object
dd_array<DDM_Data> get_mesh_data(FbxVData &vdata, dd_array<FbxEData> &edata);
/// \brief Read file stream and export material data
/// \param _io file reader handle
/// \return Parsed material data object
obj_mat get_mat_data(ddIO &_io);
/// \brief Create new material from obj_mat
/// \param mat_info obj_mat material information
/// \return Pointer to created material
ddMat *create_material(obj_mat &mat_info);
/// \brief Create new 2D texture
/// \param path File name of texture
/// \return Pointer to created texture
ddTex2D *create_tex2D(const char *path, const char *img_id);
