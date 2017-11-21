#pragma once

/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	DD_Renderer:
*		- manages deferred renderer and renders GameObjects, Lights, etc...
*			- Sends OpenGL calls to GPU
*			- Contains and manages shaders
*			- Sorts and culls gameobjects
*
*	TODO:	== Add shadows
*			== Add spot light
*			==
-----------------------------------------------------------------------------*/

#include "DD_Types.h"
#include "DD_ResourceLoader.h"
#include "DD_Shader.h"
#include "DD_EventQueue.h"
#include "DD_Timer.h"
#include "DD_LoadScreen.h"
#include "DD_MeshTypes.h"
#include "DD_ParticleEngine.h"

#include <time.h>
#include <omp.h>

struct GBuffer
{
	GLuint deferredFBO, depthBuf, posTex, normTex, colorTex;
};

struct ShadowBuffer
{
	GLuint depthMapFBO, depthBuf, shadowTex, width, height;
};

struct LightBuffer
{
	GLuint lightFBO, depthBuf, colorTex, colorTex2;
};

struct ParticleBuffer
{
	GLuint particleFBO, depthBuf, colorTex;
};

struct CubeMapBuffer
{
	GLuint cubeFBO, depthBuf;
};

struct FilterBuffer
{
	GLuint filterFBO, filterDepthFBO, colorTex, depthTex;
};

enum VR_Eye
{
	LEFT,
	RIGHT,
	DEFAULT
};

enum Shaders
{
	GBUFFER = 0,
	LIGHT,
	LIGHT_STENCIL,
	LINES_3D,
	POST_PROCESS,
	LUMINANCE,
	TEXT,
	PRIMITIVE,
	DEPTH,
	DEPTH_SKINNED,
	TEX_SAMPLER,
	SKINNED,
	BLUR,
	NUM_SHADERS
};

struct DD_Renderer
{
	DD_Renderer();

	~DD_Renderer()
	{
		glDeleteFramebuffers(1, &(m_gbuffer.deferredFBO));
		glDeleteFramebuffers(1, &(m_lbuffer.lightFBO));
		//if (currentFont) {
			//delete currentFont;
		//}
	}

	// Main methods
	DD_Event RenderHandler(DD_Event& event);
	void LoadRendererEngine(const GLfloat _Width, const GLfloat _Height);
	void LoadEngineAssets();
	void QueryShaders();
	void Draw(float dt);
	void DrawDynamicCube(float dt);
	void LinePass(GLfloat dt, glm::mat4& view, glm::mat4& proj);
	void DrawLoadScreen(float timeElasped);
	void DrawTextToScreen(std::string text,
						  GLfloat x,
						  GLfloat y,
						  GLfloat scale,
						  glm::vec3 color);
	void CullObjects(FrustumBox& frustum, glm::mat4& viewMat);
	void FrustumCull(DD_Agent& obj, FrustumBox & frust, glm::mat4& view);
	void LevelOfDetail(DD_Agent& obj, const int objInstIndex, glm::mat4& viewMat);
	void GBufferPass(GLfloat dt,
					 glm::mat4& view,
					 glm::mat4& proj,
					 const bool clip = false,
					 glm::vec4 c_plane = glm::vec4());
	void ShadowPass(GLfloat dt, VR_Eye eye = VR_Eye::DEFAULT);
	void InstanceSetup(float dt,
					   glm::mat4& view,
					   glm::mat4& proj,
					   DD_Agent* agent,
					   DD_Shader* shader,
					   const bool flagInstance = false,
					   DD_Light* shadow = nullptr);
	void StaticMeshRender(DD_Shader* shader,
						  DD_Agent* agent,
						  const size_t modelIndex,
						  const size_t bufferOffet,
						  glm::mat4& proj,
						  glm::mat4& view,
						  DD_Light* shadow = nullptr);
	void SkinnedMeshRender(DD_Shader* shader,
						   DD_Agent* agent,
						   const size_t modelIndex,
						   const size_t bufferOffet,
						   glm::mat4& proj,
						   glm::mat4& view,
						   DD_Light* shadow = nullptr);
	void LightPass(DD_Shader* shader,
				   DD_Camera* cam,
				   glm::mat4& view,
				   glm::mat4& proj,
				   VR_Eye eye = VR_Eye::DEFAULT);
	void RenderLightSphere(DD_Shader* shader, const glm::mat4 MVP);
	void CreateCubeMap(const char * ID);
	GLfloat	ComputeLuminance();

	GBuffer			m_gbuffer;
	ShadowBuffer	m_sbuffer;
	LightBuffer		m_lbuffer;
	ParticleBuffer	m_pbuffer;
	CubeMapBuffer	m_cbuffer;
	FilterBuffer	m_fbuffer;
	DD_Resources*	m_resourceBin;
	DD_Timer*		m_time;
	DD_Camera		menuCam;
	DD_Model*		m_volumeSphere;
	DD_ParticleSys*	m_particleSys;
	std::string		m_lvl_cubMap;

	glm::mat4		m_currentLSM;
	GLfloat			m_Width;
	GLfloat			m_Height;
	GLfloat			cam_eye_dist;
	GLfloat			m_scrVertDist;
	GLfloat			m_scrHorzDist;
	GLfloat			m_textW;
	GLfloat			m_textH;
	GLfloat			bgcol[4];
	dd_array<int>	m_lumiTexSizes;
	dd_array<int>	m_objSink;
	dd_array<GLfloat> m_luminValues;
	dd_array<GLfloat> m_lowDist;
	dd_array<GLfloat> m_highDist;
	dd_array<GLuint> m_luminOutput;
	dd_array<GLuint> m_renderable_objects;
	size_t			m_LODBufferSize[10];
	GLuint			m_cubeMapHandle;
	GLuint			m_objectsInFrame;
	DD_Camera		*m_active_cam;
	DD_Camera		*m_cube_cam;
	dd_array<DD_Shader> m_shaders;
	GLboolean		m_flagVR;
	GLboolean		m_flagCubeMap;
	GLboolean		m_flagShadow;
	GLboolean		m_flagLineRend;
	GLboolean		m_flagDCM;
private:
	GLuint m_jointVBO = 0;
	bool m_debugOn = false;
	bool screen_shot_on = false;
	u64 last_frame_time = 0;
	size_t framesPerSec = 0;
	size_t drawCallsInFrame = 0;
	size_t trisInFrame = 0;
	size_t linesInFrame = 0;
	float flagSecond = 0.0f;
	float msPerFrameTotal = 0.0f;
	float avgFT = 0.0f;
	float avgFPS = 0.0f;
	float fps60_tick = 0.f;
	cbuff<64> screen_shot_name;
};

namespace RendSpace {
	void RenderQuad(DD_Shader* shader, const glm::mat4 &identityMatrix);
	void RenderVR_Split(DD_Shader* shader, const glm::mat4 &identityMatrix,
						const int eye);
	void RenderCube(DD_Shader* shader, const glm::mat4 &MVP);
	void RenderLine(DD_Shader* shader, const glm::mat4 &MVP, dd_array<glm::vec4>& bin,
					const glm::vec4 color);
	void RenderBBox(DD_Shader* shader, const BoundingBox* bbox, glm::vec3 color);
	void BindPassTexture(DD_Shader* shader, const GBuffer* gBuf);
	void BindPassTexture(DD_Shader* shader, 
						 const LightBuffer* lBuf);
	void BindPassTexture(DD_Shader* shader, const ParticleBuffer* pBuf);
	void BindPassTexture(DD_Shader* shader, const CubeMapBuffer* cBuf,
						 GLenum target, DD_Skybox* sb);
	void BindPassTexture(DD_Shader* shader, const ShadowBuffer* sBuf,
						 const bool lightPass = false);
	void CreateTex(const GLenum texUnit, const GLenum format, GLuint &texID,
				   const int width, const int height);
	GBuffer CreateGBuffer(const int width, const int height);
	LightBuffer CreateLightBuffer(const int width, const int height);
	ShadowBuffer CreateShadowBuffer(const int width, const int height);
	ParticleBuffer CreateParticleBuffer(const int width, const int height);
	CubeMapBuffer CreateCubeMapBuffer(const int width, 
									  const int height, 
									  DD_Skybox* sb);
	FilterBuffer CreateFilterBuffer(const int width, 
									const int height, 
									ShadowBuffer* sbuf);
	void screenShot(const char* sig, 
					const float game_time,
					const unsigned width,
					const unsigned height);
}

namespace SortSpace {
	typedef int(*partition_func)(DD_Agent*, const int, const int);

	inline void SetUp() { std::srand((unsigned)time(NULL)); }
	void QuickSort(DD_Agent* agent, const int p, const int r,
				   partition_func partfunc);
	int PartitionLOD(DD_Agent* agent, const int p, const int r);
}
