#include "ddImport_Model.h"
#include "ddTerminal.h"
#include <omp.h>

#include "stb_image.h"
#include "stb_image_write.h"

ddModelData *load_ddm(const char *filename) {
	/// \brief Lambda to get uint from string
	auto getUint = [](const char *str) {
		return (unsigned)strtoul(str, nullptr, 10);
	};
	ddModelData *mdata = nullptr;
	ddIO io_handle;
	dd_array<DDM_Data> out_data;
	dd_array<FbxEData> edata;
	dd_array<obj_mat> o_mats;
	FbxVData vdata;
	cbuff<32> name;
	unsigned mat_idx = 0;
	unsigned ebo_idx = 0;
	unsigned vdata_size = 0;

	if (io_handle.open(filename, ddIOflag::READ)) {
		const char *line = io_handle.readNextLine();

		while (line) {
			// check tag for information to parse
			if (strcmp("<name>", line) == 0) {
				line = io_handle.readNextLine();
				name = line;

				// check if Mesh already exists and exit if found
				mdata = find_ddModelData(name.gethash());
				if (mdata) {
					ddTerminal::f_post("Duplicate mesh <%s>", name.str());
					return mdata;
				}
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
					o_mats.resize(getUint(&line[2]));
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
				o_mats[mat_idx] = std::move(get_mat_data(io_handle));
				mat_idx += 1;
			}
			line = io_handle.readNextLine();
		}  // end of while
			 // create ddModelData only if file exists
		mdata = spawn_ddModelData(name.gethash());
	}
	if (!mdata) {  // failed to create object
		ddTerminal::f_post("[error]load_ddm::Failed to create ddModelData object");
		return nullptr;
	}
	// set MeshInfo and material data
	out_data = get_mesh_data(vdata, edata);
	mdata->mesh_info = std::move(out_data);
	for (size_t i = 0; i < mdata->mesh_info.size(); i++) {  // assign materials
		uint32_t idx = edata[i].mat_idx;
		// set path
		mdata->mesh_info[i].path = filename;
		// create material
		ddMat *mat = create_material(o_mats[idx]);
		if (mat) {  // set mesh's material id
			mdata->mesh_info[i].mat_id = mat->id;
		}
		else {
			mdata->mesh_info[i].mat_id = getCharHash("default");
		}
	}
	return mdata;
}

FbxVData get_fbx_vdata(const unsigned _size, ddIO &_io) {
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
			}
			else {
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

FbxEData get_fbx_edata(ddIO &_io) {
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

dd_array<DDM_Data> get_mesh_data(FbxVData &vdata, dd_array<FbxEData> &edata) {
	/// \brief set MeshData components
	auto setMData = [](const unsigned m_idx, const unsigned v_idx,
										 DDM_Data &mdata, const FbxVData &vdata) {
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

	dd_array<DDM_Data> mdata;
	mdata.resize(edata.size());
	std::map<unsigned, unsigned> vertbin;

	for (size_t i = 0; i < edata.size(); i++) {  // granualarity of ebo
		FbxEData &_e = edata[i];
		DDM_Data &md = mdata[i];

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
			size_t ithread = (size_t)omp_get_thread_num();
			size_t nthreads = (size_t)omp_get_num_threads();
			for (auto vert = vertbin.begin(); vert != vertbin.end(); ++vert, cnt++) {
				if (cnt % nthreads != ithread) continue;
				setMData(vert->second, vert->first, md, vdata);
			}
		}
	}

	return mdata;
}

obj_mat get_mat_data(ddIO &_io) {
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

ddMat *create_material(obj_mat &mat_info) {
	// simple utility function for retrieving index of flag
	auto get_tex_idx = [](const TexType t_t) {
		return (size_t)std::log2((double)t_t);
	};
	// simple utility function for assigning a texture to material
	auto set_texture = [&](ddMat *mat, const size_t tex, const TexType t_t) {
		size_t tex_idx = get_tex_idx(t_t);
		mat->textures[tex_idx] = tex;
		mat->texture_flag |= t_t;
	};

	// check if material already exists
	ddMat *mat = find_ddMat(mat_info.mat_id.gethash());
	if (mat) {
		ddTerminal::f_post("Duplicate material <%s>", mat_info.mat_id.str());
		return mat;
	}

	mat = spawn_ddMat(mat_info.mat_id.gethash());
	if (!mat) {  // failed to allocate
		ddTerminal::f_post("[error]create_material::Failed to create ddMat object");
		return nullptr;
	}
	// set size of texture id container
	mat->textures.resize(get_tex_idx(TexType::NULL_T));
	mat->base_color = mat_info.diff_raw;
	cbuff<32> tex_id;

	if (mat_info.albedo_flag) {
		ddTex2D *tex =
			create_tex2D(mat_info.albedo_tex.str(), mat_info.mat_id.str());
		if (tex) set_texture(mat, tex->id, TexType::ALBEDO);
	}
	if (mat_info.spec_flag) {
		ddTex2D *tex =
			create_tex2D(mat_info.specular_tex.str(), mat_info.mat_id.str());
		if (tex) set_texture(mat, tex->id, TexType::SPEC);
	}
	if (mat_info.ao_flag) {
		ddTex2D *tex = create_tex2D(mat_info.ao_tex.str(), mat_info.mat_id.str());
		if (tex) set_texture(mat, tex->id, TexType::AMBIENT);
	}
	if (mat_info.norm_flag) {
		ddTex2D *tex =
			create_tex2D(mat_info.normal_tex.str(), mat_info.mat_id.str());
		if (tex) set_texture(mat, tex->id, TexType::NORMAL);
	}
	if (mat_info.rough_flag) {
		ddTex2D *tex =
			create_tex2D(mat_info.roughness_tex.str(), mat_info.mat_id.str());
		if (tex) set_texture(mat, tex->id, TexType::ROUGH);
	}
	if (mat_info.metal_flag) {
		ddTex2D *tex =
			create_tex2D(mat_info.metalness_tex.str(), mat_info.mat_id.str());
		if (tex) set_texture(mat, tex->id, TexType::METAL);
	}
	if (mat_info.emit_flag) {
		ddTex2D *tex =
			create_tex2D(mat_info.emissive_tex.str(), mat_info.mat_id.str());
		if (tex) set_texture(mat, tex->id, TexType::EMISSIVE);
	}
	// set multiplier material
	mat->color_modifier = mat_info.multiplier;
	return mat;
}

ddTex2D *create_tex2D(const char *path, const char *img_id) {
	// check if texture already exists
	size_t tex_id = getCharHash(img_id);
	ddTex2D *new_tex = find_ddTex2D(tex_id);
	if (new_tex) {
		ddTerminal::f_post("Duplicate texture <%s>", img_id);
		return new_tex;
	}

	// initialize
	ImageInfo img_info;

	// find and load image to RAM
	img_info.path[0] = path;
	unsigned char *temp = nullptr;
	temp =
		stbi_load(path, &img_info.width, &img_info.height, &img_info.channels, 4);
	POW2_VERIFY_MSG(temp != nullptr, "stbi failed to read image: %s", path);

	// copy to dd_array and free stbi data
	img_info.image_data[0].resize(img_info.width * img_info.height * 4);
#pragma omp for
	for (int i = 0; i < img_info.width * img_info.height * 4; i++) {
		img_info.image_data[0][i] = temp[i];
	}
	stbi_image_free(temp);

	// create texture object and assign img_info
	new_tex = spawn_ddTex2D(tex_id);
	if (!new_tex) {  // failed to allocate
									 // report
		ddTerminal::f_post("[error]create_tex2D::Failed to create ddTex2D object");
		return new_tex;
	}
	new_tex->image_info = std::move(img_info);

	return new_tex;
}