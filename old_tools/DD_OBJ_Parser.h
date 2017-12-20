/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	Line_Type(enum):
*		-
*	FaceStyle(enum):
*		-
*	MeshIndex:
*		-
*	ObjInfo:
*		-
*			-
*	ObjAsset:
*		-
*			-
*	ObjAssetParser(namespace):
*		-
*			-
*
*	TODO:	== finish setsinglemeshdata (maybe)
*			== add tangent space calc
*			== add native format support
*
-----------------------------------------------------------------------------*/

#include <sys/stat.h>
#include "DD_MeshTypes.h"
#include "DD_Types.h"

enum LineType {
  VERTEX,
  VERTEX_NORMAL,
  VERTEX_UV,
  FACE,
  MTL,
  MTL_FILE,
  COMMENT,
  NUM_L_TYPES
};

enum FaceStyle { ONLY_V, V_VT, V_VN, V_VT_VN, NOT_SET };

struct MeshIndex {
  size_t start = 0, end = 0, tris = 0;
};

struct ObjInfo {
  std::string mtl_filename, directory, ddMeshName;
  size_t num_v, num_vt, num_vn, num_f, num_meshes;
  dd_array<MeshIndex> mesh_indices;
  dd_array<std::string> mat_IDs;
  dd_array<obj_mat> mat_buffer;
  dd_array<FaceStyle> f_style;
  ObjInfo()
      : num_v(0),
        num_vt(0),
        num_vn(0),
        num_f(0),
        num_meshes(0),
        mesh_indices(50),
        mat_IDs(50),
        mat_buffer(50),
        f_style(50) {}
};

struct ObjAsset {
  ObjAsset();
  ~ObjAsset();

  dd_array<MeshData> meshes;
  dd_array<obj_vec3> indexed_v;
  dd_array<obj_vec3> indexed_vt;
  dd_array<obj_vec3> indexed_vn;
  ObjInfo info;
  char* file_buffer;
};

namespace ObjAssetParser {
struct ParseFlag {
  bool success = false, ddMesh = false;
};
inline bool FileExists(const char* full_file_path) {
  struct stat file_buffer;
  return (stat(full_file_path, &file_buffer) == 0);
}

ParseFlag PreProcess(ObjAsset& obj, const char* obj_full_path,
                     const char* mtlName = "");
void PrintInfo(ObjAsset& obj);
bool FormatForOpenGL(ObjAsset& obj);
void CreateDDMesh(ObjAsset& obj);
bool LoadDDMesh(ObjAsset& obj);
void SetBoundingBox(const obj_vec3& test, MeshData& mesh);
void CalculateTangentSpace(ObjAsset& obj, const size_t meshIndex);
}
