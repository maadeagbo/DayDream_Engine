#pragma once

#define RES_DECL_ALL(TYPE)                                      \
  TYPE* getNew##TYPE(DD_Resources* res, const char* id);        \
  int get##TYPE##_idx(const DD_Resources* res, const char* id); \
  bool remove##TYPE(DD_Resources* res, const char* id);         \
  TYPE* find##TYPE(const DD_Resources* res, const char* id);    \
  TYPE* find##TYPE(const DD_Resources* res, const unsigned idx);

#define RES_DECL_ALL_PTR(TYPE)                                  \
  TYPE* getNew##TYPE(DD_Resources* res, const char* id);        \
  int get##TYPE##_idx(const DD_Resources* res, const char* id); \
  bool remove##TYPE(DD_Resources* res, const char* id);         \
  TYPE* find##TYPE(const DD_Resources* res, const char* id);    \
  TYPE* find##TYPE(const DD_Resources* res, const unsigned idx);

#define RES_DEF_ALL(TYPE, BNAME, ITER)                            \
  TYPE* getNew##TYPE(DD_Resources* res, const char* id) {         \
    if (res->ITER >= res->BNAME.size()) {                         \
      return nullptr;                                             \
    }                                                             \
    res->BNAME[res->ITER] = TYPE();                               \
    res->BNAME[res->ITER].m_ID = id;                              \
    res->ITER += 1;                                               \
    return &res->BNAME[res->ITER - 1];                            \
  }                                                               \
  int get##TYPE##_idx(const DD_Resources* res, const char* id) {  \
    for (unsigned i = 0; i < res->ITER; i++) {                    \
      if (res->BNAME[i].m_ID.compare(id) == 0) {                  \
        return (int)i;                                            \
      }                                                           \
    }                                                             \
    return -1;                                                    \
  }                                                               \
  bool remove##TYPE(DD_Resources* res, const char* id) {          \
    int idx = get##TYPE##_idx(res, id);                           \
    if (idx < 0) {                                                \
      return false;                                               \
    } else {                                                      \
      unsigned range = ((unsigned)res->ITER - idx) - 1;           \
      for (unsigned i = idx; i < (idx + range); i++) {            \
        res->BNAME[i] = std::move(res->BNAME[i + 1]);             \
      }                                                           \
      res->ITER -= 1;                                             \
      return true;                                                \
    }                                                             \
  }                                                               \
  TYPE* find##TYPE(const DD_Resources* res, const char* id) {     \
    int idx = get##TYPE##_idx(res, id);                           \
    if (idx >= 0) {                                               \
      return &res->BNAME[idx];                                    \
    }                                                             \
    return nullptr;                                               \
  }                                                               \
  TYPE* find##TYPE(const DD_Resources* res, const unsigned idx) { \
    if (idx < res->ITER) {                                        \
      return &res->BNAME[idx];                                    \
    }                                                             \
    return nullptr;                                               \
  }

#define RES_DEF_ALL_PTR(TYPE, BNAME, ITER)                        \
  TYPE* getNew##TYPE(DD_Resources* res, const char* id) {         \
    if (res->ITER >= res->BNAME.size()) {                         \
      return nullptr;                                             \
    }                                                             \
    res->BNAME[res->ITER] = new TYPE();                           \
    res->BNAME[res->ITER]->m_ID = id;                             \
    res->ITER += 1;                                               \
    return res->BNAME[res->ITER - 1];                             \
  }                                                               \
  int get##TYPE##_idx(const DD_Resources* res, const char* id) {  \
    for (unsigned i = 0; i < (unsigned)res->ITER; i++) {          \
      if (res->BNAME[i]->m_ID.compare(id) == 0) {                 \
        return (int)i;                                            \
      }                                                           \
    }                                                             \
    return -1;                                                    \
  }                                                               \
  bool remove##TYPE(DD_Resources* res, const char* id) {          \
    int idx = get##TYPE##_idx(res, id);                           \
    if (idx < 0) {                                                \
      return false;                                               \
    } else {                                                      \
      delete res->BNAME[idx];                                     \
      unsigned range = ((unsigned)res->ITER - idx) - 1;           \
      for (unsigned i = idx; i < (idx + range); i++) {            \
        res->BNAME[i] = res->BNAME[i + 1];                        \
      }                                                           \
      res->ITER -= 1;                                             \
      return true;                                                \
    }                                                             \
  }                                                               \
  TYPE* find##TYPE(const DD_Resources* res, const char* id) {     \
    int idx = get##TYPE##_idx(res, id);                           \
    if (idx >= 0) {                                               \
      return res->BNAME[idx];                                     \
    }                                                             \
    return nullptr;                                               \
  }                                                               \
  TYPE* find##TYPE(const DD_Resources* res, const unsigned idx) { \
    if ((size_t)idx < res->ITER) {                                \
      return res->BNAME[idx];                                     \
    }                                                             \
    return nullptr;                                               \
  }

#include "DD_AITypes.h"
#include "DD_Agent.h"
#include "DD_Camera.h"
#include "DD_EventQueue.h"
#include "DD_Light.h"
#include "DD_Model.h"
#include "DD_ModelSK.h"
#include "DD_ParticleTypes.h"
#include "DD_Shader.h"
#include "DD_Skeleton.h"
#include "DD_Texture2D.h"
#include "DD_Types.h"

const size_t num_models = 100;

struct DD_Resources {
  dd_array<DD_Agent*> agents;
  dd_array<DD_Model> models;
  dd_array<DD_Material> materials;
  dd_array<DD_Camera> cameras;
  dd_array<DD_Light> lights;
  dd_array<DD_Texture2D> textures;
  dd_array<DD_Skybox> skyboxes;
  dd_array<DD_Emitter> emitters;
  dd_array<DD_Cloth> clothes;
  dd_array<DD_Water> water;
  dd_array<DD_LineAgent> lines;
  dd_array<DD_AIObject> ai_s;
  dd_array<AI_Agent> ai_agents;
  dd_array<DD_Shader> shaders;
  dd_array<DD_Skeleton> skeletons;
  dd_array<DD_ModelSK> sk_models;
  dd_array<DD_AnimClip> clips;
  size_t m_num_agents = 0, mdl_counter = 0, mtl_counter = 0, cam_counter = 0,
         light_counter = 0, tex_counter = 0, skybox_counter = 0,
         emitter_counter = 0, clothes_counter = 0, water_counter = 0,
         l_agent_counter = 0, ai_obj_counter = 0, ai_agent_counter = 0,
         shader_counter = 0, skeleton_counter = 0, sk_mdl_counter = 0,
         clip_counter = 0;
  GLuint G_BUFFER = 0;

  DD_Resources()
      : agents(500),
        models(500),
        materials(500),
        cameras(100),
        lights(100),
        textures(1000),
        skyboxes(100),
        emitters(50),
        clothes(50),
        water(10),
        lines(100),
        ai_s(50),
        ai_agents(100),
        shaders(100),
        skeletons(50),
        sk_models(100),
        clips(100) {}

  void LoadDefaults();

  DD_Queue* queue;
};

namespace ResSpace {
RES_DECL_ALL_PTR(DD_Agent)
RES_DECL_ALL(DD_Model)
RES_DECL_ALL(DD_Material)
RES_DECL_ALL(DD_Camera)
RES_DECL_ALL(DD_Light)
RES_DECL_ALL(DD_Texture2D)
RES_DECL_ALL(DD_Skybox)
RES_DECL_ALL(DD_Emitter)
RES_DECL_ALL(DD_Cloth)
RES_DECL_ALL(DD_Water)
RES_DECL_ALL(DD_LineAgent)
RES_DECL_ALL(DD_AIObject)
RES_DECL_ALL(AI_Agent)
RES_DECL_ALL(DD_Shader)
RES_DECL_ALL(DD_Skeleton)
RES_DECL_ALL(DD_ModelSK)
RES_DECL_ALL(DD_AnimClip)

void Load(DD_Resources* res, const char* fileName);
DD_Model loadModel(DD_Resources* res, const char* obj_path,
                   const char* mtlName = "");
DD_ModelSK* loadSkinnedModel(DD_Resources* res, const char* new_mdl_id,
                             const char* skeleton_id,
                             const dd_array<MeshData>& md);
DD_ModelSK* loadSkinnedModel(DD_Resources* res, const char* new_mdl_id,
                             const char* ddm_path, const char* skeleton_id,
                             const char* ddb_path);
DD_Model* loadModel_MD(DD_Resources* res, const char* id,
                       const dd_array<MeshData>& md, DD_Model* mdl = nullptr);
DD_Model* loadModel_DDM(DD_Resources* res, const char* id,
                        const char* ddm_path);
dd_array<MeshData> loadDDM(const char* filename);
dd_array<MeshData> loadDDG(const char* filename);
DD_Skeleton* loadDDB(DD_Resources* res, const char* path, const char* id);
DD_AnimClip* loadDDA(DD_Resources* res, const char* path, const char* id);

DD_Material createMaterial(DD_Resources* res, obj_mat& matbuff);
void initCameras(DD_Resources* res, const float width, const float height);
void initCameras(DD_Resources* res);
void initShaders(DD_Resources* res);
bool setAgentParent(const DD_Resources* res, DD_Agent* agent, const char* pID);
bool setCamParent(const DD_Resources* res, DD_Camera* cam, const char* pID);
bool setLightParent(const DD_Resources* res, DD_Light* light, const char* pID);

DD_Model* checkModelExists(DD_Resources* res, const char* m_id);
bool GetActiveCamera(const DD_Resources* res, DD_Camera*& cam);

DD_Agent* AddAgent(DD_Resources* res, DD_Agent* agent);
bool addAnimationToModel(DD_Resources* res, const char* model_id,
                         const char* clip_id, const char* reference_id);
void loadAgentsToMemory(DD_Resources* res);
bool loadAgentToGPU(DD_Resources* res, const size_t index);
void loadAgentToGPU(DD_Resources* res, const char* agentID);
void loadAgentToGPU_M(DD_Resources* res, const char* agentID);
void loadAgent_ID(DD_Resources* res, const char* agentID,
                  bool mem_flag = false);

void updateSceneGraph(DD_Resources* res, const float dt);

void deleteAgent(DD_Resources* res, const char* agent_id,
                 bool free_gpu_memory = false);
void deleteEmitter(DD_Resources* res, const char* emitterID);
void deleteWater(DD_Resources* res, const char* waterID);
void flushLineAgents(DD_Resources* res);
}
