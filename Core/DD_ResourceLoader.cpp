#include "DD_ResourceLoader.h"
#include "DD_FileIO.h"
#include "DD_OBJ_Parser.h"
#include "DD_Terminal.h"

// parsing specific variables
namespace {
const std::string whitespace = " \t", dot = ".", slash = "/";

// std::map<buffer16, int> index_buffer;
std::string TrimWhiteSpace(const std::string& str) {
  const size_t strBegin = str.find_first_not_of(whitespace);
  if (strBegin == std::string::npos) {
    return str;  // no leading whitespaces
  }
  return str.substr(strBegin);
}

std::string GetLineID(const std::string& str) {
  const size_t strBegin = str.find_first_of(whitespace);
  if (strBegin == std::string::npos) {
    return str;  // no leading whitespaces
  }
  return str.substr(0, strBegin);
}

obj_vec3 GetFloatData(const std::string str) {
  obj_vec3 v_data;
  std::string tempData = str;

  size_t numEnd = tempData.find_first_of(whitespace);
  u8 index = 0;
  while (numEnd != std::string::npos) {
    v_data.data[index] = float(atof((tempData.substr(0, numEnd)).c_str()));
    index += 1;
    tempData = tempData.substr(numEnd + 1);  // cut number out
    numEnd = tempData.find_first_of(whitespace);
  }
  v_data.data[index] = float(atof(tempData.c_str()));

  return v_data;
}

void skipPastSpace(char*& str) {
  while (*str != ' ' && *str) {
    str++;
  }
  str++;
}
}

struct meshStuff {
  std::string id, path, mtl;
};

/// \brief Object to store vertex data from DDM
struct FbxVertData {
  dd_array<glm::vec3> v;
  dd_array<glm::vec3> n;
  dd_array<glm::vec3> t;
  dd_array<glm::vec2> u;
  dd_array<glm::vec4> j;
  dd_array<glm::vec4> b;
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
struct FbxEboData {
  dd_array<GLuint> idxs;
  unsigned mat_idx;
};

DD_Texture2D* CreateTexture_OBJMAT(DD_Resources* res, std::string dir,
                                   std::string tex_path);
dd_array<MeshData> getMeshData(FbxVertData& vdata, dd_array<FbxEboData>& edata);
void populateFbxVertexData(FbxVertData& vdata, std::ifstream& _stream);
void populateFbxEboData(FbxEboData& edata, std::ifstream& _stream);
obj_mat getMatData(std::ifstream& _stream);
;
glm::mat4 UpdateInstance(const int index, DD_Resources* rsrc, float thisFrame);
void calcDeltaBoneTransform(DD_Skeleton* sk, bool* visited, const unsigned idx);

void DD_Resources::LoadDefaults() {
  obj_mat m = obj_mat();
  m.ID = "default";
  m.diffuseRaw = obj_vec3({0.5f, 0.5f, 0.5f});
  materials[mtl_counter] = std::move(ResSpace::createMaterial(this, m));
  mtl_counter += 1;

  DD_Terminal::post("[status] Loading primitive resources...\n");
  cbuff<128> path;

  path.format("%s%s", MESH_DIR, "primitives/cube.ddm");
  DD_Model* mdl = ResSpace::loadModel_DDM(this, "cube_prim", path.str());
  if (!mdl) {
    DD_Terminal::post("[error] Cube primitive not loaded");
  }

  path.format("%s%s", MESH_DIR, "primitives/plane.ddm");
  mdl = ResSpace::loadModel_DDM(this, "plane_prim", path.str());
  if (!mdl) {
    DD_Terminal::post("[error] Plane primitive not loaded");
  }

  path.format("%s%s", MESH_DIR, "primitives/sphere_10.ddm");
  mdl = ResSpace::loadModel_DDM(this, "sphere_prim", path.str());
  if (!mdl) {
    DD_Terminal::post("[error] Sphere primitive not loaded");
  }

  path.format("%s%s", MESH_DIR, "primitives/cylinder_10.ddm");
  mdl = ResSpace::loadModel_DDM(this, "cylinder_prim", path.str());
  if (!mdl) {
    DD_Terminal::post("[error] Cylinder primitive not loaded");
  }

  path.format("%s%s", MESH_DIR, "primitives/tempmesh.ddm");
  mdl = ResSpace::loadModel_DDM(this, "noModel", path.str());
  if (!mdl) {
    DD_Terminal::post("[error] tempmesh primitive not loaded");
  }
}

namespace ResSpace {

RES_DEF_ALL_PTR(DD_Agent, agents, m_num_agents)
RES_DEF_ALL(DD_Model, models, mdl_counter)
RES_DEF_ALL(DD_Material, materials, mtl_counter)
RES_DEF_ALL(DD_Camera, cameras, cam_counter)
RES_DEF_ALL(DD_Light, lights, light_counter)
RES_DEF_ALL(DD_Texture2D, textures, tex_counter)
RES_DEF_ALL(DD_Skybox, skyboxes, skybox_counter)
RES_DEF_ALL(DD_Emitter, emitters, emitter_counter)
RES_DEF_ALL(DD_Cloth, clothes, clothes_counter)
RES_DEF_ALL(DD_Water, water, water_counter)
RES_DEF_ALL(DD_LineAgent, lines, l_agent_counter)
RES_DEF_ALL(DD_AIObject, ai_s, ai_obj_counter)
RES_DEF_ALL(AI_Agent, ai_agents, ai_agent_counter)
RES_DEF_ALL(DD_Shader, shaders, shader_counter)
RES_DEF_ALL(DD_Skeleton, skeletons, skeleton_counter)
RES_DEF_ALL(DD_ModelSK, sk_models, sk_mdl_counter)
RES_DEF_ALL(DD_AnimClip, clips, clip_counter)

void Load(DD_Resources* res, const char* fileName) {
  res->LoadDefaults();
  std::string res_path = std::string(fileName) + ".ddres";

  meshStuff meshes[100];
  int mesh_count = 0;

  std::ifstream file(res_path);
  if (file.good()) {
    std::stringstream analysisBuf;
    analysisBuf << file.rdbuf();
    file.close();
    std::string line, subline;

    while (std::getline(analysisBuf, line)) {
      // remove whitespace
      line = TrimWhiteSpace(line);
      std::string lineID = GetLineID(line);

      if (lineID == "<Agent>") {
        // grab agent info
        std::getline(analysisBuf, line);
        std::string agent_ID = line.substr(3);  // ID
        std::getline(analysisBuf, line);
        std::string inst_count = line.substr(5);  // INST

        DD_Agent* agent = ResSpace::getNewDD_Agent(res, agent_ID.c_str());
        // INST info---> MODEL, N_F, POS, ROT, SCL
        std::string tempStr[5];
        const size_t num_inst = atoi(inst_count.c_str());
        for (size_t i = 0; i < num_inst; i++) {
          std::getline(analysisBuf, line);
          tempStr[0] = line.substr(6);  // model ID
          std::getline(analysisBuf, line);
          tempStr[1] = line.substr(4);  // N_F
          obj_vec3 near_far = GetFloatData(tempStr[1]);
          std::getline(analysisBuf, line);
          tempStr[2] = line.substr(4);  // POS
          obj_vec3 _p = GetFloatData(tempStr[2]);
          std::getline(analysisBuf, line);
          tempStr[3] = line.substr(4);  // ROT
          obj_vec3 _r = GetFloatData(tempStr[3]);
          std::getline(analysisBuf, line);
          tempStr[4] = line.substr(4);  // SCL
          obj_vec3 _s = GetFloatData(tempStr[4]);
          // create new model
          agent->AddModel(tempStr[0].c_str(), near_far.x(), near_far.y());
          agent->UpdatePosition(glm::vec3(_p.x(), _p.y(), _p.z()));
          agent->UpdateRotation(glm::quat(glm::vec3(_r.x(), _r.y(), _r.z())));
          agent->UpdateScale(glm::vec3(_s.x(), _s.y(), _s.z()));
        }
      }
      if (lineID == "<Model>") {
        res->mdl_counter += 1;
        mesh_count += 1;
        std::string tempStr[3];
        // grab model info
        for (size_t i = 0; i < 3; i++) {
          std::getline(analysisBuf, line);
          switch (i) {
            case 0:
              // ID
              tempStr[i] = line.substr(3);
              break;
            case 1:
              // MTL
              tempStr[i] = line.substr(4);
              break;
            case 2:
              // PATH
              tempStr[i] = line.substr(5);
              break;
            default:
              break;
          }
        }
        const std::string path = MESH_DIR + tempStr[2];
        const std::string mtlname = (tempStr[1] == "<>") ? "" : tempStr[1];

        meshes[res->mdl_counter - 1].id = tempStr[0];
        meshes[res->mdl_counter - 1].path = path;
        meshes[res->mdl_counter - 1].mtl = mtlname;
      }
      if (lineID == "<Texture2D>") {
        res->tex_counter += 1;
        std::string tempStr[3];
        // grab texture info
        for (size_t i = 0; i < 3; i++) {
          std::getline(analysisBuf, line);
          switch (i) {
            case 0:
              // ID
              tempStr[i] = line.substr(3);
              break;
            case 1:
              // EXT
              tempStr[i] = line.substr(4);
              break;
            case 2:
              // PATH
              tempStr[i] = line.substr(5);
              break;
            default:
              break;
          }
        }
        res->textures[res->tex_counter - 1] = DD_Texture2D();
        res->textures[res->tex_counter - 1].path = TEX_DIR + tempStr[2];
        // < check extension to determine image type >
        res->textures[res->tex_counter - 1].m_ID = tempStr[0];
      }
      if (lineID == "<Shader>") {
        res->shader_counter += 1;
        std::string tempStr[5];
        // grab shader info
        for (size_t i = 0; i < 5; i++) {
          // ID, VS, FS, GS, CS
          std::getline(analysisBuf, line);
          tempStr[i] = line.substr(3);
        }
        res->shaders[res->shader_counter - 1] = DD_Shader();
        DD_Shader& sh = res->shaders[res->shader_counter - 1];
        sh.m_ID = tempStr[0];
        sh.m_vs = (tempStr[1] == "<>") ? "" : tempStr[1];
        sh.m_fs = (tempStr[2] == "<>") ? "" : tempStr[2];
        sh.m_gs = (tempStr[3] == "<>") ? "" : tempStr[3];
        sh.m_cs = (tempStr[4] == "<>") ? "" : tempStr[4];
      }
      if (lineID == "<Light>") {
        res->light_counter += 1;
        std::string tempStr[7];
        // grab light info
        for (size_t i = 0; i < 7; i++) {
          std::getline(analysisBuf, line);
          switch (i) {
            case 0:
              // ID
              tempStr[i] = line.substr(3);
              break;
            case 1:
              // POS
              tempStr[i] = line.substr(4);
              break;
            case 2:
              // DIR
              tempStr[i] = line.substr(4);
              break;
            case 3:
              // TYPE
              tempStr[i] = line.substr(5);
              break;
            case 4:
              // COLOR
              tempStr[i] = line.substr(6);
              break;
            case 5:
              // LINEAR
              tempStr[i] = line.substr(7);
              break;
            case 6:
              // QUAD
              tempStr[i] = line.substr(5);
              break;
            default:
              break;
          }
        }
        obj_vec3 pos = GetFloatData(tempStr[1]);
        obj_vec3 dir = GetFloatData(tempStr[2]);
        obj_vec3 col = GetFloatData(tempStr[4]);
        LightType l_t = LightType(std::atoi(tempStr[3].c_str()));

        res->lights[res->light_counter - 1] = DD_Light();

        res->lights[res->light_counter - 1].m_ID = tempStr[0];
        res->lights[res->light_counter - 1]._position.x = pos.x();
        res->lights[res->light_counter - 1]._position.y = pos.y();
        res->lights[res->light_counter - 1]._position.z = pos.z();
        res->lights[res->light_counter - 1].m_direction.x = dir.x();
        res->lights[res->light_counter - 1].m_direction.y = dir.y();
        res->lights[res->light_counter - 1].m_direction.z = dir.z();
        res->lights[res->light_counter - 1].m_color.r = col.x();
        res->lights[res->light_counter - 1].m_color.g = col.y();
        res->lights[res->light_counter - 1].m_color.b = col.z();
        res->lights[res->light_counter - 1].m_type = l_t;

        // Linear
        if (tempStr[5] != "<>") {
          res->lights[res->light_counter - 1].m_linear =
              float(std::atof(tempStr[5].c_str()));
        }
        // Quadratic
        if (tempStr[6] != "<>") {
          res->lights[res->light_counter - 1].m_quadratic =
              float(std::atof(tempStr[6].c_str()));
        }
      }
      if (lineID == "<Camera>") {
        res->cam_counter += 1;
        std::string tempStr[7];
        // grab camera info
        for (size_t i = 0; i < 7; i++) {
          std::getline(analysisBuf, line);
          switch (i) {
            case 0:
              // ID
              tempStr[i] = line.substr(3);
              break;
            case 1:
              // FAR
              tempStr[i] = line.substr(4);
              break;
            case 2:
              // NEAR
              tempStr[i] = line.substr(5);
              break;
            case 3:
              // FOVH
              tempStr[i] = line.substr(5);
              break;
            case 4:
              // ACTIVE
              tempStr[i] = line.substr(7);
              break;
            case 5:
              // POS
              tempStr[i] = line.substr(4);
              break;
            case 6:
              // ROT
              tempStr[i] = line.substr(4);
              break;
            default:
              break;
          }
        }
        obj_vec3 cam_pos = GetFloatData(tempStr[5]);
        obj_vec3 cam_rot = GetFloatData(tempStr[6]);

        res->cameras[res->cam_counter - 1] = DD_Camera();

        res->cameras[res->cam_counter - 1].m_ID = tempStr[0];
        int camflag = std::atoi(tempStr[4].c_str());
        res->cameras[res->cam_counter - 1].active =
            (camflag != 0) ? true : false;
        // Far
        if (tempStr[1] != "<>") {
          res->cameras[res->cam_counter - 1].far_plane =
              float(std::atof(tempStr[1].c_str()));
        }
        // Near
        if (tempStr[2] != "<>") {
          res->cameras[res->cam_counter - 1].near_plane =
              float(std::atof(tempStr[2].c_str()));
        }
        // FOV H
        if (tempStr[3] != "<>") {
          res->cameras[res->cam_counter - 1].fov_h =
              float(std::atof(tempStr[3].c_str()));
        }
        // Position and rotation
        glm::vec4 pos = glm::vec4(cam_pos.x(), cam_pos.y(), cam_pos.z(), 1.0f);
        glm::quat cam_q =
            glm::quat(glm::vec3(cam_rot.x(), cam_rot.y(), cam_rot.z()));
        res->cameras[res->cam_counter - 1].updateCamera(pos, cam_q);
      }
      if (lineID == "<DD_Skybox>") {
        res->skybox_counter += 1;
        std::string tempStr[7];
        // grab skybox info
        for (size_t i = 0; i < 7; i++) {
          std::getline(analysisBuf, line);
          switch (i) {
            case 0:
              // ID
              tempStr[i] = line.substr(3);
              break;
            case 1:
              // RT
              tempStr[i] = (line.substr(3) == "<>") ? "" : line.substr(3);
              break;
            case 2:
              // LT
              tempStr[i] = (line.substr(3) == "<>") ? "" : line.substr(3);
              break;
            case 3:
              // TP
              tempStr[i] = (line.substr(3) == "<>") ? "" : line.substr(3);
              break;
            case 4:
              // BM
              tempStr[i] = (line.substr(3) == "<>") ? "" : line.substr(3);
              break;
            case 5:
              // FT
              tempStr[i] = (line.substr(3) == "<>") ? "" : line.substr(3);
              break;
            case 6:
              // BK
              tempStr[i] = (line.substr(3) == "<>") ? "" : line.substr(3);
              break;
            default:
              break;
          }
        }
        res->skyboxes[res->skybox_counter - 1] = DD_Skybox();

        res->skyboxes[res->skybox_counter - 1].m_ID = tempStr[0];
        res->skyboxes[res->skybox_counter - 1].right = tempStr[1];
        res->skyboxes[res->skybox_counter - 1].left = tempStr[2];
        res->skyboxes[res->skybox_counter - 1].top = tempStr[3];
        res->skyboxes[res->skybox_counter - 1].bottom = tempStr[4];
        res->skyboxes[res->skybox_counter - 1].front = tempStr[5];
        res->skyboxes[res->skybox_counter - 1].back = tempStr[6];
      }
    }
    // process meshes and materials
    int start = (int)res->mdl_counter - mesh_count;
#pragma omp parallel for
    for (int i = start; i < (int)res->mdl_counter; i++) {
      res->models[i] = std::move(
          loadModel(res, meshes[i].path.c_str(), meshes[i].mtl.c_str()));
      res->models[i].m_ID = meshes[i].id.c_str();
      // save pointer to material
    }
  } else {
    fprintf(stderr, "Could not open asset: %s\n", res_path.c_str());
  }
}

// Create new material from obj_mat
DD_Material createMaterial(DD_Resources* res, obj_mat& matbuff) {
  DD_Material mat = DD_Material();
  DD_Texture2D* tex = nullptr;

  mat.m_ID = matbuff.ID;
  mat.m_base_color = glm::vec4(matbuff.diffuseRaw.x(), matbuff.diffuseRaw.y(),
                               matbuff.diffuseRaw.z(), 1.0f);
  if (matbuff.albedo_flag) {
    tex = CreateTexture_OBJMAT(res, matbuff.directory, matbuff.albedo_tex);
    mat.AddTexture(tex, TextureType::ALBEDO);
  }
  if (matbuff.spec_flag) {
    tex = CreateTexture_OBJMAT(res, matbuff.directory, matbuff.specular_tex);
    mat.AddTexture(tex, TextureType::SPECULAR);
  }
  if (matbuff.ao_flag) {
    tex = CreateTexture_OBJMAT(res, matbuff.directory, matbuff.ao_tex);
    mat.AddTexture(tex, TextureType::AO);
  }
  if (matbuff.norm_flag) {
    tex = CreateTexture_OBJMAT(res, matbuff.directory, matbuff.normal_tex);
    mat.AddTexture(tex, TextureType::NORMAL);
  }
  if (matbuff.rough_flag) {
    tex = CreateTexture_OBJMAT(res, matbuff.directory, matbuff.roughness_tex);
    mat.AddTexture(tex, TextureType::ROUGH);
  }
  if (matbuff.metal_flag) {
    tex = CreateTexture_OBJMAT(res, matbuff.directory, matbuff.metalness_tex);
    mat.AddTexture(tex, TextureType::METAL);
  }
  if (matbuff.emit_flag) {
    tex = CreateTexture_OBJMAT(res, matbuff.directory, matbuff.emissive_tex);
    mat.AddTexture(tex, TextureType::EMISSIVE);
  }
  // set multiplier material
  if (matbuff.multiplier) {
    mat.SetMultiplierMaterial(TextureType::ALBEDO);
  }

  DD_Terminal::post("Created material->" + mat.m_ID + "\n");
  return mat;
}

DD_Model loadModel(DD_Resources* res, const char* obj_path,
                   const char* mtlName) {
  DD_Model model = DD_Model();
  ObjAsset asset = ObjAsset();
  ObjAssetParser::ParseFlag parseResult;
  parseResult = ObjAssetParser::PreProcess(asset, obj_path, mtlName);
  if (parseResult.success && !parseResult.ddMesh) {
    ObjAssetParser::FormatForOpenGL(asset);

    // initialize model's containers
    model.meshes.resize(asset.meshes.size());
    model.materials.resize(asset.meshes.size());
    model.VAO.resize(asset.meshes.size());
    model.VBO.resize(asset.meshes.size());
    model.EBO.resize(asset.meshes.size());
    model.instVBO.resize(asset.meshes.size());
    model.instColorVBO.resize(asset.meshes.size());

    model.meshes = std::move(asset.meshes);
    model.directory = asset.info.directory;

    // Create materials (if new)
    for (size_t i = 0; i < asset.info.mat_buffer.size(); i++) {
      if (asset.info.mat_buffer[i].ID.compare("default") == 0) {
        continue;  // skip creation (already present in materials)
      }
      if (asset.info.mat_buffer[i].ID.compare("") == 0) {
        continue;  // skip creation (mesh with no material)
      }
      obj_mat& _mat = asset.info.mat_buffer[i];
      res->materials[res->mtl_counter] =
          std::move(ResSpace::createMaterial(res, _mat));
      res->mtl_counter += 1;
    }

    for (size_t i = 0; i < model.meshes.size(); i++) {
      std::string mat_ID = model.meshes[i].material_ID;
      if (mat_ID.compare("default") == 0) {
        // set index of material to 0
        model.materials[i] = 0;
      } else {
        // set index of material
        int index = ResSpace::getDD_Material_idx(res, mat_ID.c_str());
        if (index == -1) {
          model.meshes[i].material_ID = "default";
          model.materials[i] = 0;
        } else {
          model.materials[i] = index;
        }
      }
    }  // end of for loop
    return model;
  } else if (parseResult.success && parseResult.ddMesh) {
    // load .ddmesh
    ObjAssetParser::LoadDDMesh(asset);

    // initialize model's containers
    model.meshes.resize(asset.meshes.size());
    model.materials.resize(asset.meshes.size());
    model.VAO.resize(asset.meshes.size());
    model.VBO.resize(asset.meshes.size());
    model.EBO.resize(asset.meshes.size());
    model.instVBO.resize(asset.meshes.size());
    model.instColorVBO.resize(asset.meshes.size());

    model.meshes = std::move(asset.meshes);
    model.directory = asset.info.directory;

    for (size_t i = 0; i < model.meshes.size(); i++) {
      std::string mat_ID = model.meshes[i].material_ID;
      if (mat_ID.compare("default") == 0) {
        // set index of material to 0
        model.materials[i] = 0;
      } else {
        // set index of material
        int index = ResSpace::getDD_Material_idx(res, mat_ID.c_str());
        if (index == -1) {
          model.meshes[i].material_ID = "default";
          model.materials[i] = 0;
        } else {
          model.materials[i] = index;
        }
      }
    }  // end of for loop
    return model;
  } else {
    return DD_Model();
  }
}

DD_ModelSK* loadSkinnedModel(DD_Resources* res, const char* new_mdl_id,
                             const char* skeleton_id,
                             const dd_array<MeshData>& md) {
  DD_ModelSK* mdlsk = nullptr;
  // check if model already exists
  mdlsk = ResSpace::findDD_ModelSK(res, new_mdl_id);
  if (mdlsk) {
    DD_Terminal::f_post("loadSkinnedModel::<%s> already exists.", new_mdl_id);
  } else {
    mdlsk = ResSpace::getNewDD_ModelSK(res, new_mdl_id);
    mdlsk = static_cast<DD_ModelSK*>(loadModel_MD(res, new_mdl_id, md, mdlsk));
    if (!mdlsk) {
      DD_Terminal::f_post("[error] loadSkinnedModel::<%s>::null", new_mdl_id);
      return mdlsk;
    }
  }

  // add skeleton
  mdlsk->m_finalPose.m_skeletonID.set(skeleton_id);
  DD_Skeleton* sk = ResSpace::findDD_Skeleton(res, skeleton_id);
  if (sk) {
    mdlsk->m_finalPose.m_globalMat = sk->m_globalMat;
    mdlsk->m_finalPose.m_localPose.resize(sk->m_bones.size());
    mdlsk->m_finalPose.m_globalPose.resize(sk->m_bones.size());
    mdlsk->m_finalPose.m_invBPs.resize(sk->m_bones.size());
    for (unsigned i = 0; i < sk->m_bones.size(); i++) {
      mdlsk->m_finalPose.m_invBPs[i] = sk->m_bones[i].m_invBP;
    }
  }

  return mdlsk;
}

/// \brief Create DD_ModelSK file from ddm and dda file
DD_ModelSK* loadSkinnedModel(DD_Resources* res, const char* new_mdl_id,
                             const char* ddm_path, const char* skeleton_id,
                             const char* ddb_path) {
  DD_ModelSK* mdlsk = nullptr;
  DD_Skeleton* sk = nullptr;
  // check if skeleton already exists
  sk = findDD_Skeleton(res, skeleton_id);
  if (!sk) {
    if (!loadDDB(res, ddb_path, skeleton_id)) {
      DD_Terminal::f_post("[error] loadSkinnedModel::Invalid ddb file: %s",
                          ddb_path);
      return mdlsk;
    }
  }
  // load ddm or ddg file if provided
  dd_array<MeshData> mesh;
  if (*ddm_path) {
    cbuff<512> path(ddm_path);
    if (path.contains(".ddm")) {
      mesh = loadDDM(ddm_path);
    } else if (path.contains(".ddg")) {
      mesh = loadDDG(ddm_path);
    }
  }
  mdlsk = loadSkinnedModel(res, new_mdl_id, skeleton_id, mesh);

  return mdlsk;
}

/// \brief Load model directly from MeshData
DD_Model* loadModel_MD(DD_Resources* res, const char* id,
                       const dd_array<MeshData>& md, DD_Model* mdl) {
  DD_Model* model = nullptr;
  model = mdl ? mdl : ResSpace::getNewDD_Model(res, id);

  // resize containers
  model->meshes.resize(md.size());
  model->materials.resize(md.size());
  model->VAO.resize(md.size());
  model->VBO.resize(md.size());
  model->EBO.resize(md.size());
  model->instVBO.resize(md.size());
  model->instColorVBO.resize(md.size());

  model->meshes = md;

  // Create materials (if new)
  for (size_t i = 0; i < md.size(); i++) {
    if (md[i].material_ID.compare("default") == 0) {
      model->materials[i] = 0;
      continue;  // skip creation (already present in materials)
    }
    if (md[i].material_ID.compare("") == 0) {
      model->materials[i] = 0;
      continue;  // skip creation (mesh with no material)
    }
    obj_mat& mat = md[i].material_info;
    res->materials[res->mtl_counter] =
        std::move(ResSpace::createMaterial(res, mat));
    res->mtl_counter += 1;
  }

  for (size_t i = 0; i < model->meshes.size(); i++) {
    std::string mat_ID = model->meshes[i].material_ID;
    // set index of material if doesn't exist
    int index = ResSpace::getDD_Material_idx(res, mat_ID.c_str());
    if (index == -1) {
      model->meshes[i].material_ID = "default";
      model->materials[i] = 0;
    } else {
      model->materials[i] = index;
    }
  }
  return model;
}

/// \brief Creates DD_Model* with ddm mesh path
DD_Model* loadModel_DDM(DD_Resources* res, const char* id,
                        const char* ddm_path) {
  cbuff<512> path(ddm_path);
  if (!path.contains(".ddm")) {
    DD_Terminal::f_post("[error] Not valid ddm file: %s", ddm_path);
    return nullptr;
  }

  dd_array<MeshData> mesh = loadDDM(ddm_path);
  if (mesh.size() > 0) {
    return loadModel_MD(res, id, mesh);
  }
  DD_Terminal::f_post("[error] <%s> failed to load", ddm_path);

  return nullptr;
}

/// \brief Load DDS skeleton file
DD_Skeleton* loadDDB(DD_Resources* res, const char* path, const char* id) {
  DD_IOhandle io_handle;
  DD_Skeleton* skele = nullptr;

  // check if already exists
  skele = findDD_Skeleton(res, id);
  if (skele) {
    DD_Terminal::f_post("loadDDB::<%s> already exists", id);
    return skele;
  }
  cbuff<512> path_check(path);
  if (!path_check.contains(".ddb")) {
    DD_Terminal::f_post("[error] Not valid ddb file: %s", path);
    return nullptr;
  }

  if (io_handle.open(path, DD_IOflag::READ)) {
    skele = getNewDD_Skeleton(res, id);
    cbuff<64> mybuff;
    const char* nxtLine = io_handle.readNextLine();
    while (nxtLine) {
      mybuff.set(nxtLine);
      // number of bones
      if (mybuff.compare("<size>") == 0) {
        nxtLine = io_handle.readNextLine();
        unsigned size = std::strtoul(nxtLine, nullptr, 10);
        skele->m_bones.resize(size);
      }
      // global array
      if (mybuff.compare("<global>") == 0) {
        nxtLine = io_handle.readNextLine();
        nxtLine += 2;  // position
        glm::vec3 pos = getVec3f(nxtLine);
        nxtLine = io_handle.readNextLine();
        nxtLine += 2;  // rotation
        glm::vec3 rot = getVec3f(nxtLine);
        nxtLine = io_handle.readNextLine();
        nxtLine += 2;  // scale
        glm::vec3 scale = getVec3f(nxtLine);

        glm::mat4 global_mat = createMatrix(pos, rot, scale);
        skele->m_globalMat = global_mat;
        // printGlmMat(global_mat);
      }
      // joints
      if (mybuff.compare("<joint>") == 0) {
        nxtLine = io_handle.readNextLine();
        auto tkns = StrSpace::tokenize512<64>(nxtLine, " ");
        // set id and parent index
        unsigned idx = strtoul(tkns[1].str(), nullptr, 10);
        unsigned parent_idx = strtoul(tkns[2].str(), nullptr, 10);
        skele->m_bones[idx].m_ID.set(tkns[0].str());
        skele->m_bones[idx].m_parent = (u8)parent_idx;
        // get inverse bind pose matrix (object to joint space)
        nxtLine = io_handle.readNextLine();
        nxtLine += 2;  // position
        glm::vec3 pos = getVec3f(nxtLine);
        nxtLine = io_handle.readNextLine();
        nxtLine += 2;  // rotation
        glm::vec3 rot = getVec3f(nxtLine);
        nxtLine = io_handle.readNextLine();
        nxtLine += 2;  // scale
        glm::vec3 scale = getVec3f(nxtLine);

        glm::mat4 jnt_mat = createMatrix(pos, rot, scale);
        skele->m_bones[idx].m_invBP = glm::inverse(jnt_mat);
      }
      nxtLine = io_handle.readNextLine();
    }
    // calculate delta bone transform of skeleton
    bool v_bin[MAX_JOINTS];
    for (unsigned i = 0; i < skele->m_bones.size(); i++) {
      v_bin[i] = false;
    }
    for (unsigned i = 0; i < skele->m_bones.size(); i++) {
      calcDeltaBoneTransform(skele, v_bin, i);
    }
  }
  return skele;
}

/// \brief Load animation from a dda file
/// \param res Resource bin pointer
/// \param path Path to dda file
/// \param id In-engine animation identifier
DD_AnimClip* loadDDA(DD_Resources* res, const char* path, const char* id) {
  DD_IOhandle io_handle;
  DD_AnimClip* a_clip = nullptr;

  // check if already exists
  a_clip = findDD_AnimClip(res, id);
  if (a_clip) {
    DD_Terminal::f_post("loadDDA::<%s> already exists", id);
    return a_clip;
  }
  cbuff<512> path_check(path);
  if (!path_check.contains(".dda")) {
    DD_Terminal::f_post("[error] Not valid dda file: %s", path);
    return nullptr;
  }

  if (io_handle.open(path, DD_IOflag::READ)) {
    a_clip = getNewDD_AnimClip(res, id);
    cbuff<128> mybuff;
    const char* val = nullptr;
    mybuff.format("Successfully opened %s", path);

    const char* nxtLine = io_handle.readNextLine();
    while (nxtLine) {
      mybuff.set(nxtLine);
      // framerate of clip
      if (mybuff.compare("<framerate>") == 0) {
        nxtLine = io_handle.readNextLine();
        float fr = std::strtof(nxtLine, nullptr);
        a_clip->fps = fr;
      }
      // buffer sizes
      if (mybuff.compare("<buffer>") == 0) {
        nxtLine = io_handle.readNextLine();

        if (*nxtLine == 'j') {  // total joints
          val = nxtLine + 2;
          unsigned long j = std::strtoul(val, nullptr, 10);
          a_clip->num_joints = (unsigned)j;
          nxtLine = io_handle.readNextLine();
        }
        if (*nxtLine == 'f') {  // total frames
          val = nxtLine + 2;
          unsigned long tf = std::strtoul(val, nullptr, 10);
          a_clip->num_frames = (unsigned)tf;
          a_clip->samples.resize(a_clip->num_frames);
          // calculate clip length
          a_clip->length = (float)a_clip->num_frames / a_clip->fps;
          a_clip->step_size = a_clip->length / a_clip->num_frames;
          for (unsigned i = 0; i < tf; i++) {  // resize storage bin
            a_clip->samples[i].m_pose.resize(a_clip->num_joints);
          }
          nxtLine = io_handle.readNextLine();
        }
      }
      // animation data
      if (mybuff.compare("<animation>") == 0) {
        nxtLine = io_handle.readNextLine();
        unsigned idx = 0;
        unsigned f_idx = 0;
        dd_array<DD_JointPose> output(a_clip->num_frames);

        if (*nxtLine == '-') {  // joint index
          val = nxtLine + 2;
          idx = (unsigned)std::strtoul(val, nullptr, 10);
          nxtLine = io_handle.readNextLine();
        }
        while (*nxtLine != '<') {  // keep reading till </animation>
          if (*nxtLine == 'r') {   // joint rotation
            val = nxtLine + 2;
            a_clip->samples[f_idx].m_pose[idx].m_rot = getQuat(val);
            // printf("r %u:%u %s\n", f_idx, idx, val);

            f_idx += 1;  // increment frame index
          }
          if (*nxtLine == 'p') {  // joint translation
            val = nxtLine + 2;
            a_clip->samples[f_idx - 1].m_pose[idx].m_trans = getVec3f(val);
            // printf("t %u:%u %s\n", f_idx, idx, val);
          }
          nxtLine = io_handle.readNextLine();
        }
      }
      nxtLine = io_handle.readNextLine();
    }
  }
  return a_clip;
}

/// \brief Import translated .ddm mesh file and load to RAM
/// \param filename .ddm file to open
dd_array<MeshData> loadDDM(const char* filename) {
  /// \brief Lambda to get uint from string
  auto getUint = [](const char* str) {
    return (unsigned)strtoul(str, nullptr, 10);
  };

  dd_array<FbxEboData> ebos;
  dd_array<obj_mat> _mats;
  FbxVertData _data;
  cbuff<64> name;
  unsigned mat_idx = 0;
  unsigned ebo_idx = 0;
  dd_array<MeshData> output;

  std::ifstream file(filename);
  if (file.good()) {
    char line[256];

    while (file.getline(line, sizeof(line))) {
      // check tag for information to parse
      if (strcmp("<name>", line) == 0) {
        file.getline(line, sizeof(line));
        name.set(line);
      }
      if (strcmp("<buffer>", line) == 0) {
        file.getline(line, sizeof(line));

        if (*line == 'v') {  // vertex buffer
          _data.setSize(getUint(&line[2]));
          file.getline(line, sizeof(line));
        }
        if (*line == 'e') {  // element buffer
          ebos.resize(getUint(&line[2]));
          file.getline(line, sizeof(line));
        }
        if (*line == 'm') {  // # of materials
          _mats.resize(getUint(&line[2]));
          file.getline(line, sizeof(line));
        }
      }
      if (strcmp("<vertex>", line) == 0) {
        populateFbxVertexData(_data, file);
      }
      if (strcmp("<ebo>", line) == 0) {
        populateFbxEboData(ebos[ebo_idx], file);
        ebo_idx += 1;
      }
      if (strcmp("<material>", line) == 0) {
        _mats[mat_idx] = getMatData(file);
        mat_idx += 1;
      }
    }  // end of while
    file.close();

    output = getMeshData(_data, ebos);
    for (size_t i = 0; i < output.size(); i++) {  // assign materials
      u32 idx = ebos[i].mat_idx;
      output[i].material_info = _mats[idx];
      output[i].material_ID = _mats[idx].ID;
    }
  }
  return output;
}

/// \brief Import translated .ddg mesh file and load to RAM
/// \param filename .ddg file to open
dd_array<MeshData> loadDDG(const char* filename) {
  DD_IOhandle io_handle;
  dd_array<MeshData> outmesh;

  if (io_handle.open(filename, DD_IOflag::READ)) {
    const char* nxtLine = io_handle.readNextLine();
    cbuff<512> mybuff;
    unsigned meshes_added = 0;

    while (nxtLine) {
      mybuff.set(nxtLine);
      // possible meshes
      if (mybuff.compare("<mesh>") == 0) {
        nxtLine = io_handle.readNextLine();
        dd_array<MeshData> in_mesh = loadDDM(nxtLine);
        if (in_mesh.size() != 0) {
          if (meshes_added == 0) {
            meshes_added += (unsigned)in_mesh.size();
            outmesh = std::move(in_mesh);
          } else {
            // add to outmesh bin
            unsigned idx = meshes_added;
            meshes_added += (unsigned)in_mesh.size();
            dd_array<MeshData> temp = std::move(outmesh);
            outmesh.resize(meshes_added);
            outmesh = temp;
            // copy
            for (unsigned i = 0; idx < meshes_added; idx++, i++) {
              outmesh[idx] = std::move(in_mesh[i]);
            }
          }
        }
      }
      nxtLine = io_handle.readNextLine();
    }
  }
  return outmesh;
}

// set and update screen attributes on cameras
void initCameras(DD_Resources* res, const float width, const float height) {
  for (size_t i = 0; i < res->cam_counter; i++) {
    res->cameras[i].scr_width = width;
    res->cameras[i].scr_height = height;
    res->cameras[i].updateCamera(res->cameras[i].pos(),
                                 res->cameras[i].camQuat());
    // res->cameras[i]->SetupFrustum();
  }
}

void initCameras(DD_Resources* res) {
  for (size_t i = 0; i < res->tex_counter; i++) {
    if (!res->textures[i].loaded()) {
      res->textures[i].Generate(res->textures[i].path.c_str());
    }
  }
}

void initShaders(DD_Resources* res) {
  for (size_t i = 0; i < res->shader_counter; i++) {
    DD_Shader* shader = ResSpace::findDD_Shader(res, (unsigned)i);
    shader->init();

    // vertex
    if (shader->m_vs.find(".vert") != std::string::npos) {
      shader->CreateVertexShader(shader->m_vs.c_str());
    }
    // geometry
    if (shader->m_gs.find(".geom") != std::string::npos) {
      shader->CreateGeomShader(shader->m_gs.c_str());
    }
    // fragment
    if (shader->m_fs.find(".frag") != std::string::npos) {
      shader->CreateFragShader(shader->m_fs.c_str());
    }
    // compute
    if (shader->m_cs.find(".comp") != std::string::npos) {
      shader->CreateComputeShader(shader->m_cs.c_str());
    }
  }
}

// Only set parent to DD_Agent objects
bool setAgentParent(const DD_Resources* res, DD_Agent* agent, const char* pID) {
  const size_t numAgents = res->m_num_agents;
  size_t index = 0;
  bool searching = true;
  std::string _ID = pID;
  while (searching && index < numAgents) {
    std::string name = res->agents[index]->m_ID;
    if (name == _ID) {
      searching = false;
    }
    index += 1;
  }
  if (searching) {
    agent->unParent();  // if name not found, remove parent
    return false;
  }
  agent->SetParentIndex((int)index - 1);  // name found, set index
  return true;
}

// Only set parent to DD_Agent objects
bool setCamParent(const DD_Resources* res, DD_Camera* cam, const char* pID) {
  const size_t numAgents = res->m_num_agents;
  size_t index = 0;
  bool searching = true;
  std::string _ID = pID;
  while (searching && index < numAgents) {
    std::string name = res->agents[index]->m_ID;
    if (name == _ID) {
      searching = false;
    }
    index += 1;
  }
  if (searching) {
    cam->unParent();  // if name not found, remove parent
    return false;
  }
  // cam->SetParent(pID);
  cam->SetParentIndex((int)index - 1);  // name found, set index
  return true;
}

// Only set parent to DD_Agent objects
bool setLightParent(const DD_Resources* res, DD_Light* light, const char* pID) {
  const size_t numAgents = res->m_num_agents;
  size_t index = 0;
  bool searching = true;
  std::string _ID = pID;
  while (searching && index < numAgents) {
    std::string name = res->agents[index]->m_ID;
    if (name == _ID) {
      searching = false;
    }
    index += 1;
  }
  if (searching) {
    light->unParent();  // if name not found, remove parent
    return false;
  }
  light->SetParentIndex((int)index - 1);  // name found, set index
  return true;
}

bool GetActiveCamera(const DD_Resources* res, DD_Camera*& cam) {
  for (size_t i = 0; i < res->cam_counter; i++) {
    if (res->cameras[i].active) {
      cam = &res->cameras[i];
      return true;
    }
  }
  return false;
}

// helper function to find Model or Skinned Model
DD_Model* checkModelExists(DD_Resources* res, const char* m_id) {
  DD_Model* mdl = nullptr;
  mdl = ResSpace::findDD_Model(res, m_id);
  if (!mdl) {
    mdl = ResSpace::findDD_ModelSK(res, m_id);
  }
  return mdl;
}

// add child agent to resource bin
DD_Agent* AddAgent(DD_Resources* res, DD_Agent* agent) {
  res->agents[res->m_num_agents] = agent;
  res->m_num_agents += 1;
  return res->agents[res->m_num_agents - 1];
}

/// \brief Add animation clip to model
/// \param res Bin containing assete
/// \param model_id Model name
/// \param clip_id Animation clip name
/// \param reference_id New DD_AnimState name
bool addAnimationToModel(DD_Resources* res, const char* model_id,
                         const char* clip_id, const char* reference_id) {
  // get model
  DD_ModelSK* m_sk = findDD_ModelSK(res, model_id);
  if (!m_sk) {
    DD_Terminal::f_post("[error] addAnimationToModel::<%s> model not found",
                        model_id);
    return false;
  }
  // get animation
  DD_AnimClip* clip = findDD_AnimClip(res, clip_id);
  if (!clip) {
    DD_Terminal::f_post("[error] addAnimationToModel::<%s> clip not found",
                        clip_id);
    return false;
  }

  // check if name already exists
  bool name_exists = false;
  for (unsigned i = 0; i < m_sk->m_animStates.size() && !name_exists; i++) {
    if (m_sk->m_animStates[i].m_ID.compare(reference_id) == 0) {
      name_exists = true;
    }
  }
  if (name_exists) {
    return false;
  }

  // add new state
  dd_array<DD_AnimState> temp(m_sk->m_animStates.size() + 1);
  unsigned new_idx = (unsigned)temp.size() - 1;
  if (new_idx > 0) {
    temp = m_sk->m_animStates;
  }  // store old anim states

  temp[new_idx].m_ID.set(reference_id);
  temp[new_idx].clip_id.set(clip_id);
  temp[new_idx].weight = 1.f;
  m_sk->m_animStates = std::move(temp);

  return true;
}

void loadAgentsToMemory(DD_Resources* res) {
  DD_Agent* agent;
  for (size_t i = 0; i < res->m_num_agents; i++) {
    // register handlers
    agent = res->agents[i];
    size_t range = agent->num_handlers;
    for (size_t j = 0; j < range; j++) {
      if (agent->tickets[j] == "post") {
        //res->queue->RegisterPoster(agent->handlers[j]);
      } else {
        //res->queue->RegisterHandler(agent->handlers[j],
                                    //agent->tickets[j].c_str());
      }
    }
    // register models
    for (size_t j = 0; j < agent->mesh_buffer.size(); j++) {
      std::string n = agent->mesh_buffer[j].model;
      DD_Model* model = checkModelExists(res, n.c_str());
      if (!model) {
        agent->mesh_buffer[j].model = "noModel";
        model = ResSpace::findDD_Model(res, "noModel");
      }
      // set bounding box
      agent->BBox = ModelSpace::CalculateBBox(*model);
    }
  }
}

bool loadAgentToGPU(DD_Resources* res, const size_t index) {
  if (index >= res->m_num_agents) {
    return false;
  }
  const bool hasModel =
      res->agents[index]->flag_model || res->agents[index]->flag_modelsk;
  if (!hasModel) {
    return true;
  }

  for (size_t i = 0; i < res->agents[index]->mesh_buffer.size(); i++) {
    std::string n = res->agents[index]->mesh_buffer[i].model;
    DD_Model* model = checkModelExists(res, n.c_str());
    if (!model) {
      continue;
    }
    // Load meshes unto GPU
    for (size_t j = 0; j < model->meshes.size(); j++) {
      if (!model->m_loaded_to_GPU) {
        size_t inst_size = res->agents[index]->inst_m4x4.size();
        size_t inst_c_size = res->agents[index]->inst_colors.size();

        if (res->agents[index]->flag_modelsk) {
          ModelSKSpace::OpenGLBindMesh(
              (unsigned)j, *(static_cast<DD_ModelSK*>(model)),
              (unsigned)inst_size, (unsigned)inst_c_size);
        } else {
          ModelSpace::OpenGLBindMesh((unsigned)j, *model, (unsigned)inst_size,
                                     (unsigned)inst_c_size);
        }
        // free space on system memory
        model->meshes[j].data.resize(1);
      }
      size_t mat_index =
          (j >= model->materials.size()) ? 0 : model->materials[j];
      DD_Material* mat = ResSpace::findDD_Material(res, (unsigned)mat_index);

      mat->OpenGLBindMaterial();

      if (!res->agents[index]->mat_buffer.isValid()) {
        res->agents[index]->mat_buffer.resize(model->materials.size());
        res->agents[index]->mat_buffer = model->materials;
      }
    }
    model->m_loaded_to_GPU = true;
  }
  return true;
}

void loadAgentToGPU(DD_Resources* res, const char* agentID) {
  // Load mesh unto GPU
  for (size_t i = 0; i < res->m_num_agents; i++) {
    DD_Agent* ag = res->agents[i];
    if (ag->m_ID == agentID) {
      for (size_t j = 0; j < ag->mesh_buffer.size(); j++) {
        std::string n = ag->mesh_buffer[j].model;
        DD_Model* model = checkModelExists(res, n.c_str());
        for (size_t k = 0; k < model->meshes.size(); k++) {
          if (!model->m_loaded_to_GPU) {
            size_t inst_size = ag->inst_m4x4.size();
            size_t inst_c_size = ag->inst_colors.size();

            if (ag->flag_modelsk) {
              ModelSKSpace::OpenGLBindMesh(
                  (unsigned)k, *(static_cast<DD_ModelSK*>(model)),
                  (unsigned)inst_size, (unsigned)inst_c_size);
            } else {
              ModelSpace::OpenGLBindMesh((unsigned)k, *model,
                                         (unsigned)inst_size,
                                         (unsigned)inst_c_size);
            }
            // free space on system memory
            model->meshes[k].data.resize(1);
          }
          size_t mat_index =
              (j >= model->materials.size()) ? 0 : model->materials[j];
          DD_Material* mat =
              ResSpace::findDD_Material(res, (unsigned)mat_index);

          mat->OpenGLBindMaterial();

          if (!ag->mat_buffer.isValid()) {
            ag->mat_buffer.resize(model->materials.size());
            ag->mat_buffer = model->materials;
          }
        }
        model->m_loaded_to_GPU = true;
      }
    }
  }
}

void loadAgentToGPU_M(DD_Resources* res, const char* agentID) {
  // Load mesh to GPU but keep models in memory
  DD_Agent* agent = ResSpace::findDD_Agent(res, agentID);

  if (!agent) {
    return;
  }
  if (!agent->flag_model || !agent->flag_modelsk) {
    return;
  }

  for (size_t i = 0; i < agent->mesh_buffer.size(); i++) {
    std::string n = agent->mesh_buffer[i].model;
    DD_Model* model = checkModelExists(res, n.c_str());
    if (!model) {
      continue;
    }
    // Load meshes unto GPU
    for (size_t j = 0; j < model->meshes.size(); j++) {
      if (!model->m_loaded_to_GPU) {
        size_t inst_size = agent->inst_m4x4.size();
        size_t inst_c_size = agent->inst_colors.size();

        if (agent->flag_modelsk) {
          ModelSKSpace::OpenGLBindMesh(
              (unsigned)j, *(static_cast<DD_ModelSK*>(model)),
              (unsigned)inst_size, (unsigned)inst_c_size);
        } else {
          ModelSpace::OpenGLBindMesh((unsigned)j, *model, (unsigned)inst_size,
                                     (unsigned)inst_c_size);
        }

        // free space on system memory
        // model->meshes[j].data.resize(1);		// MAGIC!!!
      }
      size_t mat_index =
          (j >= model->materials.size()) ? 0 : model->materials[j];
      DD_Material* mat = ResSpace::findDD_Material(res, (unsigned)mat_index);

      mat->OpenGLBindMaterial();

      if (!agent->mat_buffer.isValid()) {
        agent->mat_buffer.resize(model->materials.size());
        agent->mat_buffer = model->materials;
      }
    }
    agent->BBox = ModelSpace::CalculateBBox(*model);
    model->m_loaded_to_GPU = true;
  }
}

/// \brief Load an agent to memory (register events handler, load GL buffers)
void loadAgent_ID(DD_Resources* res, const char* agentID, bool mem_flag) {
  DD_Agent* agent = nullptr;
  agent = findDD_Agent(res, agentID);
  if (agent) {
    // register handlers and replaces missing models w/ stand-in
    size_t range = agent->num_handlers;
    for (size_t i = 0; i < range; i++) {
      if (agent->tickets[i] == "post") {
        //res->queue->RegisterPoster(agent->handlers[i], agentID);
      } else {
        //res->queue->RegisterHandler(agent->handlers[i],
                                    //agent->tickets[i].c_str(), agentID);
      }
    }
    // register models
    for (size_t i = 0; i < agent->mesh_buffer.size(); i++) {
      std::string n = agent->mesh_buffer[i].model;
      DD_Model* model = checkModelExists(res, n.c_str());
      if (!model) {
        agent->mesh_buffer[i].model = "noModel";
        model = ResSpace::findDD_Model(res, "noModel");
      }
      // set bounding box
      agent->BBox = ModelSpace::CalculateBBox(*model);

      // bind mesh and material buffers
      for (size_t j = 0; j < model->meshes.size(); j++) {
        if (!model->m_loaded_to_GPU) {
          size_t inst_size = agent->inst_m4x4.size();
          size_t inst_c_size = agent->inst_colors.size();

          if (agent->flag_modelsk) {  // bind skinned mesh
            ModelSKSpace::OpenGLBindMesh(
                (unsigned)j, *(static_cast<DD_ModelSK*>(model)),
                (unsigned)inst_size, (unsigned)inst_c_size);
          } else {
            ModelSpace::OpenGLBindMesh(  // bind normal mesh
                (unsigned)j, *model, (unsigned)inst_size,
                (unsigned)inst_c_size);
          }
          // free space on system memory
          if (!mem_flag) {
            model->meshes[j].data.resize(1);
          }
        }
        // find and set material
        size_t mat_index =
            (j >= model->materials.size()) ? 0 : model->materials[j];
        DD_Material* mat = ResSpace::findDD_Material(res, (unsigned)mat_index);

        mat->OpenGLBindMaterial();

        if (!agent->mat_buffer.isValid()) {
          agent->mat_buffer.resize(model->materials.size());
          agent->mat_buffer = model->materials;
        }
      }
      model->m_loaded_to_GPU = true;
    }
  }
}

void updateSceneGraph(DD_Resources* res, const float dt) {
  const size_t camsInScene = res->cam_counter;
  const size_t lghtInScene = res->light_counter;
  const size_t objsInScene = res->m_num_agents;

  // scene graph on cameras
  for (size_t i = 0; i < camsInScene; i++) {
    DD_Camera* cam = &res->cameras[i];
    if (cam->isChild()) {
      // check if parent index is valid
      if (cam->parentIndex() == -1) {
        ResSpace::setCamParent(res, cam, cam->parentID().c_str());
        // setCamParent will remove child attachment if parent not found
      } else {
        // valid parent index, set transform
        cam->parent_transform = UpdateInstance(cam->parentIndex(), res, dt);
        // update camera transforms
        glm::quat camRot = glm::quat_cast(cam->parent_transform);
        glm::vec4 camPos = cam->parent_transform[3];
        cam->updateCamera(camPos, camRot);
      }
    } else {
      cam->parent_transform = glm::mat4();  // use identity if no parent
    }
  }
  // scene graph on lights
  for (size_t i = 0; i < lghtInScene; i++) {
    DD_Light* lght = &res->lights[i];
    if (lght->isChild()) {
      // check if parent index is valid
      if (lght->parentIndex() == -1) {
        ResSpace::setLightParent(res, lght, lght->parentID().c_str());
        // setLightParent will remove child attachment if parent not found
      } else {
        // valid parent index, set transform
        lght->parent_transform = UpdateInstance(lght->parentIndex(), res, dt);
        // update lght transforms
        glm::vec3 lghtRot =
            glm::eulerAngles(glm::quat_cast(lght->parent_transform));
        glm::vec4 lghtPos = lght->parent_transform[3];
        lght->_position = glm::vec3(lghtPos);
        lght->m_direction = lghtRot;
      }
    } else {
      lght->parent_transform = glm::mat4();  // use identity if no parent
    }
  }
  // scene graph on objects
  for (size_t i = 0; i < objsInScene; i++) {
    DD_Agent* agent = res->agents[i];
    if (agent->isChild()) {
      // check if parent index is valid
      if (agent->parentIndex() == -1) {
        ResSpace::setAgentParent(res, agent, agent->parentID().c_str());
        // setAgentParent will remove child attachment if parent not found
      } else {
        // valid parent index, set transform
        agent->parent_transform = UpdateInstance(agent->parentIndex(), res, dt);
      }
    } else {
      agent->parent_transform = glm::mat4();  // use identity if no parent
    }
  }
}

/// \brief Grab the address of a function pointer
template <typename T, typename... U>
size_t getAddress(std::function<T(U...)> f) {
  typedef T(fnType)(U...);
  fnType** fnPointer = f.template target<fnType*>();
  return (size_t)*fnPointer;
}

/*
void deleteAgent(DD_Resources* res, const char* agent_id,
                 bool free_gpu_memory) {
  DD_Agent* agent = ResSpace::findDD_Agent(res, agent_id);
  if (agent) {
    // remove event handlers from queue
    for (unsigned i = 0; i < agent->num_handlers; i++) {
      // remove from normal handlers
      bool found = false;
      callbackContainer* cc = &res->queue->handlers[agent->tickets[i]];
      unsigned count = (unsigned)res->queue->m_counter[agent->tickets[i]];
      for (unsigned j = 0; j < count && !found; j++) {
        // grab the signature of the function pointer and compare them
        if ((*cc)[j].sig.compare(agent_id) == 0) {
          found = true;
          // switch with handler at end and decrement number of tickets
          (*cc)[j] = (*cc)[count - 1];
          res->queue->m_counter[agent->tickets[i]] -= 1;
        }
      }
      // remove from post handler
      found = false;
      cc = &res->queue->m_postHandlers;
      count = (unsigned)res->queue->m_postLimit;
      for (unsigned j = 0; j < count && !found; j++) {
        if ((*cc)[j].sig.compare(agent_id) == 0) {
          found = true;
          // switch with handler at end and decrement number of tickets
          (*cc)[j] = (*cc)[count - 1];
          res->queue->m_postLimit -= 1;
        }
      }
    }
    if (free_gpu_memory) {
      // unbind meshes if necessary
      for (size_t i = 0; i < agent->mesh_buffer.size(); i++) {
        std::string n = agent->mesh_buffer[i].model;
        DD_Model* model = checkModelExists(res, n.c_str());

        if (model && agent->flag_model) {
          ModelSpace::OpenGLUnBindMesh((int)i, *model);
        } else if (model && agent->flag_modelsk) {
          ModelSKSpace::OpenGLUnBindMesh((int)i,
                                         *(static_cast<DD_ModelSK*>(model)));
        }
        model->m_loaded_to_GPU = false;
      }
    }
    // delete agent
    removeDD_Agent(res, agent_id);
  }
}
//*/

void deleteEmitter(DD_Resources* res, const char* emitterID) {
  int index = -1;
  for (size_t i = 0; i < res->emitter_counter; i++) {
    if (res->emitters[i].m_ID.compare(emitterID) == 0) {
      index = (int)i;
    }
  }
  if (index != -1) {
    size_t range = (res->emitter_counter - index) - 1;
    for (size_t i = index; i < (index + range); i++) {
      res->emitters[i] = std::move(res->emitters[i + 1]);
    }
    res->emitter_counter -= 1;
  }
}

void deleteWater(DD_Resources* res, const char* waterID) {
  int index = -1;
  for (size_t i = 0; i < res->water_counter; i++) {
    if (res->water[i].m_ID.compare(waterID) == 0) {
      index = (int)i;
    }
  }
  if (index != -1) {
    size_t range = (res->water_counter - index) - 1;
    for (size_t i = index; i < (index + range); i++) {
      res->water[i] = std::move(res->water[i + 1]);
    }
    res->water_counter -= 1;
  }
}

// clean out line agents bin
void flushLineAgents(DD_Resources* res) {
  size_t check = res->l_agent_counter;
  for (size_t i = 0; i < check; i++) {
    res->lines[i].FlushLines();
  }
  res->l_agent_counter = 0;
}
}

DD_Texture2D* CreateTexture_OBJMAT(DD_Resources* res, std::string dir,
                                   std::string tex_path) {
  for (size_t i = 0; i < res->tex_counter; i++) {
    DD_Texture2D& tex = res->textures[i];
    if (tex.m_ID == tex_path) {
      return &tex;
    }
  }
  DD_Texture2D& tex = res->textures[res->tex_counter];
  res->tex_counter += 1;

  tex.path = dir + tex_path;
  tex.m_ID = tex_path;
  return &tex;
}

/// \brief Translate DDM ebo data into MeshData object
dd_array<MeshData> getMeshData(FbxVertData& vdata,
                               dd_array<FbxEboData>& edata) {
  dd_array<MeshData> mdata;
  mdata.resize(edata.size());
  std::map<unsigned, unsigned> vertbin;

  auto setBBox = [&](const glm::vec3& test, MeshData& mesh) {
    // max
    mesh.bbox_max.x() =
        (mesh.bbox_max.x() > test.x) ? mesh.bbox_max.x() : test.x;
    mesh.bbox_max.y() =
        (mesh.bbox_max.y() > test.y) ? mesh.bbox_max.y() : test.y;
    mesh.bbox_max.z() =
        (mesh.bbox_max.z() > test.z) ? mesh.bbox_max.z() : test.z;
    // min
    mesh.bbox_min.x() =
        (mesh.bbox_min.x() < test.x) ? mesh.bbox_min.x() : test.x;
    mesh.bbox_min.y() =
        (mesh.bbox_min.y() < test.y) ? mesh.bbox_min.y() : test.y;
    mesh.bbox_min.z() =
        (mesh.bbox_min.z() < test.z) ? mesh.bbox_min.z() : test.z;
  };

  for (size_t i = 0; i < edata.size(); i++) {  // granualarity of ebo
    FbxEboData& _e = edata[i];
    MeshData& md = mdata[i];
    md.indices.resize(_e.idxs.size());
    vertbin.clear();
    unsigned vert_idx = 0;
    for (size_t j = 0; j < _e.idxs.size(); j++) {  // granularity of ebo indices
      const unsigned _i = _e.idxs[j];
      if (vertbin.count(_i) == 0) {
        vertbin[_i] = vert_idx;
        vert_idx += 1;
      }
      md.indices[j] = vertbin[_i];
    }
    md.data.resize(vertbin.size());
    for (auto& k_v : vertbin) {  // save vertices
      /*
      DD_Terminal::post(std::to_string(k_v.first) + ":" +
                                        std::to_string(k_v.second));
      //*/
      setBBox(vdata.v[k_v.first], md);
      // position
      md.data[k_v.second].position[0] = vdata.v[k_v.first].x;
      md.data[k_v.second].position[1] = vdata.v[k_v.first].y;
      md.data[k_v.second].position[2] = vdata.v[k_v.first].z;
      /*printf("%u:%u - %.3f %.3f %.3f\n",
                 k_v.first,
                 k_v.second,
                 vdata.v[k_v.first].x,
                 vdata.v[k_v.first].y,
                 vdata.v[k_v.first].z);*/
      // normal
      md.data[k_v.second].normal[0] = vdata.n[k_v.first].x;
      md.data[k_v.second].normal[1] = vdata.n[k_v.first].y;
      md.data[k_v.second].normal[2] = vdata.n[k_v.first].z;
      // tangent
      md.data[k_v.second].tangent[0] = vdata.t[k_v.first].x;
      md.data[k_v.second].tangent[1] = vdata.t[k_v.first].y;
      md.data[k_v.second].tangent[2] = vdata.t[k_v.first].z;
      // uv
      md.data[k_v.second].texCoords[0] = vdata.u[k_v.first].x;
      md.data[k_v.second].texCoords[1] = vdata.u[k_v.first].y;
      /*snprintf(buff, sizeof(buff), "-> %.3f %.3f %.3f",
                       md.data[k_v.second].position[0],
                       md.data[k_v.second].position[1],
                       md.data[k_v.second].position[2]);
      DD_Terminal::post(buff);*/
      // joints
      md.data[k_v.second].joints[0] = vdata.j[k_v.first].x;
      md.data[k_v.second].joints[1] = vdata.j[k_v.first].y;
      md.data[k_v.second].joints[2] = vdata.j[k_v.first].z;
      md.data[k_v.second].joints[3] = vdata.j[k_v.first].w;
      // blend weights
      md.data[k_v.second].blendweight[0] = vdata.b[k_v.first].x;
      md.data[k_v.second].blendweight[1] = vdata.b[k_v.first].y;
      md.data[k_v.second].blendweight[2] = vdata.b[k_v.first].z;
      md.data[k_v.second].blendweight[3] = vdata.b[k_v.first].w;
    }
  }
  /*
  for (size_t i = 0; i < edata.size(); i++) {
          DD_Terminal::post("Ebo #" + std::to_string(i));
          DD_Terminal::post("    fbx verts:        " +
                                            std::to_string(vdata.v.size()));
          DD_Terminal::post("    verts registered: " +
                                            std::to_string(mdata[i].data.size()));
  }
  //*/

  return mdata;
}

/// \brief Read file stream and grab vertex data
void populateFbxVertexData(FbxVertData& vdata, std::ifstream& _stream) {
  char line[256];
  line[0] = '\0';
  glm::vec4 v4;
  glm::uvec4 v4u;
  glm::vec3 v3;
  glm::vec2 v2;
  unsigned idx = 0;

  while (strcmp("</vertex>", line) != 0) {
    _stream.getline(line, sizeof(line));

    switch (*line) {
      case 'v':  // position
        v3 = getVec3f(&line[2]);
        vdata.v[idx] = v3;
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
  }
}

/// \brief Read file stream and grab ebo data
void populateFbxEboData(FbxEboData& edata, std::ifstream& _stream) {
  /// \brief Lambda to get vec3 unsigned from c string
  auto getVec3u = [&](const char* str) {
    glm::uvec3 v3;
    char* nxt;
    v3[0] = strtoul(str, &nxt, 10);
    nxt++;
    v3[1] = strtoul(nxt, &nxt, 10);
    nxt++;
    v3[2] = strtoul(nxt, nullptr, 10);
    return v3;
  };
  char line[256];
  line[0] = '\0';
  glm::uvec3 v3u;
  unsigned idx = 0;

  while (strcmp("</ebo>", line) != 0) {
    _stream.getline(line, sizeof(line));
    switch (*line) {
      case 's':  // number of triangles
      {
        const char* s = &line[2];
        edata.idxs.resize(strtoul(s, nullptr, 10));
      } break;
      case 'm':  // material index
      {
        const char* s = &line[2];
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
  }
}

/// \brief Read file stream and export material data
obj_mat getMatData(std::ifstream& _stream) {
  obj_mat _data;
  char line[256];
  line[0] = '\0';
  glm::vec3 v3;

  while (strcmp("</material>", line) != 0) {
    _stream.getline(line, sizeof(line));
    switch (*line) {
      case 'n':
        _data.ID = &line[2];
        break;
      case 'D':
        _data.albedo_tex = &line[2];
        _data.albedo_flag = true;
        break;
      case 'N':
        _data.normal_tex = &line[2];
        _data.norm_flag = true;
        break;
      case 'S':
        _data.specular_tex = &line[2];
        _data.spec_flag = true;
        break;
      case 'R':
        _data.roughness_tex = &line[2];
        _data.rough_flag = true;
        break;
      case 'M':
        _data.metalness_tex = &line[2];
        _data.metal_flag = true;
        break;
      case 'E':
        _data.emissive_tex = &line[2];
        _data.emit_flag = true;
        break;
      case 'A':
        _data.ao_tex = &line[2];
        _data.ao_flag = true;
        break;
      default:
        break;
    }
  }
  return _data;
}

/// \brief Recursively set instance variables for scene graph
// posibble flaw: Parents must be processed before children
glm::mat4 UpdateInstance(const int index, DD_Resources* rsrc, float thisFrame) {
  DD_Agent* agent = rsrc->agents[index];
  agent->cleanInst();
  glm::mat4 myMat = agent->inst_m4x4[0];

  if (agent->checkFrame() < thisFrame) {
    agent->frameFlag(thisFrame);  // set flag to avoid recheck

    if (agent->isChild()) {
      // check if parent index is valid (and set if not previously set)
      if (agent->parentIndex() == -1) {
        bool parentFound =
            ResSpace::setAgentParent(rsrc, agent, agent->parentID().c_str());

        if (parentFound) {
          // recursive lookup
          return myMat * UpdateInstance(agent->parentIndex(), rsrc, thisFrame);
        }
        // parentID is not valid (return my instance matrix)
        return myMat;
      }
      // parentID is valid and already set
      return myMat * UpdateInstance(agent->parentIndex(), rsrc, thisFrame);
    }
    // highest node is this one/ no higher parent
    return myMat;
  }
  // node tree already traversed
  return myMat;
}

/// \brief Recursively calculate delta transform between bone joints
void calcDeltaBoneTransform(DD_Skeleton* sk, bool* visited,
                            const unsigned idx) {
  if (!visited[idx]) {
    visited[idx] = true;

    DD_Joint& jnt = sk->m_bones[idx];
    const unsigned p_idx = jnt.m_parent;
    // check parent bone
    if (!visited[p_idx]) {
      calcDeltaBoneTransform(sk, visited, p_idx);
    }
    if (idx == 0) {
      jnt.m_pDelta = glm::mat4();
    } else {
      jnt.m_pDelta = sk->m_bones[p_idx].m_invBP * glm::inverse(jnt.m_invBP);
    }
  }
}
