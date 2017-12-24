#include <omp.h>
#include "DD_AssetManager.h"
#include "DD_FileIO.h"

namespace {
// function callback buffer
DD_FuncBuff fb;
// containers only exist in this translation unit
btDiscreteDynamicsWorld *p_world = nullptr;

// DD_BaseAgents
ASSET_CREATE(DD_BaseAgent, b_agents, ASSETS_CONTAINER_MAX_SIZE)
// DD_Cam
ASSET_CREATE(DD_Cam, cams, ASSETS_CONTAINER_MIN_SIZE)
// DD_Bulb
ASSET_CREATE(DD_Bulb, lights, ASSETS_CONTAINER_MIN_SIZE)
// DD_MeshData
ASSET_CREATE(DD_MeshData, meshes, ASSETS_CONTAINER_MAX_SIZE)
// DD_Skeleton
ASSET_CREATE(DD_Skeleton, skeletons, ASSETS_CONTAINER_MIN_SIZE)
// DD_SkeletonPose
ASSET_CREATE(DD_SkeletonPose, poses, ASSETS_CONTAINER_MAX_SIZE)
// DD_Tex2D
ASSET_CREATE(DD_Tex2D, textures, ASSETS_CONTAINER_MAX_SIZE)
// DD_Mat
ASSET_CREATE(DD_Mat, mats, ASSETS_CONTAINER_MAX_SIZE)
}  // namespace

ASSET_DEF(DD_BaseAgent, b_agents)
ASSET_DEF(DD_Cam, cams)
ASSET_DEF(DD_Bulb, lights)
ASSET_DEF(DD_MeshData, meshes)
ASSET_DEF(DD_Skeleton, skeletons)
ASSET_DEF(DD_SkeletonPose, poses)
ASSET_DEF(DD_Tex2D, textures)
ASSET_DEF(DD_Mat, mats)

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
  dd_array<GLuint> idxs;
  unsigned mat_idx;
};

/// \brief Parse ddm file and load to ram
/// \param filename Path to .ddm file
/// \return DD_MeshData of input ddm file
DD_MeshData* load_ddm(const char *filename);
/// \brief Read file in io handle and grab vertex data
/// \param _size Number of vertices
/// \param _io file reader handle
/// \return Parsed fbx vertex data
FbxVData get_fbx_vdata(const unsigned _size, DD_IOhandle &_io);
/// \brief Read file in io handle and grab index buffer data
/// \param _size Number of indices
/// \param _io file reader handle
/// \return Parsed fbx element buffer data
FbxEData get_fbx_edata(DD_IOhandle &_io);
/// \brief Translate ebo & vertex data into MeshData object
/// \param vdata Vertices
/// \param edata Indices
/// \return Parsed mesh data object
dd_array<MeshData> get_mesh_data(FbxVData &vdata, dd_array<FbxEData> &edata);
/// \brief Read file stream and export material data
/// \param _io file reader handle
/// \return Parsed material data object
obj_mat get_mat_data(DD_IOhandle &_io);
/// \brief Create new material from obj_mat
/// \param mat_info obj_mat material information
/// \return Pointer to created material
DD_Mat* create_material(obj_mat &mat_info);

void dd_assets_initialize(btDiscreteDynamicsWorld *physics_world) {
  p_world = physics_world;
  // set free lists
  fl_b_agents.initialize(b_agents.size());
  fl_cams.initialize(cams.size());
  fl_lights.initialize(lights.size());
  fl_meshes.initialize(meshes.size());
  fl_skeletons.initialize(skeletons.size());
  fl_poses.initialize(poses.size());
  fl_textures.initialize(textures.size());
	fl_mats.initialize(mats.size());
}

int dd_assets_create_agent(lua_State *L) {
  parse_lua_events(L, fb);
  // get arguments and use them to create DD_BaseAgent
  return 0;
}

int dd_assets_create_mesh(lua_State *L) {
	parse_lua_events(L, fb);
	DD_MeshData* new_mdata = nullptr;
  // get arguments and use them to create DD_MeshData
	const char * file = fb.get_func_val<const char>("path");
	if (file) {
		new_mdata = load_ddm(file);
	}
	// return key of new mesh back up to script if created
	if (new_mdata) {
		lua_pushinteger(L, new_mdata->id);
		return 1;
	}
  return 0;
}

int dd_assets_create_cam(lua_State *L) {
  parse_lua_events(L, fb);
  // get arguments and use them to create DD_Cam
  return 0;
}

int dd_assets_create_light(lua_State *L) {
  parse_lua_events(L, fb);
  // get arguments and use them to create DD_Bulb
  return 0;
}

DD_MeshData * load_ddm(const char * filename) {
  /// \brief Lambda to get uint from string
  auto getUint = [](const char *str) {
    return (unsigned)strtoul(str, nullptr, 10);
  };
	DD_IOhandle io_handle;
  dd_array<MeshData> out_data;
	dd_array<FbxEData> edata;
	dd_array<obj_mat> mats;
	FbxVData vdata;
	cbuff<32> name;
	unsigned mat_idx = 0;
	unsigned ebo_idx = 0;
	unsigned vdata_size = 0;

	if (io_handle.open(filename, DD_IOflag::READ)) {
		const char* line = io_handle.readNextLine();

		while (line) {
			// check tag for information to parse
			if (strcmp("<name>", line) == 0) {
				line = io_handle.readNextLine();
				name = line;
			}
			if (strcmp("<buffer>", line) == 0) {
				line = io_handle.readNextLine();

				if (*line == 'v') {  // vertex buffer
					vdata_size = getUint(&line[2]);
					line = io_handle.readNextLine();
				}
				if (*line == 'e') {  // element buffer
					edata.resize(getUint(&line[2]));
					line = io_handle.readNextLine();
				}
				if (*line == 'm') {  // # of materials
					mats.resize(getUint(&line[2]));
					line = io_handle.readNextLine();
				}
			}
			if (strcmp("<vertex>", line) == 0) {
				vdata = std::move(get_fbx_vdata(vdata_size, io_handle));
			}
			if (strcmp("<ebo>", line) == 0) {
				edata[ebo_idx] = std::move(get_fbx_edata(io_handle));
				ebo_idx += 1;
			}
			if (strcmp("<material>", line) == 0) {
				mats[mat_idx] = std::move(get_mat_data(io_handle));
				mat_idx += 1;
			}
			line = io_handle.readNextLine();
		} // end of while
	}
	// create DD_MeshData
	DD_MeshData *mdata = spawnDD_MeshData(name.gethash());
	if (!mdata) { // failed to create object
		printf("load_ddm::Failed to create DD_MeshData object\n");
		return nullptr;
	}
	// set MeshInfo and material data
	out_data = get_mesh_data(vdata, edata);
	mdata->mesh_info.resize(out_data.size());
	for (size_t i = 0; i < out_data.size(); i++) {  // assign materials
		u32 idx = edata[i].mat_idx;
		// create material
		DD_Mat* mat = create_material(mats[idx]);
		if (mat) { // set mesh's material id
			mdata->mesh_info[i].material = mat->id;
		} else {
			mdata->mesh_info[i].material = getCharHash("default");
		}
		mdata->mesh_info[i].mesh_data = out_data[i];
	}

  return mdata;
}

FbxVData get_fbx_vdata(const unsigned _size, DD_IOhandle &_io) {
	/// \brief Set min and max bounding box values
	auto setBBox = [&](const glm::vec3 &test, BoundingBox &bbox) {
		// max
		bbox.max.x = (bbox.max.x > test.x) ? bbox.max.x : test.x;
		bbox.max.y = (bbox.max.y > test.y) ? bbox.max.y : test.y;
		bbox.max.z = (bbox.max.z > test.z) ? bbox.max.z : test.z;
		// min
		bbox.min.x = (bbox.min.x < test.x) ? bbox.min.x : test.x;
		bbox.min.y = (bbox.min.y < test.y) ? bbox.min.y : test.y;
		bbox.min.z = (bbox.min.z < test.z) ? bbox.min.z : test.z;
	};
  FbxVData vdata;
  vdata.setSize(_size);
  const char *line = _io.readNextLine();
  glm::vec4 v4;
  glm::uvec4 v4u;
  glm::vec3 v3;
  glm::vec2 v2;
  unsigned idx = 0;

  while (strcmp("</vertex>", line) != 0) {
    switch (*line) {
      case 'v':  // position
        v3 = getVec3f(&line[2]);
        vdata.v[idx] = v3;
				if (idx == 0) {
					vdata.bbox.max = v3;
					vdata.bbox.min = v3;
				} else {
					setBBox(v3, vdata.bbox);
				}
        break;
      case 'n':  // normal
        v3 = getVec3f(&line[2]);
        vdata.n[idx] = v3;
        break;
      case 't':  // tangent
        v3 = getVec3f(&line[2]);
        vdata.t[idx] = v3;
        break;
      case 'u':  // uv
        v2 = getVec2f(&line[2]);
        vdata.u[idx] = v2;
        break;
      case 'j':  // joint index
        v4u = getVec4f(&line[2]);
        vdata.j[idx] = v4u;
        break;
      case 'b':  // joint blend weights
        v4 = getVec4f(&line[2]);
        vdata.b[idx] = v4;
        idx += 1;  // increment index
        break;
      default:
        break;
    }
		line = _io.readNextLine();
  }
	vdata.bbox.SetCorners();

  return vdata;
}

FbxEData get_fbx_edata(DD_IOhandle &_io) {
  /// \brief Lambda to get vec3 unsigned from c string
  auto getVec3u = [&](const char *str) {
    glm::uvec3 v3;
    char *nxt;
    v3[0] = strtoul(str, &nxt, 10);
    nxt++;
    v3[1] = strtoul(nxt, &nxt, 10);
    nxt++;
    v3[2] = strtoul(nxt, nullptr, 10);
    return v3;
  };
  FbxEData edata;
  const char *line = _io.readNextLine();
  glm::uvec3 v3u;
  unsigned idx = 0;

  while (strcmp("</ebo>", line) != 0) {
    switch (*line) {
      case 's':  // number of triangles
      {
        const char *s = &line[2];
        edata.idxs.resize(strtoul(s, nullptr, 10));
      } break;
      case 'm':  // material index
      {
        const char *s = &line[2];
        edata.mat_idx = strtoul(s, nullptr, 10);
      } break;
      case '-':  // triangle indices
        v3u = getVec3u(&line[2]);
        edata.idxs[idx] = v3u.x;
        idx++;
        edata.idxs[idx] = v3u.y;
        idx++;
        edata.idxs[idx] = v3u.z;
        idx++;
        break;
      default:
        break;
    }
		line = _io.readNextLine();
  }
  return edata;
}

dd_array<MeshData> get_mesh_data(FbxVData & vdata, dd_array<FbxEData>& edata) {
	/// \brief set MeshData components
	auto setMData = [](const unsigned m_idx, const unsigned v_idx,
										 MeshData &mdata, const FbxVData &vdata) {
		// position
		mdata.data[m_idx].position[0] = vdata.v[v_idx].x;
		mdata.data[m_idx].position[1] = vdata.v[v_idx].y;
		mdata.data[m_idx].position[2] = vdata.v[v_idx].z;
		// normal
		mdata.data[m_idx].normal[0] = vdata.n[v_idx].x;
		mdata.data[m_idx].normal[1] = vdata.n[v_idx].y;
		mdata.data[m_idx].normal[2] = vdata.n[v_idx].z;
		// tangent
		mdata.data[m_idx].tangent[0] = vdata.t[v_idx].x;
		mdata.data[m_idx].tangent[1] = vdata.t[v_idx].y;
		mdata.data[m_idx].tangent[2] = vdata.t[v_idx].z;
		// uv
		mdata.data[m_idx].texCoords[0] = vdata.u[v_idx].x;
		mdata.data[m_idx].texCoords[1] = vdata.u[v_idx].y;
		// joints
		mdata.data[m_idx].joints[0] = vdata.j[v_idx].x;
		mdata.data[m_idx].joints[1] = vdata.j[v_idx].y;
		mdata.data[m_idx].joints[2] = vdata.j[v_idx].z;
		mdata.data[m_idx].joints[3] = vdata.j[v_idx].w;
		// blend weights
		mdata.data[m_idx].blendweight[0] = vdata.b[v_idx].x;
		mdata.data[m_idx].blendweight[1] = vdata.b[v_idx].y;
		mdata.data[m_idx].blendweight[2] = vdata.b[v_idx].z;
		mdata.data[m_idx].blendweight[3] = vdata.b[v_idx].w;
	};

	dd_array<MeshData> mdata;
	mdata.resize(edata.size());
	std::map<unsigned, unsigned> vertbin;

	for (size_t i = 0; i < edata.size(); i++) {  // granualarity of ebo
		FbxEData& _e = edata[i];
		MeshData& md = mdata[i];
		
		// set bounding box properties
		md.bb_max = vdata.bbox.max;
		md.bb_min = vdata.bbox.min;

		md.indices.resize(_e.idxs.size());
		vertbin.clear();
		unsigned vert_idx = 0;
		// set indices (using dictionary to avoid copies)
		for (size_t j = 0; j < _e.idxs.size(); j++) {  // granularity of ebo indices
			const unsigned _i = _e.idxs[j];
			if (vertbin.count(_i) == 0) {
				vertbin[_i] = vert_idx;
				vert_idx += 1;
			}
			md.indices[j] = vertbin[_i];
		}
		md.data.resize(vertbin.size());

#pragma omp parallel
		{
			size_t cnt = 0;
			int ithread = omp_get_thread_num();
			int nthreads = omp_get_num_threads();
			for (auto vert = vertbin.begin(); vert != vertbin.end(); ++vert, cnt++) {
				if (cnt%nthreads != ithread) continue;
				setMData(vert->second, vert->first, md, vdata);
			}
		}
	}

	return mdata;
}

obj_mat get_mat_data(DD_IOhandle & _io) {
	obj_mat data;
	const char *line = _io.readNextLine();
	glm::vec3 v3;

	while (strcmp("</material>", line) != 0) {
		switch (*line) {
			case 'n':
				data.mat_id = &line[2];
				break;
			case 'D':
				data.albedo_tex = &line[2];
				data.albedo_flag = true;
				break;
			case 'N':
				data.normal_tex = &line[2];
				data.norm_flag = true;
				break;
			case 'S':
				data.specular_tex = &line[2];
				data.spec_flag = true;
				break;
			case 'R':
				data.roughness_tex = &line[2];
				data.rough_flag = true;
				break;
			case 'M':
				data.metalness_tex = &line[2];
				data.metal_flag = true;
				break;
			case 'E':
				data.emissive_tex = &line[2];
				data.emit_flag = true;
				break;
			case 'A':
				data.ao_tex = &line[2];
				data.ao_flag = true;
				break;
			default:
				break;
		}
		line = _io.readNextLine();
	}
	return data;
}

DD_Mat* create_material(obj_mat & mat_info) {
	DD_Mat *mat = spawnDD_Mat(mat_info.mat_id.gethash());
	if (!mat) { // failed to allocate
		printf("create_material::Failed to create DD_Mat object\n");
		return nullptr;
	}
	mat->base_color = mat_info.diff_raw;
	if (mat_info.albedo_flag) {
		//tex = CreateTexture_OBJMAT(res, mat_info.directory, mat_info.albedo_tex);
		//mat.AddTexture(tex, TextureType::ALBEDO);
	}
	if (mat_info.spec_flag) {
		//tex = CreateTexture_OBJMAT(res, mat_info.directory, mat_info.specular_tex);
		//mat.AddTexture(tex, TextureType::SPECULAR);
	}
	if (mat_info.ao_flag) {
		//tex = CreateTexture_OBJMAT(res, mat_info.directory, mat_info.ao_tex);
		//mat.AddTexture(tex, TextureType::AO);
	}
	if (mat_info.norm_flag) {
		//tex = CreateTexture_OBJMAT(res, mat_info.directory, mat_info.normal_tex);
		//mat.AddTexture(tex, TextureType::NORMAL);
	}
	if (mat_info.rough_flag) {
		//tex = CreateTexture_OBJMAT(res, mat_info.directory, mat_info.roughness_tex);
		//mat.AddTexture(tex, TextureType::ROUGH);
	}
	if (mat_info.metal_flag) {
		//tex = CreateTexture_OBJMAT(res, mat_info.directory, mat_info.metalness_tex);
		//mat.AddTexture(tex, TextureType::METAL);
	}
	if (mat_info.emit_flag) {
		//tex = CreateTexture_OBJMAT(res, mat_info.directory, mat_info.emissive_tex);
		//mat.AddTexture(tex, TextureType::EMISSIVE);
	}
	// set multiplier material
	if (mat_info.multiplier) {
		//mat.SetMultiplierMaterial(TextureType::ALBEDO);
	}
	return mat;
}
