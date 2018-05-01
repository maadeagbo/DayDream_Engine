#include "ddImport_Model.h"
#include <omp.h>
#include "ddAssetManager.h"
#include "ddTerminal.h"

#include "stb_image.h"
#include "stb_image_write.h"

struct DDM_Data_Out {
  dd_array<DDM_Data> data;
  cbuff<32> name;
  dd_array<FbxEData> edata;
  dd_array<obj_mat> o_mats;
  FbxVData vdata;
  ddModelData *duplicate = nullptr;
};

// parse ddm file and output mesh data
DDM_Data_Out get_ddm_data(ddIO &io) {
  /// \brief Lambda to get uint from string
  auto getUint = [](const char *str) {
    return (unsigned)strtoul(str, nullptr, 10);
  };

  DDM_Data_Out out_data;
  unsigned mat_idx = 0;
  unsigned ebo_idx = 0;
  unsigned vdata_size = 0;

  const char *line = io.readNextLine();
  while (line && *line) {
    // check tag for information to parse
    if (strcmp("<name>", line) == 0) {
      line = io.readNextLine();
      out_data.name = line;

      // check if Mesh already exists and exit if found
      out_data.duplicate = find_ddModelData(out_data.name.gethash());
      if (out_data.duplicate) {
        ddTerminal::f_post("Duplicate mesh <%s>", out_data.name.str());
        return out_data;
      }
    }
    if (strcmp("<buffer>", line) == 0) {
      line = io.readNextLine();

      if (*line == 'v') {  // vertex buffer
        vdata_size = getUint(&line[2]);
        line = io.readNextLine();
      }
      if (*line == 'e') {  // element buffer
        out_data.edata.resize(getUint(&line[2]));
        line = io.readNextLine();
      }
      if (*line == 'm') {  // # of materials
        out_data.o_mats.resize(getUint(&line[2]));
        line = io.readNextLine();
      }
    }
    if (strcmp("<vertex>", line) == 0) {
      out_data.vdata = std::move(get_fbx_vdata(vdata_size, io));
    }
    if (strcmp("<ebo>", line) == 0) {
      out_data.edata[ebo_idx] = std::move(get_fbx_edata(io));
      ebo_idx += 1;
    }
    if (strcmp("<material>", line) == 0) {
      out_data.o_mats[mat_idx] = std::move(get_mat_data(io));
      mat_idx += 1;
    }
    line = io.readNextLine();
  }
  out_data.data = get_mesh_data(out_data.vdata, out_data.edata);

  return out_data;
}

ddModelData *load_ddm(const char *filename) {
  ddModelData *mdata = nullptr;
  DDM_Data_Out out_data;
  ddIO io_handle;
  cbuff<32> name;

  if (io_handle.open(filename, ddIOflag::READ)) {
    out_data = get_ddm_data(io_handle);

    // check if duplicate exists or else create ddModelData if DDM_Data exists
    if (out_data.duplicate) {
      mdata = out_data.duplicate;
    } else if (out_data.data.size() > 0) {
      mdata = spawn_ddModelData(out_data.name.gethash());
    }
  }
  if (!mdata) {  // failed to create object
    ddTerminal::f_post("[error]load_ddm::Failed to create ddModelData object");
    return nullptr;
  }
  // set MeshInfo and material data
  mdata->mesh_info = std::move(out_data.data);
  for (size_t i = 0; i < mdata->mesh_info.size(); i++) {  // assign materials
    uint32_t idx = out_data.edata[i].mat_idx;
    // set path
    mdata->mesh_info[i].path = filename;
    // create material
    ddMat *mat = create_material(out_data.o_mats[idx]);
    if (mat) {  // set mesh's material id
      mdata->mesh_info[i].mat_id = mat->id;
    } else {
      mdata->mesh_info[i].mat_id = getCharHash("default");
    }
  }
  return mdata;
}

ddModelData *load_ddg(const char *filename) {
  ddModelData *mdata = nullptr;
  ddIO io_handle;
  dd_array<DDM_Data> out_data;
  cbuff<32> name;

  // open .ddg file
  if (io_handle.open(filename, ddIOflag::READ)) {
    cbuff<512> mybuff;
    unsigned meshes_added = 0;
    const char *line = io_handle.readNextLine();

    while (line) {
      mybuff.set(line);

      // name of .ddg object
      if (mybuff.compare("<name>") == 0) {
        line = io_handle.readNextLine();

        name = line;
      }

      // parse each .ddm file to separate DDM_Data_Out
      if (mybuff.compare("<mesh>") == 0) {
        line = io_handle.readNextLine();

        ddIO io_handle2;
        DDM_Data_Out mesh_data;
        if (io_handle2.open(line, ddIOflag::READ)) {
          mesh_data = get_ddm_data(io_handle2);
        }

        // for each ddm data: create material, assign mat id to DDM_Data,
        // add to out_data buffer
        if (mesh_data.data.size() != 0) {
          for (size_t i = 0; i < mesh_data.edata.size(); i++) {
            uint32_t idx = mesh_data.edata[i].mat_idx;
            // set path
            mesh_data.data[i].path = filename;
            // create material
            ddMat *mat = create_material(mesh_data.o_mats[idx]);
            if (mat) {  // set mesh's material id
              mesh_data.data[i].mat_id = mat->id;
            } else {
              mesh_data.data[i].mat_id = getCharHash("default");
            }
          }

          // add to ddModelData
          if (meshes_added == 0) {
            meshes_added += (unsigned)mesh_data.data.size();
            out_data = std::move(mesh_data.data);
          } else {
            // add to outmesh bin
            unsigned idx = meshes_added;
            meshes_added += (unsigned)mesh_data.data.size();
            dd_array<DDM_Data> temp = std::move(out_data);
            out_data.resize(meshes_added);
            out_data = temp;
            // copy
            for (unsigned i = 0; idx < meshes_added; idx++, i++) {
              out_data[idx] = std::move(mesh_data.data[i]);
            }
          }
        }
      }
      line = io_handle.readNextLine();
    }
    // end of while
  }
  // create ddModelData from out_data
  if (name.compare("") != 0 && out_data.isValid()) {
    mdata = spawn_ddModelData(name.gethash());

    mdata->mesh_info = std::move(out_data);
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

void flip_im(unsigned char *image, const int width, const int height,
								const int channels) {
	// validate parameters
	if (width < 0 || height < 0 || channels < 0) {
		fprintf(stderr, "flip_image::Invalid paramters <%d::%d::%d>", width, height,
						channels);
		return;
	}
	// flip
	for (int j = 0; j * 2 < height; j++) {
		int idx_1 = width * channels * j;
		int idx_2 = width * channels * (height - 1 - j);
		for (int i = width * channels; i > 0; i--) {
			unsigned char temp = image[idx_1];
			image[idx_1] = image[idx_2];
			image[idx_2] = temp;
			// img_ptr.get()[idx_2] = image[idx_1];
			idx_1++;
			idx_2++;
		}
	}
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
	flip_im(temp, img_info.width, img_info.height, img_info.channels);

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

  // skip loading if using opengl api and on separate thread
  bool skip = DD_GRAPHICS_API == 0 && ddAssets::load_screen_check();
  if (!skip) {
    ddGPUFrontEnd::generate_texture2D_RGBA8_LR(new_tex->image_info);
    new_tex->image_info.image_data[0].resize(0);
  }

  return new_tex;
}

dd_array<OOBoundingBox> load_ddx(const char *path) {
  /// \brief Lambda to get int from string
  auto get_int = [](const char *str) { return (int)strtol(str, nullptr, 10); };

  dd_array<OOBoundingBox> bboxes;
  int b_idx = -1;

  // check that path is .ddx
  cbuff<512> path_check(path);
  if (!path_check.contains(".ddx")) {
    ddTerminal::f_post("[error]Invalid ddx file: %s", path);
    return bboxes;
  }

  // attempt to open file
  ddIO _io;
  if (!_io.open(path, ddIOflag::READ)) return bboxes;

  const char *line = _io.readNextLine();
  while (line && *line) {
    // check tag for information to parse
    if (strcmp("<size>", line) == 0) {
      line = _io.readNextLine();
      bboxes.resize(get_int(line)); // get value & resize output array
    }
    if (strcmp("<box>", line) == 0) {
      b_idx++; // update current box index
    }
    if (*line == 'j') {  // joint id
      bboxes[b_idx].joint_idx = get_int(&line[2]); // set joint index for box
    }
		if (*line == 'm') {  // mirror vector and flag
			bboxes[b_idx].mirror = getVec3f(&line[2]);
			bboxes[b_idx].mirror_flag = (bboxes[b_idx].mirror.x < 0.f ||
																	 bboxes[b_idx].mirror.y < 0.f ||
																	 bboxes[b_idx].mirror.z < 0.f);
		}
    if (*line == 'p') {  // position
      bboxes[b_idx].pos = getVec3f(&line[2]);
    }
		if (*line == 'r') {  // rotation
			bboxes[b_idx].rot = getQuat(&line[2]);
		}
		if (*line == 's') {  // scale
			bboxes[b_idx].scale = getVec3f(&line[2]);
		}
    line = _io.readNextLine();
  }

  return bboxes;
}
