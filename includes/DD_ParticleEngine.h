#pragma once

/*
* Copyright (c) 2017, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	DD_ParticleEngine:
*		-	Particle system
*
*	TODO:	==
*			==
-----------------------------------------------------------------------------*/

#include <random>
#include "DD_Agent.h"
#include "DD_Camera.h"
#include "DD_ParticleTypes.h"
#include "DD_ResourceLoader.h"
#include "DD_Shader.h"
#include "DD_Timer.h"
#include "DD_Types.h"

struct BallCollider {
  glm::vec3 vel, pos;
  float mass, radius;
  bool active;
};

class DD_ParticleSys {
 public:
  DD_ParticleSys()
      : pause(false),
        m_texSetIndex(RenderTextureSets::NUM_TEXSETS),
        m_activeClothes(0),
        m_numJobsA(0) {
    for (int i = 0; i < RenderTextureSets::NUM_TEXSETS; i++) {
      m_texSetIndex[i] = -1;
    }
  }
  ~DD_ParticleSys() {
    glDeleteBuffers(1, &m_jobs_VBO);
    glDeleteVertexArrays(1, &m_jobs_VAO);
  }

  DD_Resources* m_resBin;
  PushFunc push;

  void Load(const float Width, const float Height);
  bool Init();
  DD_Event Create(DD_Event& event);
  void create(DD_LEvent& _event);
  bool Draw(const float dt, glm::mat4 view, glm::mat4 proj,
            const glm::vec3 camP, const GLuint particleFBO,
            const GLuint gbufferFBO);
  DD_Event AddJobToQueue(DD_Event& event);
  void add_job(DD_LEvent& _event);
  bool pause;

 private:
  void CleanEmitterBin();
  int Generate(DD_Emitter* em, const float dt);
  void LoadToGPU(DD_Emitter* em);
  void LoadToGPU(DD_Cloth* cloth);
  bool SetEmitterParent(DD_Emitter* em, const char* pID);
  DD_Cloth m_clothes[20];
  dd_array<int> m_texSetIndex;
  DD_Shader m_prend_sh, m_pinitcp_sh, m_psimcp_sh, m_crend_sh, m_csimcp_sh,
      m_normcp_sh, m_wcomp_sh, m_wrend_sh, m_wsetzcp_sh, m_wsetxcp_sh,
      m_wmidcp_sh;
  size_t m_activeClothes, m_numJobsA;
  GLfloat m_Width, m_Height;
  shaderDataA m_jobsA[5];
  BallCollider ballColl;
  GLuint m_jobs_VAO, m_jobs_VBO;
};
