#include "ddAssetManager.h"
#include <SOIL.h>
#include <omp.h>
#include "ddFileIO.h"
#include "ddTerminal.h"

namespace {
// function callback buffer
DD_FuncBuff fb;
// containers only exist in this translation unit
btDiscreteDynamicsWorld *p_world = nullptr;

// ddAgents
ASSET_CREATE(ddAgent, b_agents, ASSETS_CONTAINER_MAX_SIZE)
// ddCam
ASSET_CREATE(ddCam, cams, ASSETS_CONTAINER_MIN_SIZE)
// ddLBulb
ASSET_CREATE(ddLBulb, lights, ASSETS_CONTAINER_MIN_SIZE)
// ddModelData
ASSET_CREATE(ddModelData, meshes, ASSETS_CONTAINER_MAX_SIZE)
// ddSkeleton
ASSET_CREATE(ddSkeleton, skeletons, ASSETS_CONTAINER_MIN_SIZE)
// ddSkeletonPose
ASSET_CREATE(ddSkeletonPose, poses, ASSETS_CONTAINER_MAX_SIZE)
// ddTex2D
ASSET_CREATE(ddTex2D, textures, ASSETS_CONTAINER_MAX_SIZE)
// ddMat
ASSET_CREATE(ddMat, mats, ASSETS_CONTAINER_MAX_SIZE)
}  // namespace

ASSET_DEF(ddAgent, b_agents)
ASSET_DEF(ddCam, cams)
ASSET_DEF(ddLBulb, lights)
ASSET_DEF(ddModelData, meshes)
ASSET_DEF(ddSkeleton, skeletons)
ASSET_DEF(ddSkeletonPose, poses)
ASSET_DEF(ddTex2D, textures)
ASSET_DEF(ddMat, mats)

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
/// \brief Add agent to physics world
/// \param agent to add rigid body
/// \param mesh data containing bounding box information
/// \return True if rigid body is successfully added
bool add_rigid_body(ddAgent *agent, ddModelData *mdata);
// source <Muhammad Mobeen Movania::OpenGL Development Cookbook>
/// \brief Invert image along Y-axis
/// \param image Pointer to image on RAM
/// \param width
/// \param height
/// \param channels
void flip_image(unsigned char *image, const int width, const int height,
                const int channels);

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

void dd_assets_cleanup() {
  // clean up bullet physics bodies
  for (auto &idx : map_b_agents) {
    if (b_agents[idx.second].inst.body[0].bt_bbox) {
      delete b_agents[idx.second].inst.body[0].bt_bbox;
      b_agents[idx.second].inst.body[0].bt_bbox = nullptr;
    }
    // if (b_agents[idx.second].body.body) delete
    // b_agents[idx.second].body.body;
  }
  // cleanup any unfreed images
  for (auto &idx : map_textures) {
    for (unsigned i = 0; i < 6; i++) {
      if (textures[idx.second].image_info.image_data[i]) {
        SOIL_free_image_data(textures[idx.second].image_info.image_data[i]);
      }
    }
  }
}

int dd_assets_create_agent(lua_State *L) {
  parse_lua_events(L, fb);
  // get arguments and use them to create ddAgent
  const char *agent_id = fb.get_func_val<const char>("id");
  int64_t *mesh_id = fb.get_func_val<int64_t>("mesh");
  int64_t *sk_id = fb.get_func_val<int64_t>("skeleton");
  int64_t *p_id = fb.get_func_val<int64_t>("parent");

  ddAgent *new_agent = nullptr;
  if (agent_id) {
    size_t out_id = getCharHash(agent_id);

    // check if agent already exists
    new_agent = find_ddAgent(out_id);
    if (new_agent) {
      ddTerminal::f_post("Duplicate agent <%s>", agent_id);
      // return agent id
      lua_pushinteger(L, new_agent->id);
      return 1;
    }

    new_agent = spawn_ddAgent(out_id);
    if (new_agent) {
      // add mesh for render
      ddModelData *mdata = nullptr;
      if (mesh_id) {
        mdata = find_ddModelData((size_t)(*mesh_id));
        if (mdata) {
          // modify ModelIDs struct
          new_agent->mesh.resize(1);
          new_agent->mesh[0].model = mdata->id;
          // modify instance information
          new_agent->inst.inst_v3.resize(1);
          new_agent->inst.inst_v3[0] = glm::vec3(1.f);
          new_agent->inst.body.resize(1);
        } else {
          ddTerminal::f_post("[error]  Failed to find mesh <%ld>", *mesh_id);
        }
      }
      // add skeleton for animation
      if (sk_id) {
        ddSkeleton *sk = find_ddSkeleton((size_t)*sk_id);
        if (sk) {
          // create skeleton pose struct and log in agent
          ddSkeletonPose *skpose = spawn_ddSkeletonPose(new_agent->id);
          if (skpose) {
            skpose->sk_id = sk->id;
            skpose->global_mat = sk->global_mat;
            skpose->local_pose.resize(sk->bones.size());
            skpose->global_pose.resize(sk->bones.size());
            new_agent->mesh[0].sk_flag = true;
          } else {
            ddTerminal::f_post("[error]  Failed to find skeleton <%ld>",
                               *sk_id);
          }
        }
      }
      // add parent object
      if (p_id) {
        ddAgent *p_agent = find_ddAgent((size_t)*p_id);
        if (p_agent) {
          // set parent id
          new_agent->parent.parent_id = p_agent->id;
          new_agent->parent.parent_set = true;
        } else {
          ddTerminal::f_post("[error]  Failed to find parent <%ld>", *p_id);
        }
      }
      // add ddBody to agent and then agent to world
      add_rigid_body(new_agent, mdata);
      // return agent id
      lua_pushinteger(L, new_agent->id);
      return 1;
    }
    ddTerminal::f_post("[error]Failed to create new agent <%s>", agent_id);
  }
  return 0;
}

int dd_assets_create_mesh(lua_State *L) {
  parse_lua_events(L, fb);
  ddModelData *new_mdata = nullptr;
  // get arguments and use them to create ddModelData
  const char *file = fb.get_func_val<const char>("path");
  if (file) {
    cbuff<256> f = file;
    // check file extension
    if (f.contains(".ddm")) {
      new_mdata = load_ddm(file);
      // return key of new mesh back up to script if created
      if (new_mdata) {
        lua_pushinteger(L, new_mdata->id);
        return 1;
      }
    }
    ddTerminal::f_post("[error]Failed to create new mesh <%s>", file);
  }
  ddTerminal::f_post("[error]Failed to create new mesh");
  return 0;
}

int dd_assets_create_cam(lua_State *L) {
  parse_lua_events(L, fb);
  // get arguments and use them to create ddCam
  ddCam *new_cam = nullptr;
  // get arguments
  const char *id = fb.get_func_val<const char>("id");
  if (id) {
    // check if camera already exists otherwise allocate
    size_t cam_id = getCharHash(id);
    new_cam = find_ddCam(cam_id);
    if (new_cam) {
      ddTerminal::f_post("Duplicate camera <%s>", id);
    } else {
      new_cam = spawn_ddCam(cam_id);
    }

    if (new_cam) {
      lua_pushinteger(L, new_cam->id);
      return 1;
    }
    ddTerminal::f_post("[error]Failed to create new camera <%s>", id);
  }
  ddTerminal::f_post("[error]Failed to create new camera");
  return 0;
}

int dd_assets_create_light(lua_State *L) {
  parse_lua_events(L, fb);
  // get arguments and use them to create ddLBulb
  ddLBulb *new_bulb = nullptr;
  // get arguments
  const char *id = fb.get_func_val<const char>("id");
  if (id) {
    // check if light already exist otherwise create new light
    size_t light_id = getCharHash(id);
    new_bulb = find_ddLBulb(light_id);
    if (new_bulb) {
      ddTerminal::f_post("Duplicate light <%s>", id);
    } else {
      new_bulb = spawn_ddLBulb(light_id);
    }

    if (new_bulb) {
      lua_pushinteger(L, new_bulb->id);
      return 1;
    }
    ddTerminal::f_post("[error]Failed to create new light <%s>", id);
  }
  ddTerminal::f_post("[error]Failed to create new light");
  return 0;
}

ddModelData *load_ddm(const char *filename) {
  /// \brief Lambda to get uint from string
  auto getUint = [](const char *str) {
    return (unsigned)strtoul(str, nullptr, 10);
  };
  ddModelData *mdata = nullptr;
  ddIO io_handle;
  dd_array<DDM_Data> out_data;
  dd_array<FbxEData> edata;
  dd_array<obj_mat> mats;
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
    ddMat *mat = create_material(mats[idx]);
    if (mat) {  // set mesh's material id
      mdata->mesh_info[i].mat_id = mat->id;
    } else {
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

ddTex2D *create_tex2D(const char *path, const char *img_id) {
  // check if texture already exists
  size_t tex_id = getCharHash(img_id);
  ddTex2D *new_tex = find_ddTex2D(tex_id);
  if (new_tex) {
    ddTerminal::f_post("Duplicate texture <%s>", img_id);
    return new_tex;
  }

  ImageInfo img_info;

  // find and load image to RAM
  img_info.path[0] = path;
  img_info.image_data[0] =
      SOIL_load_image(path, &img_info.width, &img_info.height,
                      &img_info.channels, SOIL_LOAD_RGBA);
  if (!img_info.image_data[0]) {
    ddTerminal::f_post("[error]create_tex2D::Failed to open image: %s", path);
    return new_tex;
  }
  // flip image (SOIL loads images inverted) and delete temp img
  flip_image(img_info.image_data[0], img_info.width, img_info.height,
             img_info.channels);

  // create texture object and assign img_info
  new_tex = spawn_ddTex2D(tex_id);
  if (!new_tex) {  // failed to allocate
    ddTerminal::f_post("[error]create_tex2D::Failed to create ddTex2D object");
    return new_tex;
  }
  new_tex->image_info = std::move(img_info);

  return new_tex;
}

bool add_rigid_body(ddAgent *agent, ddModelData *mdata) {
  if (!agent) return false;

  // set up bounding box
  glm::vec3 bb_max = glm::vec3(0.1f, 0.1f, 0.1f);
  glm::vec3 bb_min = glm::vec3(-0.1f, -0.1f, -0.1f);
  if (mdata) {
    bb_max = mdata->mesh_info[0].bb_max;
    bb_min = mdata->mesh_info[0].bb_min;
  }
  float h_width = (bb_max.x - bb_min.x) * 0.5f;
  float h_height = (bb_max.y - bb_min.y) * 0.5f;
  float h_depth = (bb_max.z - bb_min.z) * 0.5f;
  agent->inst.body[0].bt_bbox = new btBoxShape(
      btVector3(btScalar(h_width), btScalar(h_height), btScalar(h_depth)));

  // set up rigid body constructor
  btTransform transform;
  transform.setIdentity();
  transform.setOrigin(btVector3(0, 0, 0));

  // rigidbody is dynamic if and only if mass is non zero, otherwise static
  btScalar mass(0.);
  bool isDynamic = (mass != 0.f);
  btVector3 localInertia(0, 0, 0);
  if (isDynamic)
    agent->inst.body[0].bt_bbox->calculateLocalInertia(mass, localInertia);

  // set up rigid body
  // using motionstate is optional, it provides interpolation capabilities, and
  // only synchronizes 'active' objects
  btDefaultMotionState *myMotionState = new btDefaultMotionState(transform);
  btRigidBody::btRigidBodyConstructionInfo rbInfo(
      mass, myMotionState, agent->inst.body[0].bt_bbox, localInertia);
  agent->inst.body[0].bt_bod = new btRigidBody(rbInfo);

  // add to world
  p_world->addRigidBody(agent->inst.body[0].bt_bod);
  return true;
}

void flip_image(unsigned char *image, const int width, const int height,
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
