#pragma once

/*
* Copyright (c) 2017, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	DD_Types:
*		- Typesdefs and enums for engine use
*	DD_Event:
*		- Defines the message objects
*
-----------------------------------------------------------------------------*/

#include "DD_Types.h"

// Particles
enum EmitterType { HALO, FIRE, WATER, MAGIC, NUM_TYPES };

enum RenderTextureSets {
  NA,
  STAR,
  FIRE01,
  B_BALL,
  SMOG,
  SKITTLES,
  FABRIC_1,
  NUM_TEXSETS,
};

struct emitterInit : public BaseData {
  std::string ID, parentID;
  RenderTextureSets textureSet;
  glm::mat4 model, direction;
  float radius, lifetime, size, emitPerSec, deathRate, rotPerSec, velUp;
  EmitterType type;
};

struct clothInit : public BaseData {
  std::string ID;
  glm::vec3 firstPoint;
  float rowSize, colSize, pointDist;
  dd_array<size_t> pinnedPoints;
};

struct clothUpdate : public BaseData {
  std::string ID;
  glm::vec3 windSpeed, ball_pos, ball_velocity;
  float ball_radius, ball_mass;
};

class DD_Emitter {
 public:
  DD_Emitter()
      : m_ID(""),
        m_parentMat(glm::mat4()),
        _parent_id(""),
        m_textureSet(RenderTextureSets::NUM_TEXSETS),
        m_modelMat(glm::mat4()),
        m_radius(0.0f),
        m_lifetime(0.0f),
        m_livedTime(0.0f),
        _parent_idx(-1),
        _flag_parent(false),
        m_flagTextureSet(false) {}

  ~DD_Emitter() {}

  void Free();
  inline void Clear() {
    m_ID = "";
    m_parentMat = m_modelMat = glm::mat4();
    _parent_id = "";
    m_textureSet = RenderTextureSets::NA;
    m_radius = m_lifetime = m_livedTime = 0.0f;
    _parent_idx = -1;
    _flag_parent = m_flagTextureSet = false;
  }
  void Initialize(const char* ID, const char* parentID, const float radius,
                  const float lifetime, const float size, const int _emitPerSec,
                  const EmitterType type, glm::mat4& placement,
                  const float deathR, const float rotationPS, glm::mat4& _direc,
                  const RenderTextureSets ts, const float streamF);
  bool isDead();
  inline glm::mat4 modelMatrix() const { return m_modelMat; }
  inline float size() const { return _size; }
  inline EmitterType type() const { return m_type; }
  inline void SetParent(const char* parentID) {
    _parent_id = parentID;
    _parent_idx = -1;
    _flag_parent = true;
  }
  inline void unParent() {
    _parent_id = "";
    _parent_idx = -1;
    _flag_parent = false;
  }
  inline void SetParentIndex(const int index) { _parent_idx = index; }
  inline int parentIndex() const { return _parent_idx; }
  inline std::string parentID() const { return _parent_id; }
  inline bool isChild() const { return _flag_parent; }
  inline float updateLivedTime(float dt) { return {m_livedTime += dt}; }
  inline int numParticles() const { return m_numPtcl; }
  inline float emitterLife() const { return m_lifetime; }
  inline int emitPerSec() const { return m_emitPerSec; }
  inline float radius() const { return m_radius; }
  inline void registerTexSet(RenderTextureSets ts) {
    m_textureSet = ts;
    m_flagTextureSet = true;
  }
  inline RenderTextureSets tex() const { return m_textureSet; }
  inline bool texSetExists() const { return m_flagTextureSet; }
  inline glm::vec3 pos() const {
    return glm::vec3((m_parentMat * m_modelMat[3]));
  }
  inline float deathRate() const { return m_deathRate; }
  inline glm::mat4 updateRotMat() {
    m_rotMat = glm::rotate(m_rotMat, glm::radians(m_rotPerSec),
                           glm::vec3(0.0, 1.0, 0.0));
    return m_rotMat;
  }
  inline glm::mat4 getRotMat() const { return m_rotMat; }

  std::string m_ID;
  glm::mat4 m_parentMat, m_streamDirec;
  GLuint m_VAO, m_posBuf, m_velBuf, m_colBuf, m_lifeBuf, m_initBuf,
      scratch_numCreated;
  float m_forceUp;

  // void Load(const int startIndex, const int stopIndex, const float dt);
 private:
  std::string _parent_id;
  RenderTextureSets m_textureSet;
  EmitterType m_type;
  glm::mat4 m_modelMat, m_rotMat;
  float m_radius, m_lifetime, m_livedTime, _size, m_deathRate, m_rotPerSec;
  int _parent_idx, m_numPtcl, m_emitPerSec;
  bool _flag_parent, m_flagTextureSet;
};

struct DD_Cloth {
  DD_Cloth() : m_windSpeed(glm::vec3()), m_lifetime(0) {}
  ~DD_Cloth() {}

  void Initialize(const char* ID, const size_t rowSize, const size_t colSize,
                  const float pointDist, const glm::vec3 firstPos,
                  dd_array<size_t> pinned);
  inline size_t numPoints() const { return m_points.size(); }

  void Free();
  std::string m_ID;
  dd_array<glm::vec4> m_points, m_velocity, m_oldPos, m_normal;
  dd_array<glm::vec2> m_uvs;
  dd_array<GLuint> m_indices;
  GLuint m_VAO, m_EBO, m_posBuf[2], m_velBuf[2], m_uvBuf, m_oldBuf, m_normBuf,
      m_rowS, m_colS;
  glm::vec3 m_windSpeed;
  float m_horizDist, m_vertDist, m_diagDist, m_lifetime;
};

struct DD_Water {
  DD_Water() : active(false), pause(false) {}
  ~DD_Water();

  void initialize(const size_t rowSize, const size_t colSize,
                  const float x_pointDist, const float y_pointDist,
                  const glm::vec3 firstPos, const size_t tex_repeat = 1);
  void update(const float dt);
  void addDrop(const glm::vec2 pos_xz, const float radius, const float height);
  inline size_t numPoints() const { return m_pos.size(); }

  std::string m_ID, m_skyb01, m_normTex;
  dd_2Darray<glm::vec4> m_pos, m_normal;
  dd_2Darray<glm::vec3> m_xhalf, m_zhalf;
  dd_2Darray<glm::vec2> m_vel;
  dd_array<glm::vec2> m_uvs;
  dd_array<GLuint> m_indices;
  glm::mat4 m_transpose_mat;
  GLuint m_VAO, m_EBO, m_posBuf[3], m_velBuf[3], m_uvBuf, m_normBuf, m_rowS,
      m_colS;
  float m_xDist, m_yDist, m_waterHeight;
  bool active, pause;
};
