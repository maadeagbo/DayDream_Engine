#include "DD_RenderEngine.h"
#include "DD_Terminal.h"
#include <SOIL.h>

#ifdef __linux__
#pragma GCC diagnostic ignored "-Wformat-security"
#endif

namespace {
	DD_Light* GetDirectionalShadowLight(DD_Resources* res)
	{
		size_t index = 0;
		while( index < res->light_counter ) {
			DD_Light* lght = &res->lights[index];
			if( lght->m_flagShadow && lght->m_type == LightType::DIRECTION_L ) {
				return lght;
			}
			index += 1;
		}
		return nullptr;
	}

	GLuint quadVAO = 0;
	GLuint quadVBO;

	bool vrRendered = false;
	GLuint vrVAO[2];
	GLuint vrVBO[2];

	GLuint cubeVAO = 0;
	GLuint cubeVBO;

	GLuint bboxVAO = 0;
	GLuint bboxVBO;
	GLuint bboxEBO;
	GLfloat bboxBin[8 * 3];
	GLuint bboxInd[4 * 3 * 2] = {
		0, 1, 1, 2, 2, 3, 3, 1,
		0, 4, 1, 5, 2, 6, 3, 7,
		4, 5, 5, 6, 6, 7, 7, 4
	};

	GLuint lineVAO = 0, lineVBO;

	unsigned int dynamic_cb_tic = 0;
	float dynamic_cb_interval = 0.f;

	cbuff<64> debug_cbuff;
	dd_array<unsigned char> pixel_write_buffer;
}

// Renders 1X1 quad
void RendSpace::RenderQuad(DD_Shader * shader,
						   const glm::mat4 & identityMatrix)
{
	if( quadVAO == 0 ) {
		GLfloat quadVertices[] = {
			// Positions        // Texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// Setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices,
					 GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
			(GLvoid*)0);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
			(GLvoid*)(3 * sizeof(GLfloat)));
	}
	// vertex uniforms
	shader->setUniform("MVP", identityMatrix);
	//shader->setUniform("QuadRender", GLboolean(true));

	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Renders 2 half quads
void RendSpace::RenderVR_Split(DD_Shader * shader,
							   const glm::mat4 & identityMatrix,
							   const int eye)
{
	if( vrRendered == false ) {
		GLfloat vrLeft[] = {
			// Positions        // Texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			0.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		GLfloat vrRight[] = {
			// Positions        // Texture Coords
			0.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// Setup plane VAO
		glGenVertexArrays(2, vrVAO);
		glGenBuffers(2, vrVBO);

		glBindVertexArray(vrVAO[0]);
		glBindBuffer(GL_ARRAY_BUFFER, vrVBO[0]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vrLeft), &vrLeft, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
			(GLvoid*)0);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
			(GLvoid*)(3 * sizeof(GLfloat)));

		glBindVertexArray(vrVAO[1]);
		glBindBuffer(GL_ARRAY_BUFFER, vrVBO[1]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vrRight), &vrRight, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
			(GLvoid*)0);
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat),
			(GLvoid*)(3 * sizeof(GLfloat)));

		vrRendered = true;
	}
	// vertex uniforms
	shader->setUniform("MVP", identityMatrix);
	shader->setUniform("QuadRender", GLboolean(true));

	if( eye == 0 ) {
		glBindVertexArray(vrVAO[0]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	else {
		glBindVertexArray(vrVAO[1]);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
}

// RenderCube() Renders a 1x1 3D cube in NDC.
void RendSpace::RenderCube(DD_Shader * shader, const glm::mat4 & MVP)
{
	// Initialize (if necessary)
	if( cubeVAO == 0 ) {
		GLfloat skyboxVertices[] = {
			// Positions
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f,  1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			-1.0f,  1.0f, -1.0f,
			1.0f,  1.0f, -1.0f,
			1.0f,  1.0f,  1.0f,
			1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			1.0f, -1.0f, -1.0f,
			1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			1.0f, -1.0f,  1.0f
		};
		glGenVertexArrays(1, &cubeVAO);
		glGenBuffers(1, &cubeVBO);
		glBindVertexArray(cubeVAO);
		glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices,
					 GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat),
			(GLvoid*)0);
	}
	// vertex uniforms
	shader->setUniform("MVP", MVP);

	// Render Cube
	glBindVertexArray(cubeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
}

// Renders 2-point line segments with color
void RendSpace::RenderLine(DD_Shader * shader,
						   const glm::mat4 & MVP,
						   dd_array<glm::vec4>& bin,
						   const glm::vec4 color)
{
	// Initialize
	if( lineVAO == 0 ) {
		// Setup plane VAO
		glGenVertexArrays(1, &lineVAO);
		glGenBuffers(1, &lineVBO);
		glBindVertexArray(lineVAO);
		glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
		//glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * 4, NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 4,
			(GLvoid*)0);
	}

	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// vertex uniforms
	shader->setUniform("MVP", MVP);
	shader->setUniform("color", color);
	glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * bin.size(), &bin[0],
				 GL_DYNAMIC_DRAW);

	glBindVertexArray(lineVAO);
	glDrawArrays(GL_LINES, 0, (GLsizei)bin.size());
	glBindVertexArray(0);

	//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

// Takes in a BoundingBox and renders lines on the screen
void RendSpace::RenderBBox(DD_Shader * shader, const BoundingBox * bbox,
						   glm::vec3 color)
{
	glm::vec3 temp;
	for( size_t i = 0; i < 8; i++ ) {
		if( i == 0 ) {
			temp = bbox->corner1;
		}
		else if( i == 1 ) {
			temp = bbox->corner2;
		}
		else if( i == 2 ) {
			temp = bbox->corner3;
		}
		else if( i == 3 ) {
			temp = bbox->corner4;
		}
		else if( i == 4 ) {
			temp = bbox->corner5;
		}
		else if( i == 5 ) {
			temp = bbox->corner6;
		}
		else if( i == 6 ) {
			temp = bbox->corner7;
		}
		else {
			temp = bbox->corner8;
		}
		// fill bin array
		bboxBin[i * 3] = temp.x;
		bboxBin[i * 3 + 1] = temp.y;
		bboxBin[i * 3 + 2] = temp.z;
	}
	if( bboxVAO == 0 ) {
		glGenVertexArrays(1, &bboxVAO);
		glGenBuffers(1, &bboxVBO);
		glGenBuffers(1, &bboxEBO);

		glBindVertexArray(bboxVAO);
		glBindBuffer(GL_ARRAY_BUFFER, bboxVBO);
		glBufferData(GL_ARRAY_BUFFER,
					 sizeof(GLfloat) * 8 * 3, // number of vertices 4(x, y , z per vert)
					 NULL,
					 GL_DYNAMIC_DRAW
		);
		// position
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GLfloat) * 3,
			(GLvoid*)0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bboxEBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					 sizeof(GLuint) * 24, // number  of lines (2 points per line)
					 bboxInd,
					 GL_STATIC_DRAW
		);
	}
	// glBlend
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);

	// vertex uniforms
	shader->setUniform("color", color);

	// render lines
	glBindVertexArray(bboxVAO);
	glDrawElements(GL_LINE, 12, GL_UNSIGNED_INT, bboxInd);
	glBindVertexArray(0);

	glDisable(GL_BLEND);
}

// Bind textures created in deferred render pass
void RendSpace::BindPassTexture(DD_Shader * shader, const GBuffer * gBuf)
{
	glActiveTexture(GL_TEXTURE0);
	shader->setUniform("PositionTex", 0);
	glBindTexture(GL_TEXTURE_2D, gBuf->posTex);
	glActiveTexture(GL_TEXTURE1);
	shader->setUniform("ColorTex", 1);
	glBindTexture(GL_TEXTURE_2D, gBuf->colorTex);
	glActiveTexture(GL_TEXTURE2);
	shader->setUniform("NormTex", 2);
	glBindTexture(GL_TEXTURE_2D, gBuf->normTex);
}

// Bind textures created in light pass
void RendSpace::BindPassTexture(DD_Shader * shader, const LightBuffer * lBuf)
{
	glActiveTexture(GL_TEXTURE0);
	shader->setUniform("ColorTex", 0);
	glBindTexture(GL_TEXTURE_2D, lBuf->colorTex);
}

// Bind textures created in particle pass
void RendSpace::BindPassTexture(DD_Shader * shader, const ParticleBuffer * pBuf)
{
	glActiveTexture(GL_TEXTURE1);
	shader->setUniform("ParticleTex", 1);
	glBindTexture(GL_TEXTURE_2D, pBuf->colorTex);
}

// Bind textures created in cube map pass
void RendSpace::BindPassTexture(DD_Shader * shader,
								const CubeMapBuffer * cBuf,
								GLenum target, DD_Skybox* sb)
{
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, target,
						   sb->handle, 0);
}

// Bind textures created in shadow pass
void RendSpace::BindPassTexture(DD_Shader * shader,
								const ShadowBuffer * sBuf,
								const bool lightPass)
{
	if( lightPass ) {
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, sBuf->shadowTex);
	}
	else {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, sBuf->shadowTex);
	}
}

// Create empty texture
void RendSpace::CreateTex(const GLenum texUnit,
						  const GLenum format,
						  GLuint & texID,
						  const int width,
						  const int height)
{
	glActiveTexture(texUnit);
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

// Create geometry buffer for deferred shading
GBuffer RendSpace::CreateGBuffer(const int width, const int height)
{
	GBuffer gBuf;
	// create and bind fbo
	glGenFramebuffers(1, &gBuf.deferredFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuf.deferredFBO);

	// depth buffer
	glGenRenderbuffers(1, &gBuf.depthBuf);
	glBindRenderbuffer(GL_RENDERBUFFER, gBuf.depthBuf);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

	// position, normal, color buffers
	CreateTex(GL_TEXTURE0, GL_RGB16F, gBuf.posTex, width, height); // position
	CreateTex(GL_TEXTURE1, GL_RGB16F, gBuf.normTex, width, height); // normal
	CreateTex(GL_TEXTURE2, GL_RGBA8, gBuf.colorTex, width, height); // color

	// attach images to framebuffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
							  GL_RENDERBUFFER, gBuf.depthBuf);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						   gBuf.posTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D,
						   gBuf.normTex, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D,
						   gBuf.colorTex, 0);

	GLenum drawBuffers[] = { GL_NONE,
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	glDrawBuffers(4, drawBuffers);

	if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ) {
		DD_Terminal::post("\nERROR::FRAMEBUFFER:: G Framebuffer is not complete!\n");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return gBuf;
}

LightBuffer RendSpace::CreateLightBuffer(const int width, const int height)
{
	LightBuffer lBuf;

	// create and bind fbo
	glGenFramebuffers(1, &lBuf.lightFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, lBuf.lightFBO);

	// depth buffer
	glGenRenderbuffers(1, &lBuf.depthBuf);
	glBindRenderbuffer(GL_RENDERBUFFER, lBuf.depthBuf);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

	// color buffer
	CreateTex(GL_TEXTURE0, GL_RGBA16F, lBuf.colorTex, width, height);

	// attach images to framebuffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
							  GL_RENDERBUFFER, lBuf.depthBuf);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						   lBuf.colorTex, 0);

	GLenum drawBuffers[] = { GL_NONE, GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(2, drawBuffers);

	if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ) {
		DD_Terminal::post("\nERROR::FRAMEBUFFER:: Light Framebuffer is not complete!\n");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return lBuf;
}

ShadowBuffer RendSpace::CreateShadowBuffer(const int width, const int height)
{
	ShadowBuffer sBuf;
	sBuf.width = width;
	sBuf.height = height;
	GLfloat border[] = { 1.0f, 1.0f, 1.0f, 0.0f };

	// create and bind fbo
	glGenFramebuffers(1, &sBuf.depthMapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, sBuf.depthMapFBO);

	// depth image 1
	glGenTextures(1, &sBuf.shadowTex);
	glBindTexture(GL_TEXTURE_2D, sBuf.shadowTex);
	// texture paramenters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG32F, width, height);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 4);
	glGenerateMipmap(GL_TEXTURE_2D);

	// depth buffer
	glGenRenderbuffers(1, &sBuf.depthBuf);
	glBindRenderbuffer(GL_RENDERBUFFER, sBuf.depthBuf);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT32, width, height);

	// attach images to framebuffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, 
							  GL_DEPTH_ATTACHMENT,
							  GL_RENDERBUFFER, 
							  sBuf.depthBuf);
	glFramebufferTexture2D(GL_FRAMEBUFFER, 
						   GL_COLOR_ATTACHMENT0, 
						   GL_TEXTURE_2D,
						   sBuf.shadowTex, 
						   0);
	GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);

	if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ) {
		DD_Terminal::post("ERROR::FRAMEBUFFER:: Shadow Framebuffer is not complete!\n");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return sBuf;
}

ParticleBuffer RendSpace::CreateParticleBuffer(const int width, const int height)
{
	ParticleBuffer pBuf = ParticleBuffer();

	// create and bind fbo
	glGenFramebuffers(1, &pBuf.particleFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, pBuf.particleFBO);

	// depth buffer
	glGenRenderbuffers(1, &pBuf.depthBuf);
	glBindRenderbuffer(GL_RENDERBUFFER, pBuf.depthBuf);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

	// color buffer
	CreateTex(GL_TEXTURE2, GL_RGBA16F, pBuf.colorTex, width, height);

	// attach images to framebuffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
							  GL_RENDERBUFFER, pBuf.depthBuf);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
						   pBuf.colorTex, 0);

	GLenum drawBuffers[] = { GL_NONE, GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(2, drawBuffers);

	if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ) {
		DD_Terminal::post(
			"\nERROR::FRAMEBUFFER:: Particle Framebuffer is not complete!\n");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return pBuf;
}

CubeMapBuffer RendSpace::CreateCubeMapBuffer(const int width, const int height,
											 DD_Skybox* sb)
{
	CubeMapBuffer cBuf = CubeMapBuffer();

	// create and bind fbo
	glGenFramebuffers(1, &cBuf.cubeFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, cBuf.cubeFBO);

	// create cubemap texture
	sb->GenerateNull(width, height);

	// depth buffer
	glGenRenderbuffers(1, &cBuf.depthBuf);
	glBindRenderbuffer(GL_RENDERBUFFER, cBuf.depthBuf);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

	// attach depth buffer
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT,
							  GL_RENDERBUFFER, cBuf.depthBuf);

	GLenum drawBuffers[] = { GL_NONE, GL_COLOR_ATTACHMENT0 }; // bind cubeMap for render
	glDrawBuffers(2, drawBuffers);

	if( glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE ) {
		DD_Terminal::post("\nERROR::FRAMEBUFFER:: CubeMap Framebuffer is not complete!\n");
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return cBuf;
}

FilterBuffer RendSpace::CreateFilterBuffer(const int width, 
										   const int height,
										   ShadowBuffer* sbuf)
{
	FilterBuffer fBuf = FilterBuffer();

	// create and bind fbo 0
	glGenFramebuffers(1, &fBuf.filterFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, fBuf.filterFBO);
	GLfloat border[] = { 1.0f, 1.0f, 1.0f, 1.0f };

	// color buffer
	CreateTex(GL_TEXTURE0, GL_RGBA16F, fBuf.colorTex, width, height);
	// attach images to framebuffer
	glFramebufferTexture2D(GL_FRAMEBUFFER,
						   GL_COLOR_ATTACHMENT0,
						   GL_TEXTURE_2D,
						   fBuf.colorTex,
						   0);
	GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, drawBuffers);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		DD_Terminal::post("\nERROR::FRAMEBUFFER::Filter Framebuffer 0 "
						  "is not complete!\n");
	}

	// create and bind fbo 1
	glGenFramebuffers(1, &fBuf.filterDepthFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, fBuf.filterDepthFBO);
	// depth map
	glGenTextures(1, &fBuf.depthTex);
	glBindTexture(GL_TEXTURE_2D, fBuf.depthTex);
	// texture paramenters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
	glTexStorage2D(GL_TEXTURE_2D, 1, GL_RG32F, sbuf->width, sbuf->height);

	// attach images to framebuffer
	glFramebufferTexture2D(GL_FRAMEBUFFER,
						   GL_COLOR_ATTACHMENT1,
						   GL_TEXTURE_2D,
						   fBuf.depthTex,
						   0);

	GLenum drawBuffers2[] = { GL_NONE, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, drawBuffers2);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		DD_Terminal::post("\nERROR::FRAMEBUFFER::Filter Framebuffer 1 "
						  "is not complete!\n");
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return fBuf;
}

void RendSpace::screenShot(const char* sig, 
						   const float game_time,
						   const unsigned width,
						   const unsigned height)
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, 
				 &pixel_write_buffer[0]);
	cbuff<128> img_name;
	img_name.format("%sCapture/%s_%u.dds", RESOURCE_DIR, sig, 
					(unsigned)(game_time * 10000.f));
	SOIL_save_image(img_name.str(),
					SOIL_SAVE_TYPE_DDS,
					width,
					height,
					4,
					&pixel_write_buffer[0]);
}

DD_Renderer::DD_Renderer() :
	m_lumiTexSizes(20),
	m_objSink(1024),
	m_lowDist(10),
	m_highDist(10),
	m_shaders(Shaders::NUM_SHADERS),
	m_flagVR(false),
	m_flagCubeMap(false),
	m_flagShadow(true),
	m_flagLineRend(true),
	m_flagDCM(false)
{
	bgcol[0] = 0.f; bgcol[1] = 0.f; bgcol[2] = 0.f; bgcol[3] = 1.f;
}

DD_Event DD_Renderer::RenderHandler(DD_Event& event)
{
	float currTime = 0.0f, currFrameTime = 0.0f;

	// Parse throught events
	if( event.m_type == "render" ) {
		Draw(m_time->getFrameTime());

		if (m_debugOn) {
			// Debug
			currTime = m_time->getTimeFloat();
			currFrameTime = m_time->getFrameTime();

			ImGuiWindowFlags window_flags = 0;
			ImGui::Begin("Render Stats Window", nullptr, window_flags);
			debug_cbuff.format(
				"Run Time: %.3fs (time to render last frame: %.3fs)",
				currTime, currFrameTime);
			ImGui::Text(debug_cbuff.str());
			debug_cbuff.format("Avg ms per frame: %.3f", currFrameTime * 1000.f);
			ImGui::Text(debug_cbuff.str());
			debug_cbuff.format("(%.3f FPS)", 1.0f / currFrameTime);
			ImGui::Text(debug_cbuff.str());
			debug_cbuff.format("Triangles sent to GPU: %u", trisInFrame);
			ImGui::Text(debug_cbuff.str());
			ImGui::End();
		}
	}
	if( event.m_type == "rendstat" ) {
		m_debugOn ^= 1;
	}
	if (event.m_type == "toggle_screenshots") {
		screen_shot_on ^= 1;
		screen_shot_name = "dd";
	}
	return DD_Event();
}

void DD_Renderer::LoadRendererEngine(const GLfloat _Width, const GLfloat _Height)
{
	m_Width = _Width;
	m_Height = _Height;
	m_gbuffer = RendSpace::CreateGBuffer((int)m_Width, (int)m_Height);
	m_resourceBin->G_BUFFER = m_gbuffer.deferredFBO;
	pixel_write_buffer.resize((size_t)m_Width * (size_t)m_Height * 4);
	GLenum err;

	m_lbuffer = RendSpace::CreateLightBuffer((int)m_Width, (int)m_Height);
	m_sbuffer = RendSpace::CreateShadowBuffer(2048, 2048);
	m_pbuffer = RendSpace::CreateParticleBuffer((int)m_Width, (int)m_Height);
	m_fbuffer = RendSpace::CreateFilterBuffer(
		(int)m_Width, (int)m_Height, &m_sbuffer);

	// create VBO buffer for joint matrices
	glGenBuffers(1, &m_jointVBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_jointVBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_JOINTS * sizeof(glm::mat4), NULL,
		GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, m_jointVBO);

	while( (err = glGetError()) != GL_NO_ERROR ) {
		DD_Terminal::f_post("DD_RenderEngine <0> :: OpenGL error: %d", err);
	}

	// create gbuffer shader
	m_shaders[Shaders::GBUFFER].init();
	m_shaders[Shaders::GBUFFER].CreateVertexShader(
		(std::string(SHADER_DIR) + "GBuffer_V.vert").c_str());
	m_shaders[Shaders::GBUFFER].CreateFragShader(
		(std::string(SHADER_DIR) + "GBuffer_F.frag").c_str());

	// create standard shader
	m_shaders[Shaders::LIGHT].init();
	m_shaders[Shaders::LIGHT].CreateVertexShader(
		(std::string(SHADER_DIR) + "Lighting_V.vert").c_str());
	m_shaders[Shaders::LIGHT].CreateFragShader(
		(std::string(SHADER_DIR) + "Lighting_F.frag").c_str());

	// create skinned mesh gbuffer
	m_shaders[Shaders::SKINNED].init();
	m_shaders[Shaders::SKINNED].CreateVertexShader(
		(std::string(SHADER_DIR) + "SkinnedGBuffer_V.vert").c_str());
	m_shaders[Shaders::SKINNED].CreateFragShader(
		(std::string(SHADER_DIR) + "GBuffer_F.frag").c_str());

	// create stencil shader
	m_shaders[Shaders::LIGHT_STENCIL].init();
	m_shaders[Shaders::LIGHT_STENCIL].CreateVertexShader(
		(std::string(SHADER_DIR) + "Lighting_Stencil_V.vert").c_str());
	m_shaders[Shaders::LIGHT_STENCIL].CreateFragShader(
		(std::string(SHADER_DIR) + "Lighting_Stencil_F.frag").c_str());

	// create post processing shader
	m_shaders[Shaders::POST_PROCESS].init();
	m_shaders[Shaders::POST_PROCESS].CreateVertexShader(
		(std::string(SHADER_DIR) + "PostProcess_V.vert").c_str());
	m_shaders[Shaders::POST_PROCESS].CreateFragShader(
		(std::string(SHADER_DIR) + "PostProcess_F.frag").c_str());

	// create text shader
	m_shaders[Shaders::TEXT].init();
	m_shaders[Shaders::TEXT].CreateVertexShader(
		(std::string(SHADER_DIR) + "Text_V.vert").c_str());
	m_shaders[Shaders::TEXT].CreateFragShader(
		(std::string(SHADER_DIR) + "Text_F.frag").c_str());

	// create depth shader
	m_shaders[Shaders::DEPTH].init();
	m_shaders[Shaders::DEPTH].CreateVertexShader(
		(std::string(SHADER_DIR) + "Depth_V.vert").c_str());
	m_shaders[Shaders::DEPTH].CreateFragShader(
		(std::string(SHADER_DIR) + "Depth_F.frag").c_str());

	// create skinned mesh depth shader
	m_shaders[Shaders::DEPTH_SKINNED].init();
	m_shaders[Shaders::DEPTH_SKINNED].CreateVertexShader(
		(std::string(SHADER_DIR) + "SkinnedDepth_V.vert").c_str());
	m_shaders[Shaders::DEPTH_SKINNED].CreateFragShader(
		(std::string(SHADER_DIR) + "Depth_F.frag").c_str());

	// create depth sampler shader
	m_shaders[Shaders::TEX_SAMPLER].init();
	m_shaders[Shaders::TEX_SAMPLER].CreateVertexShader(
		(std::string(SHADER_DIR) + "TexSP_V.vert").c_str());
	m_shaders[Shaders::TEX_SAMPLER].CreateFragShader(
		(std::string(SHADER_DIR) + "TexSP_F.frag").c_str());

	// create 3d line shader
	m_shaders[Shaders::LINES_3D].init();
	m_shaders[Shaders::LINES_3D].CreateVertexShader(
		(std::string(SHADER_DIR) + "LineSeg_V.vert").c_str());
	m_shaders[Shaders::LINES_3D].CreateFragShader(
		(std::string(SHADER_DIR) + "LineSeg_F.frag").c_str());

	// load particle engine
	m_particleSys = new DD_ParticleSys();
	m_particleSys->m_resBin = m_resourceBin;
	m_particleSys->Load(_Width, _Height);

	// hdr compute shader and outputs
	int tex_w = (int)m_Width, tex_h = (int)m_Height;
	int limit = std::min(tex_h, tex_w);
	size_t index = 0;
	while( limit > 4 ) {
		// first change texture size
		tex_w = tex_w / 4;
		tex_h = tex_h / 4;
		limit = std::min(tex_h, tex_w);
		m_lumiTexSizes[index] = tex_w;
		index += 1;
		m_lumiTexSizes[index] = tex_h;
		index += 1;
	}
	// resize texture size array
	dd_array<int> temp(index);
	temp = m_lumiTexSizes;
	m_lumiTexSizes = std::move(temp);
	// use texture sizes for computing luminance
	int count = (int)index / 2;
	m_luminOutput = dd_array<GLuint>(count);
	m_luminValues = dd_array<GLfloat>(
		m_lumiTexSizes[2 * count - 2] * m_lumiTexSizes[2 * count - 1] * 4);
	for( int i = 0; i < count; ++i ) {
		//printf("compute_W: %d, compute_H: %d\n", this->lumiTexSizes[2 * i],
		//this->lumiTexSizes[2 * i + 1]);
		RendSpace::CreateTex(GL_TEXTURE0, GL_RGBA16F, m_luminOutput[i],
							 m_lumiTexSizes[2 * i], m_lumiTexSizes[2 * i + 1]);
	}

	// create luminence compute shader
	m_shaders[Shaders::LUMINANCE].init();
	m_shaders[Shaders::LUMINANCE].CreateComputeShader(
		(std::string(SHADER_DIR) + "Luminance_C.comp").c_str());

	// create blur compute shader
	m_shaders[Shaders::BLUR].init();
	m_shaders[Shaders::BLUR].CreateComputeShader(
		(std::string(SHADER_DIR) + "Blur_C.comp").c_str());

	// load light volumes
	cbuff<128> path;
	path.format("%s%s", MESH_DIR, "primitives/light_sphere.ddm");
	m_volumeSphere = ResSpace::loadModel_DDM(
		m_resourceBin, "light_sphere", path.str());
	ModelSpace::OpenGLBindMesh(0, *m_volumeSphere, 1, 0);

	// setup loading screen
	if( LoadScrSpace::LoadTextures((int)_Width, (int)_Height) ) {
		DD_Terminal::post("Load screen initialized...\n");
	}
}

// Assets to be loaded after resources are loaded
void DD_Renderer::LoadEngineAssets()
{
	ResSpace::initCameras(m_resourceBin, m_Width, m_Height);
	ResSpace::initCameras(m_resourceBin);
	ResSpace::initShaders(m_resourceBin);
	ResSpace::GetActiveCamera(m_resourceBin, m_active_cam);
	if( m_lvl_cubMap != "" ) {
		CreateCubeMap(m_lvl_cubMap.c_str());
	}
	// create and load camera for dynamic cube map generation
	m_cube_cam = ResSpace::getNewDD_Camera(m_resourceBin, "Cube_cam");
	m_cube_cam->scr_width = 1024;
	m_cube_cam->scr_height = 1024;
	m_cube_cam->fov_h = glm::radians(90.f);
	m_cube_cam->far_plane = 20000.f;
	if( m_flagDCM ) {
		DD_Skybox* sb = ResSpace::getNewDD_Skybox(m_resourceBin, "Dynamic");
		m_cbuffer = RendSpace::CreateCubeMapBuffer(1024, 1024, sb);
	}
}

// show shader attributes and uniform
void DD_Renderer::QueryShaders()
{
	for( size_t i = 0; i < Shaders::NUM_SHADERS; i++ ) {
		switch( i ) {
			case Shaders::GBUFFER:
				printf("\nGBuffer Shader: \n");
				break;
			case Shaders::LIGHT:
				printf("\nLight Shader: \n");
				break;
			case Shaders::LIGHT_STENCIL:
				printf("\nLight Stencil Shader: \n");
				break;
			case Shaders::POST_PROCESS:
				printf("\nPost Process Shader: \n");
				break;
			case Shaders::LUMINANCE:
				printf("\nLuminance Shader: \n");
				break;
			default:
				break;
		}
		m_shaders[i].QueryShaderAttributes();
		m_shaders[i].QueryUniforms();
	}
}

void DD_Renderer::Draw(float dt)
{
	// reset draw information
	drawCallsInFrame = 0;
	trisInFrame = 0;
	linesInFrame = 0;
	m_objectsInFrame = 0;

	if( m_flagDCM ) {
		DrawDynamicCube(dt);
	}

	// grab camera data
	ResSpace::GetActiveCamera(m_resourceBin, m_active_cam);
	m_flagVR = m_active_cam->vr_cam;
	cam_eye_dist = m_active_cam->cam_eye_dist;
	glm::mat4 viewMat = CamSpace::GetViewMatrix(m_active_cam);
	glm::mat4 projMat = CamSpace::GetPerspecProjMatrix(m_active_cam);
	glm::mat4 identity;
	FrustumBox frust = m_active_cam->frustum();

	// Cull object out of frame
	CullObjects(frust, viewMat);

	if( m_flagVR ) {
		if( m_active_cam->vr_window ) {
			projMat = CamSpace::GetOffAxisProjMatrix(
				m_active_cam,
				m_active_cam->vr_scr_pos,
				m_scrHorzDist, m_scrVertDist, m_active_cam->vr_eye_pos);
		}
		/**/

		VR_Eye eye_select[] = { VR_Eye::LEFT, VR_Eye::RIGHT };
		for( int i = 0; i < 2; ++i ) {
			if( eye_select[i] == VR_Eye::LEFT ) {
				viewMat = CamSpace::GetViewMatrixVREye(m_active_cam, true, false,
													   m_active_cam->cam_eye_dist);
			}
			else {
				viewMat = CamSpace::GetViewMatrixVREye(m_active_cam, false, true,
													   m_active_cam->cam_eye_dist);
			}
			GBufferPass(dt, viewMat, projMat);
			// shadow pass (for directional lights)
			ShadowPass(dt);

			// get standard shader
			DD_Shader* shader = &m_shaders[Shaders::LIGHT];
			LightPass(shader, m_active_cam, viewMat, projMat);

			// Draw skybox
			shader = &m_shaders[Shaders::LIGHT];
			shader->Use();
			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LEQUAL);
			if( m_flagCubeMap ) {
				shader->setUniform("DrawSky", (GLboolean)true);
				glActiveTexture(GL_TEXTURE5);
				shader->setUniform("skybox", 5);
				glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubeMapHandle);

				glm::mat4 model = glm::scale(glm::mat4(), glm::vec3(200.0f));
				glm::mat4 skyView = glm::mat4(glm::mat3(viewMat)); // remove translation
				RendSpace::RenderCube(shader, projMat * skyView * model);
				// update info
				drawCallsInFrame += 1;
				trisInFrame += 12;
			}
			glDepthFunc(GL_LESS);

			// Render particles
			glBindFramebuffer(GL_FRAMEBUFFER, m_pbuffer.particleFBO);
			glClear(GL_COLOR_BUFFER_BIT);

			glBindFramebuffer(GL_READ_FRAMEBUFFER, m_gbuffer.deferredFBO);
			// Write depth to framebuffer
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pbuffer.particleFBO);
			glBlitFramebuffer(0, 0, (GLint)m_Width, (GLint)m_Height, 0, 0, 
							  (GLint)m_Width, (GLint)m_Height, 
							  GL_DEPTH_BUFFER_BIT, GL_NEAREST);

			m_particleSys->Draw(dt, viewMat, projMat, glm::vec3(m_active_cam->pos()),
								m_pbuffer.particleFBO, m_gbuffer.deferredFBO);

			// Perform post processing using default framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDisable(GL_DEPTH_TEST);

			// hdri tone mapping
			const GLboolean doTone = false;
			float avgLum = 1.f;
			if( doTone ) {
				avgLum = ComputeLuminance();
			}
			//printf("Avg lum: %.3f\n", avgLum);

			// reset shader
			shader = &m_shaders[Shaders::POST_PROCESS];
			shader->Use();
			RendSpace::BindPassTexture(shader, &m_lbuffer);
			RendSpace::BindPassTexture(shader, &m_pbuffer);
			shader->setUniform("DoToneMap", doTone);
			shader->setUniform("AveLum", avgLum);
			shader->setUniform("Exposure", 0.75f); // control exposure
			shader->setUniform("White", 0.97f); // control white value

			// shadows
			/*BindPassTexture(shader, &this->sbuffer);
			shader->setUniform("LSM", this->m_currentLSM);
			shader->setUniform("SampleShadow", GLboolean(false));*/

			RendSpace::RenderVR_Split(shader, identity, (int)eye_select[i]);
			// update info
			drawCallsInFrame += 1;
			trisInFrame += 2;
		}
	}
	else {
		// gbuffer pass
		GBufferPass(dt, viewMat, projMat);

		// Draw lines
		if( m_flagLineRend ) {
			LinePass(dt, viewMat, projMat);
		}

		// shadow pass (for directional lights)
		glClearColor(1.f, 1.f, 1.f, 1.f); // white background for shadow pass
		ShadowPass(dt);

		glClearColor(0.f, 0.f, 0.f, 1.f); // black blackground for light pass
		// get standard shader
		DD_Shader* shader = &m_shaders[Shaders::LIGHT];
		LightPass(shader, m_active_cam, viewMat, projMat);

		// Draw skybox
		// may need to place in a different order (draw first)
		shader = &m_shaders[Shaders::LIGHT];
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LEQUAL);
		if( m_flagCubeMap ) {
			shader->Use();
			shader->setUniform("DrawSky", (GLboolean)true);
			glActiveTexture(GL_TEXTURE5);
			shader->setUniform("skybox", 5);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubeMapHandle);

			if (m_flagDCM) {
				DD_Skybox* sb = ResSpace::findDD_Skybox(m_resourceBin, "Dynamic");
				glBindTexture(GL_TEXTURE_CUBE_MAP, sb->handle);
			}

			glm::mat4 model = glm::scale(glm::mat4(), glm::vec3(200.0f));
			glm::mat4 skyView = glm::mat4(glm::mat3(viewMat)); // remove translation
			RendSpace::RenderCube(shader, projMat * skyView * model);
			// update info
			drawCallsInFrame += 1;
			trisInFrame += 12;
		}
		glDepthFunc(GL_LESS);

		// Render particles
		glBindFramebuffer(GL_FRAMEBUFFER, m_pbuffer.particleFBO);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, m_gbuffer.deferredFBO);
		// Write depth to framebuffer
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_pbuffer.particleFBO);
		glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, (GLint)m_Width, 
						  (GLint)m_Height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		bool blend_particle = m_particleSys->Draw(
			dt, viewMat, projMat, glm::vec3(m_active_cam->pos()), 
			m_pbuffer.particleFBO, m_gbuffer.deferredFBO
		);

		// Perform post processing using default framebuffer
		// restore backgound color

		// base background color
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDisable(GL_DEPTH_TEST);

		// hdri tone mapping
		const GLboolean doToneMap = true;
		float avgLum = 1.f;
		if( doToneMap ) {
			avgLum = ComputeLuminance();
		}
		//printf("Avg lum: %.3f\n", avgLum);

		// reset shader and post process
		shader = &m_shaders[Shaders::POST_PROCESS];
		shader->Use();
		RendSpace::BindPassTexture(shader, &m_lbuffer);
		if (blend_particle) {
			RendSpace::BindPassTexture(shader, &m_pbuffer);
			shader->setUniform("BlendParticle", blend_particle);
		}
		else {
			shader->setUniform("BlendParticle", blend_particle);
		}
		shader->setUniform("GammaCorrect", true);
		shader->setUniform("Blur", false);
		shader->setUniform("output2D", false);
		shader->setUniform("DoToneMap", doToneMap);
		shader->setUniform("AveLum", avgLum);
		shader->setUniform("SampleShadow", false);
		shader->setUniform("Exposure", 0.75f); // control exposure
		shader->setUniform("White", 0.97f); // control white value
		// sample shadow
		/*
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_fbuffer.depthTex);
		//glBindTexture(GL_TEXTURE_2D, m_sbuffer.shadowTex);
		shader->setUniform("SampleShadow", true);
		//*/
		
		RendSpace::RenderQuad(shader, identity);
		if (screen_shot_on) {
			//screen_shot_on = false;
			fps60_tick = 0.f;
			RendSpace::screenShot(
				screen_shot_name.str(), m_time->getTimeFloat(), (GLint)m_Width,
				(GLint)m_Height);
		}

		trisInFrame += 2;

		// Forward Rendering
		//glEnable(GL_DEPTH_TEST);
		//glBindFramebuffer(GL_READ_FRAMEBUFFER, m_pbuffer.particleFBO);
		//// Write depth to default framebuffer
		//glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		//glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, m_Width, m_Height,
		//	GL_DEPTH_BUFFER_BIT, GL_NEAREST);

		// update info
		drawCallsInFrame += m_resourceBin->emitter_counter + 1;
		fps60_tick += dt;
	}
}

void DD_Renderer::DrawDynamicCube(float dt)
{
	GLenum err;
	dynamic_cb_interval += dt;

	if( dynamic_cb_interval > 0.01f || dynamic_cb_tic == 0 ) {
		dynamic_cb_interval = 0.f;	// draw during interval
		dynamic_cb_tic += 1;
	}
	else {
		return; // skip otherwise
	}

	GLenum targets[] = {
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
	};
	glm::quat cam_rots[] = {
		glm::quat(glm::vec3(0.f, glm::radians(-90.f), 0.f)),
		glm::quat(glm::vec3(0.f, glm::radians(90.f), 0.f)),
		glm::quat(glm::vec3(0.f, glm::radians(180.f), 0.f)),
		glm::quat(glm::vec3(0.f, 0.f, 0.f)),
		glm::quat(glm::vec3(glm::radians(90.f), 0.f, 0.f)),
		glm::quat(glm::vec3(glm::radians(-90.f), 0.f, 0.f))
	};
	glm::vec4 _p = m_active_cam->pos();
	//glm::vec4 _p = glm::vec4(0.f, 400.f, 0.f, 1.f);
	//glm::vec4 _p = glm::vec4(0.f, m_active_cam->pos().y, 0.f, 1.f);

	// get skybox
	DD_Skybox* sb = ResSpace::findDD_Skybox(m_resourceBin, "Dynamic");

	//glBindFramebuffer(GL_FRAMEBUFFER, m_gbuffer.deferredFBO);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, m_pbuffer.particleFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	while( (err = glGetError()) != GL_NO_ERROR ) {
		DD_Terminal::f_post("DD_RenderEngine <Cube_A> :: OpenGL error: %d", err);
	}

	for( size_t i = 0; i < 6; i++ ) {
		// update camera
		m_cube_cam->updateCamera(_p, cam_rots[i]);

		glm::mat4 viewMat = CamSpace::GetViewMatrix(m_cube_cam);
		glm::mat4 projMat = CamSpace::GetPerspecProjMatrix(m_cube_cam);
		glm::mat4 identity;
		FrustumBox frust = m_cube_cam->frustum();
		// Cull object out of frame (culling based on results from last frame)
		CullObjects(frust, viewMat);

		GBufferPass(dt, viewMat, projMat);

		// get standard shader
		DD_Shader* shader = &m_shaders[Shaders::LIGHT];
		shader->Use();
		LightPass(shader, m_cube_cam, viewMat, projMat);

		// Draw skybox
		shader = &m_shaders[Shaders::LIGHT];
		shader->Use();
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LEQUAL);
		if( m_flagCubeMap ) {
			shader->setUniform("DrawSky", (GLboolean)true);
			glActiveTexture(GL_TEXTURE5);
			shader->setUniform("skybox", 5);
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubeMapHandle);

			glm::mat4 model = glm::scale(glm::mat4(), glm::vec3(200.0f));
			glm::mat4 skyView = glm::mat4(glm::mat3(viewMat)); // remove translation
			RendSpace::RenderCube(shader, projMat * skyView * model);
			// update info
			drawCallsInFrame += 1;
			trisInFrame += 12;
		}
		glDepthFunc(GL_LESS);

		// Perform post processing using cube map buffer
		glBindFramebuffer(GL_FRAMEBUFFER, m_cbuffer.cubeFBO);
		RendSpace::BindPassTexture(shader, &m_cbuffer, targets[i], sb);
		glDisable(GL_DEPTH_TEST);

		// reset shader
		shader = &m_shaders[Shaders::POST_PROCESS];
		shader->Use();
		RendSpace::BindPassTexture(shader, &m_lbuffer);
		RendSpace::BindPassTexture(shader, &m_pbuffer);
		shader->setUniform("DoToneMap", false);
		shader->setUniform("AveLum", 1.f);
		shader->setUniform("Exposure", 0.75f); // control exposure
		shader->setUniform("White", 0.97f); // control white value

		glViewport(0, 0, sb->m_width, sb->m_height); // set dimensions
		bool up_down = (i < 4) ? false : true;
		shader->setUniform("flip_y_coord", true);
		shader->setUniform("flip_x_coord", true);
		if( up_down ) {
			shader->setUniform("uv_rotate", true);
			// create rotation matrix
			glm::mat4 r_mat = glm::translate(
				glm::mat4(), glm::vec3(0.5f, 0.5f, 0.f));
			r_mat = glm::rotate(
				r_mat, glm::radians(180.f), glm::vec3(0.f, 0.f, 1.f));
			r_mat = glm::translate(r_mat, glm::vec3(-0.5f, -0.5f, 0.f));
			shader->setUniform("rotate_uv_mat", r_mat);
		}
		RendSpace::RenderQuad(shader, identity);

		shader->setUniform("uv_rotate", false);
		shader->setUniform("flip_y_coord", false);
		shader->setUniform("flip_x_coord", false);
		glViewport(0, 0, (GLsizei)m_Width, (GLsizei)m_Height); // reset dimensions
		// update info
		trisInFrame += 2;
		drawCallsInFrame += m_resourceBin->emitter_counter + 1;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	while( (err = glGetError()) != GL_NO_ERROR ) {
		DD_Terminal::f_post("DD_RenderEngine <Cube_B> :: OpenGL error: %d", err);
	}
}

void DD_Renderer::LinePass(GLfloat dt, glm::mat4& view, glm::mat4& proj)
{
	glBindFramebuffer(GL_FRAMEBUFFER, m_gbuffer.deferredFBO);
	DD_Shader* shader = &m_shaders[Shaders::LINES_3D];
	shader->Use();

	for( size_t i = 0; i < m_resourceBin->l_agent_counter; i++ ) {
		DD_LineAgent* l_agent = &m_resourceBin->lines[i];
		// draw lines
		if( !l_agent->m_buffer.isValid() ) {
			l_agent->m_buffer.resize(l_agent->lines.size() * 2);
		}
		if( l_agent->flag_render && l_agent->m_buffer.size() > 0 ) {
			for( size_t j = 0; j < l_agent->lines.size(); j++ ) {
				l_agent->m_buffer[j * 2] = l_agent->lines[j].pos01;
				l_agent->m_buffer[j * 2 + 1] = l_agent->lines[j].pos02;
			}
			RendSpace::RenderLine(shader, proj * view, l_agent->m_buffer,
								  l_agent->color);
		}
	}
}

void DD_Renderer::DrawLoadScreen(float timeElasped)
{
	DD_Shader* shader = &m_shaders[Shaders::POST_PROCESS];
	shader->Use();

	LoadScrSpace::DrawLoadScreen(timeElasped, shader);
}

/*
void DD_Renderer::DrawTextToScreen(std::string text, GLfloat x, GLfloat y,
	GLfloat scale, glm::vec3 color)
{
	DD_Shader* shader = &m_shaders[Shaders::TEXT];
	if (currentFont == nullptr) {
		currentFont = new DD_Text();
		*currentFont = TextSpace::InitFontLib("calibri", 0, 36, m_Width, m_Height);
	}

	TextSpace::RenderText(shader, *currentFont, text, x, y, scale, color);
}
//*/

// Cull objects w/ frustum culling and LOD
void DD_Renderer::CullObjects(FrustumBox& frustum, glm::mat4& viewMat)
{

	//#pragma omp parallel for
	for( int i = 0; i < (int)m_resourceBin->m_num_agents; i++ ) {
		DD_Agent* agent = m_resourceBin->agents[i];
		agent->cleanInst();

		if( agent->flag_model || agent->flag_modelsk ) {
			FrustumCull(*agent, frustum, viewMat);
			if( agent->flag_cull_inst[0] != -1 ) {
				m_objSink[m_objectsInFrame] = i;
				m_objectsInFrame += 1;
			}
		}
		else {
			// agents w/out models gets automatically added to frame objs
			agent->flag_cull_inst[0] = -1;
			m_objSink[m_objectsInFrame] = i;
			m_objectsInFrame += 1;
		}
	}
}

void DD_Renderer::FrustumCull(DD_Agent& obj, FrustumBox& frust, glm::mat4& view)
{
	if( obj.inst_m4x4.size() == 1 ) {
		glm::mat4 transMat = obj.parent_transform * obj.inst_m4x4[0];
		BoundingBox bbox = obj.BBox.transformCorners(transMat);
		obj.inst_sink_size = 0;

		// for loop thru planes
		for( size_t i = 0; i < 6; i++ ) {
			glm::vec3 frustNorm = frust.normals[i];
			GLfloat D = frust.d[i];
			// check if positive vertex is outside (positive vert depends
			// upon normal of plane)
			glm::vec3 maxP = bbox.GetFrustumPlaneMax(frustNorm);
			GLfloat distance = glm::dot(frustNorm, maxP) + D;
			if( distance < 0.0001f ) {
				// instance sink size equals 0
				return;
			}
		}
		obj.flag_cull_inst[0] = 0;
		obj.f_inst_m4x4[0] = obj.inst_m4x4[0];
		// Colors buffer
		if( obj.flag_color ) {
			obj.f_inst_colors[0] = obj.inst_colors[0];
		}
		obj.inst_sink_size += 1;
		// LOD cull
		if( obj.mesh_buffer.size() > 1 ) {
			LevelOfDetail(obj, 0, view);
		}
	}
	else {
		// frustum cull instance matrices
		obj.inst_sink_size = 0;

		size_t numInstances = obj.inst_m4x4.size();
		for( size_t j = 0; j < numInstances; j++ ) {
			glm::mat4 transMat = obj.parent_transform * obj.inst_m4x4[j];
			BoundingBox bbox = obj.BBox.transformCorners(transMat);
			bool skipMat = false;

			// for loop thru planes
			for( size_t i = 0; i < 6; i++ ) {
				glm::vec3 frustNorm = frust.normals[i];
				GLfloat D = frust.d[i];
				// check if positive vertex is outside (positive vert
				// depends upon normal of plane)
				glm::vec3 maxP = bbox.GetFrustumPlaneMax(frustNorm);
				GLfloat distance = glm::dot(frustNorm, (maxP)) + D;
				if( distance < 0.0001f ) {
					// skip matrix
					skipMat = true;
					break;
				}
			}
			if( !skipMat ) {
				obj.f_inst_m4x4[obj.inst_sink_size] = obj.inst_m4x4[j];
				// Colors buffer
				if( obj.flag_color ) {
					obj.f_inst_colors[obj.inst_sink_size] = obj.inst_colors[j];
				}
				obj.inst_sink_size += 1;
				// LOD cull
				if( obj.mesh_buffer.size() > 1 ) {
					LevelOfDetail(obj, (int)obj.inst_sink_size - 1, view);
				}
			}
		}
	}
}

void DD_Renderer::LevelOfDetail(DD_Agent & obj,
								const int objInstIndex,
								glm::mat4 & viewMat)
{
	GLfloat euclid_dist = 0;
	glm::mat4 newTrans = viewMat * obj.f_inst_m4x4[objInstIndex];
	glm::vec3 pos = glm::vec3(newTrans[3]);
	euclid_dist = glm::length(pos);
	// instance must lie in this z range
	// set up z distances
	for( size_t i = 0; i < obj.mesh_buffer.size(); i++ ) {
		m_lowDist[i] = obj.mesh_buffer[i].nearLOD;
		m_highDist[i] = obj.mesh_buffer[i].farLOD;
	}
	size_t lodChoice = 0;
	// check z distances
	for( size_t i = 0; i < obj.mesh_buffer.size(); i++ ) {
		if( euclid_dist > m_lowDist[i] && euclid_dist < m_highDist[i] ) {
			lodChoice = i;
			break;
		}
		lodChoice += 1;
	}
	// do not render if it falls past all z distances
	if( lodChoice == obj.mesh_buffer.size() ) {
		obj.inst_sink_size -= 1;
	}
	else {
		// set lod flag
		obj.flag_cull_inst[objInstIndex] = (int)lodChoice;
	}
}

void DD_Renderer::GBufferPass(GLfloat dt,
							  glm::mat4& view,
							  glm::mat4& proj,
							  const bool clip,
							  glm::vec4 c_plane)
{
	// gbuffer pass
	glBindFramebuffer(GL_FRAMEBUFFER, m_gbuffer.deferredFBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE); // cull back faces
	SortSpace::SetUp();

	DD_Shader* shader = &m_shaders[Shaders::GBUFFER];
	// force clipping if neccesary
	if( clip ) {
		glEnable(GL_CLIP_DISTANCE0);
		shader->setUniform("enable_clip1", true);
		shader->setUniform("cplane_01", c_plane);
	}
	for( size_t i = 0; i < m_objectsInFrame; i++ ) {
		DD_Agent* agent = m_resourceBin->agents[m_objSink[i]];
		if((agent->flag_model || agent->flag_modelsk) && agent->flag_render){
			if( agent->inst_sink_size > 1 ) {
				// Sort instances
				const int size = (int)agent->inst_sink_size - 1;
				SortSpace::QuickSort(agent, 0, size, SortSpace::PartitionLOD);
				// render with instance buffer
				InstanceSetup(dt, view, proj, agent, shader, true);
			}
			if( agent->inst_sink_size == 1 ) {
				// render as single instance
				InstanceSetup(dt, view, proj, agent, shader);
			}
		}
	}
	if( clip ) {
		glDisable(GL_CLIP_DISTANCE0);
		shader->setUniform("enable_clip1", false);
	}
}

void DD_Renderer::ShadowPass(GLfloat dt, VR_Eye eye)
{
	// render shadows using two lights (for now)
	DD_Light* shadowL = GetDirectionalShadowLight(m_resourceBin);

	if( shadowL ) {
		// sbuffer pass
		glBindFramebuffer(GL_FRAMEBUFFER, m_sbuffer.depthMapFBO);
		glViewport(0, 0, m_sbuffer.width, m_sbuffer.height); // render shadow dimensions
		DD_Shader* shader = &m_shaders[Shaders::DEPTH];
		shader->Use();
		// cull front faces (peter panning due to shadow bias)
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		//glCullFace(GL_FRONT);
		// clear fbo
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// calculate lightspace matrix
		glm::vec4 pos =
			shadowL->parent_transform * glm::vec4(shadowL->_position, 1.0f);
		glm::vec3 center;
		if( shadowL->m_type != LightType::POINT_L ) { // is not in use for now
			center - glm::vec3();
		}
		if( shadowL->m_type != LightType::SPOT_L ) { // is not in use for now
			center - glm::vec3();
		}
		if( shadowL->m_type == LightType::DIRECTION_L ) {
			// set position based on the distance of the camera's far plane
			// and the direction of the light
			//const float dist = glm::length(pos);
			const float dist = m_active_cam->far_plane - m_active_cam->near_plane;
			pos = glm::vec4(0.f, dist, 0.f, 1.f);
			center = shadowL->m_direction + glm::vec3(pos);
		}
		glm::mat4 lightView = glm::lookAt(glm::vec3(pos), center,
										  glm::vec3(m_active_cam->worldUp()));
		// use camera's view frustum to calculate ortho boundaries
		glm::vec4 corners[8];
		for( size_t i = 0; i < 8; i++ ) {
			glm::vec3 temp = m_active_cam->frustum().corners[i];
			corners[i] = 
				lightView * glm::vec4(temp.x, temp.y, temp.z, 1.0f);
		}
		BoundingBox box = BoundingBox();
		box.corner1 = glm::vec3(corners[0]);
		box.corner2 = glm::vec3(corners[1]);
		box.corner3 = glm::vec3(corners[2]);
		box.corner4 = glm::vec3(corners[3]);
		box.corner5 = glm::vec3(corners[4]);
		box.corner6 = glm::vec3(corners[5]);
		box.corner7 = glm::vec3(corners[6]);
		box.corner8 = glm::vec3(corners[7]);
		glm::vec3 min = box.UpdateAABB_min();
		glm::vec3 max = box.UpdateAABB_max();

		glm::mat4 lightProjection = glm::ortho(
			glm::floor(min.x), glm::floor(max.x),
			glm::floor(min.y), glm::floor(max.y),
			m_active_cam->near_plane, m_active_cam->far_plane);
		glm::mat4 lightSpaceMatrix1 = lightProjection * lightView;
		shadowL->SetFrameLSM(lightSpaceMatrix1);

		glm::mat4 identity = glm::mat4();
		for( size_t i = 0; i < m_resourceBin->m_num_agents; i++ ) {
			DD_Agent* agent = m_resourceBin->agents[i];
			bool mdl_exists = agent->flag_model || agent->flag_modelsk;

			if(agent->flag_render && mdl_exists) {
				shader = (agent->flag_model) ? 
					&m_shaders[Shaders::DEPTH] : &m_shaders[Shaders::DEPTH_SKINNED];
				if( agent->inst_m4x4.size() > 1 ) {
					// render with instance buffer
					InstanceSetup(dt, identity, identity, agent, shader, true,
								  shadowL);
				}
				if( agent->inst_m4x4.size() == 1 ) {
					// render as single instance
					InstanceSetup(dt, identity, identity, agent, shader, false,
								  shadowL);
				}
			}
		}

		// blur depth map w/ filter FBO
		glBindFramebuffer(GL_FRAMEBUFFER, m_fbuffer.filterDepthFBO);
		glClear(GL_COLOR_BUFFER_BIT);

		shader = &m_shaders[Shaders::POST_PROCESS];
		shader->Use();
		shader->setUniform("BlendParticle", false);
		shader->setUniform("GammaCorrect", false);
		shader->setUniform("DoToneMap", false);
		shader->setUniform("AveLum", 0.0f);
		shader->setUniform("SampleShadow", false);
		shader->setUniform("Exposure", 0.75f); // control exposure
		shader->setUniform("White", 0.97f); // control white value
		
		shader->setUniform("Blur", true);
		shader->setUniform("output2D", true);
		// horizontal
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_sbuffer.shadowTex); 
		shader->setUniform("direction_flag", glm::vec2(1.f, 0.f));
		RendSpace::RenderQuad(shader, identity);
		// vertical
		shader->setUniform("direction_flag", glm::vec2(0.f, 1.f));
		RendSpace::RenderQuad(shader, identity);

		// reset uniforms
		shader->setUniform("Blur", false);
		shader->setUniform("output2D", false);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glViewport(0, 0, (GLsizei)m_Width, (GLsizei)m_Height); // reset dimensions
		//glCullFace(GL_BACK);
	}
}

void DD_Renderer::InstanceSetup(float dt,
								glm::mat4 & view,
								glm::mat4 & proj,
								DD_Agent * agent,
								DD_Shader * shader,
								const bool flagInstance,
								DD_Light* shadow)
{
	/// \brief Lambda to set size of instance buffer set to GPU
	auto calcBufferSize = [&](const unsigned num_instances, int& currentLOD)
	{
		for( size_t i = 0; i < num_instances; i++ ) {
			// set size of buffers (use as offset in pointer arithmetic)
			if( agent->flag_cull_inst[i] == currentLOD ) {
				m_LODBufferSize[currentLOD] += 1;
			}
			else {
				currentLOD = agent->flag_cull_inst[i];
				m_LODBufferSize[currentLOD] += 1;
			}
		}
	};

	size_t bufferOffset = 0;
	// reset temp buffer per object
	for( size_t i = 0; i < 10; i++ ) {
		m_LODBufferSize[i] = 0;
	}
	if( flagInstance ) {
		// render instanced geometry
		int currentLOD = 0;
		if( shadow ) {
			// shadow mapping (no culling is enabled on instances)
			calcBufferSize((unsigned)agent->inst_m4x4.size(), currentLOD);
		}
		else {
			// culling enabled
			calcBufferSize((unsigned)agent->inst_sink_size, currentLOD);
		}

		size_t numModels = agent->mesh_buffer.size();
		for( size_t j = 0; j < numModels; j++ ) {
			// skip when all instances are culled
			if( m_LODBufferSize[j] == 0 ) { continue; }

			bufferOffset += (j == 0) ? 0 : m_LODBufferSize[j - 1];
			// render mesh w/ instance buffers and offsets
			// (10 LOD levels supported)
			if ( agent->flag_modelsk ) {
				// render animated mesh
				shader = (shadow) ? shader : &m_shaders[Shaders::SKINNED];
				SkinnedMeshRender(shader, agent, j, bufferOffset, proj, view,
					shadow);
			}
			else {
				StaticMeshRender(shader, agent, j, bufferOffset, proj, view,
					shadow);
			}
		}
	}
	else {
		// render single instance (10 LOD levels supported)
		size_t m_idx = (agent->mesh_buffer.size() == 1) ?
			0 : agent->flag_cull_inst[0];
		m_LODBufferSize[m_idx] = 1;
		if ( agent->flag_modelsk ) {
			// render animated mesh
			shader = (shadow) ? shader : &m_shaders[Shaders::SKINNED];
			SkinnedMeshRender(shader, agent, m_idx, bufferOffset, proj, view,
				shadow);
		}
		else {
			StaticMeshRender(shader, agent, m_idx, bufferOffset, proj, view,
				shadow);
		}
	}

}

void DD_Renderer::StaticMeshRender(DD_Shader * shader,
								   DD_Agent * agent,
								   const size_t modelIndex,
								   const size_t bufferOffet,
								   glm::mat4& proj,
								   glm::mat4& view,
								   DD_Light* shadow)
{
	DD_Model* model = ResSpace::findDD_Model(
		m_resourceBin, agent->mesh_buffer[modelIndex].model.c_str());
	DD_Material* mat;
	shader->Use();
	// set mesh and material elements
	for( size_t i = 0; i < model->meshes.size(); i++ ) {
		if( !shadow ) {
			int mat_index =
				(agent->mat_buffer.size() < model->meshes.size()) ? -1 : (int)i;
			// set to default if 0
			mat = (mat_index < 0) ? 
				ResSpace::findDD_Material(m_resourceBin, (unsigned)0) :
				ResSpace::findDD_Material(m_resourceBin,
										(unsigned)agent->mat_buffer[mat_index]);

			// render mesh's materials to gbuffer
			shader->setUniform("albedoFlag", (GLboolean)mat->m_albedo);
			if( mat->m_albedo ) {
				glActiveTexture(GL_TEXTURE0);
				shader->setUniform("tex_albedo", 0);
				GLuint texID = mat->m_textures[TextureType::ALBEDO]->handle;
				glBindTexture(GL_TEXTURE_2D, texID);
			}
			shader->setUniform("specFlag", (GLboolean)mat->m_specular);
			if( mat->m_specular ) {
				glActiveTexture(GL_TEXTURE1);
				shader->setUniform("tex_specular", 1);
				GLuint texID = mat->m_textures[TextureType::SPECULAR]->handle;
				glBindTexture(GL_TEXTURE_2D, texID);
			}
			shader->setUniform("normalFlag", (GLboolean)mat->m_normal);
			if( mat->m_normal ) {
				glActiveTexture(GL_TEXTURE2);
				shader->setUniform("tex_normal", 2);
				GLuint texID = mat->m_textures[TextureType::NORMAL]->handle;
				glBindTexture(GL_TEXTURE_2D, texID);
			}
			shader->setUniform("diffuse", glm::vec3(mat->m_base_color));
			shader->setUniform("shininess", mat->shininess);

			// find way to calculate normal matrix for all instances
			glm::mat4 norm = glm::transpose(glm::inverse(
				agent->parent_transform * agent->f_inst_m4x4[0]));
			shader->setUniform("Norm", norm);
			shader->setUniform("QuadRender", GLboolean(false));
			shader->setUniform("MVP", proj * view * agent->parent_transform);

			// bind instance buffer (dynamic b/c it's bound per frame)
			glBindBuffer(GL_ARRAY_BUFFER, model->instVBO[i]);
			glBufferSubData(GL_ARRAY_BUFFER, 0,
							m_LODBufferSize[modelIndex] * sizeof(glm::mat4),
							&agent->f_inst_m4x4[0] + bufferOffet);
		}
		else {
			shader->setUniform("MVP", agent->parent_transform);
			shader->setUniform("LightSpace", shadow->GetLSM());

			// bind instance buffer (dynamic b/c it's bound per frame)
			glBindBuffer(GL_ARRAY_BUFFER, model->instVBO[i]);
			glBufferSubData(GL_ARRAY_BUFFER, 0,
							m_LODBufferSize[modelIndex] * sizeof(glm::mat4),
							&agent->inst_m4x4[0] + bufferOffet);
		}

		// color instance
		dd_array<glm::vec3> defaultColor(1);
		defaultColor[0] = glm::vec3(1.0f);
		glBindBuffer(GL_ARRAY_BUFFER, model->instColorVBO[i]);
		if( agent->flag_color ) {
			shader->setUniform("multiplierMat", GLboolean(true));
			glBufferSubData(GL_ARRAY_BUFFER, 0,
							m_LODBufferSize[modelIndex] * sizeof(glm::vec3),
							&agent->f_inst_colors[0] + bufferOffet);
		}
		else {
			shader->setUniform("multiplierMat", GLboolean(false));
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3),
							&defaultColor[0]);
		}
		shader->setUniform("useDebug", false);

		MeshData& MD = model->meshes[i];

		// log stats
		trisInFrame += MD.indices.size() / 3;
		drawCallsInFrame += 1;

		// bind mesh data
		glBindVertexArray(model->VAO[i]);
		glDrawElementsInstanced(GL_TRIANGLES, (unsigned)MD.indices.size(),
								GL_UNSIGNED_INT, 0, 
								(GLsizei)m_LODBufferSize[modelIndex]);

		// render bounding box if applicable

		//glDrawElements(GL_TRIANGLES, meshes[i].indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
}

/// \brief Render skinned meshes
void DD_Renderer::SkinnedMeshRender(DD_Shader * shader,
									DD_Agent * agent,
									const size_t modelIndex,
									const size_t bufferOffet,
									glm::mat4& proj,
									glm::mat4& view,
									DD_Light* shadow)
{
	DD_ModelSK* modelsk = ResSpace::findDD_ModelSK(
		m_resourceBin, agent->mesh_buffer[modelIndex].model.c_str());
	DD_Material* mat;
	shader->Use();
	// set mesh and material elements
	for( size_t i = 0; i < modelsk->meshes.size(); i++ ) {
		// bind buffer for matrix render
		DD_SkeletonPose* pose_info = &modelsk->m_finalPose;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_jointVBO);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
						pose_info->m_globalPose.sizeInBytes(),
						&(pose_info->m_globalPose[0]));
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 11, m_jointVBO);

		// DD_SkeletonPose* pose_info = &modelsk->m_finalPose;
		// for (unsigned j = 0; j < pose_info->m_globalPose.size(); j++) {
		// 	std::string jnt_uni = "Joints[" + std::to_string(j) + "]";
		// 	shader->setUniform(jnt_uni.c_str(), pose_info->m_globalPose[j]);
		// }

		if( !shadow ) {
			int mat_index =
				(agent->mat_buffer.size() < modelsk->meshes.size()) ? -1 : (int)i;
			// set to default if -1
			mat = (mat_index < 0) ?
				ResSpace::findDD_Material(m_resourceBin, (unsigned)0) :
				ResSpace::findDD_Material(m_resourceBin,
										  (unsigned)agent->mat_buffer[mat_index]);

			// render mesh's materials to gbuffer
			shader->setUniform("albedoFlag", (GLboolean)mat->m_albedo);
			if( mat->m_albedo ) {
				glActiveTexture(GL_TEXTURE0);
				shader->setUniform("tex_albedo", 0);
				GLuint texID = mat->m_textures[TextureType::ALBEDO]->handle;
				glBindTexture(GL_TEXTURE_2D, texID);
			}
			shader->setUniform("specFlag", (GLboolean)mat->m_specular);
			if( mat->m_specular ) {
				glActiveTexture(GL_TEXTURE1);
				shader->setUniform("tex_specular", 1);
				GLuint texID = mat->m_textures[TextureType::SPECULAR]->handle;
				glBindTexture(GL_TEXTURE_2D, texID);
			}
			shader->setUniform("normalFlag", (GLboolean)mat->m_normal);
			if( mat->m_normal ) {
				glActiveTexture(GL_TEXTURE2);
				shader->setUniform("tex_normal", 2);
				GLuint texID = mat->m_textures[TextureType::NORMAL]->handle;
				glBindTexture(GL_TEXTURE_2D, texID);
			}
			shader->setUniform("diffuse", glm::vec3(mat->m_base_color));
			shader->setUniform("shininess", mat->shininess);

			// find way to calculate normal matrix for all instances
			glm::mat4 norm = glm::transpose(glm::inverse(
				agent->parent_transform * agent->f_inst_m4x4[0]));
			shader->setUniform("Norm", norm);
			shader->setUniform("QuadRender", GLboolean(false));
			shader->setUniform("MVP",
				proj * view * agent->parent_transform);

			// bind instance buffer (dynamic b/c it's bound per frame)
			glBindBuffer(GL_ARRAY_BUFFER, modelsk->instVBO[i]);
			glBufferSubData(GL_ARRAY_BUFFER, 0,
							m_LODBufferSize[modelIndex] * sizeof(glm::mat4),
							&agent->f_inst_m4x4[0] + bufferOffet);
		}
		else {
			shader->setUniform(
				"MVP", agent->parent_transform);
			shader->setUniform("LightSpace", shadow->GetLSM());

			// bind instance buffer (dynamic b/c it's bound per frame)
			glBindBuffer(GL_ARRAY_BUFFER, modelsk->instVBO[i]);
			glBufferSubData(GL_ARRAY_BUFFER, 0,
							m_LODBufferSize[modelIndex] * sizeof(glm::mat4),
							&agent->inst_m4x4[0] + bufferOffet);
		}

		// color instance
		dd_array<glm::vec3> defaultColor(1);
		defaultColor[0] = glm::vec3(1.0f);
		glBindBuffer(GL_ARRAY_BUFFER, modelsk->instColorVBO[i]);
		if( agent->flag_color ) {
			shader->setUniform("multiplierMat", GLboolean(true));
			glBufferSubData(GL_ARRAY_BUFFER, 0,
							m_LODBufferSize[modelIndex] * sizeof(glm::vec3),
							&agent->f_inst_colors[0] + bufferOffet);
		}
		else {
			shader->setUniform("multiplierMat", GLboolean(false));
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3),
							&defaultColor[0]);
		}
		shader->setUniform("useDebug", false);

		MeshData& MD = modelsk->meshes[i];

		// log stats
		trisInFrame += MD.indices.size() / 3;
		drawCallsInFrame += 1;

		// bind mesh data
		glBindVertexArray(modelsk->VAO[i]);
		glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)MD.indices.size(), 
								GL_UNSIGNED_INT, 0, m_LODBufferSize[modelIndex]);

		// render bounding box if applicable

		//glDrawElements(GL_TRIANGLES, meshes[i].indices.size(), GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
	}
}

void DD_Renderer::LightPass(DD_Shader * shader,
							DD_Camera* cam,
							glm::mat4& view,
							glm::mat4& proj,
							VR_Eye eye)
{
	glm::mat4 identity, vrProj, vrView;
	switch( eye ) {
		case LEFT:
			vrView = CamSpace::GetViewMatrixVREye(
				cam,
				true,
				false,
				cam->cam_eye_dist
			);
			vrProj = CamSpace::GetOffAxisProjMatrix(
				cam,
				cam->vr_scr_pos,
				m_scrHorzDist, m_scrVertDist,
				cam->vr_eye_pos
			);
			break;
		case RIGHT:
			vrView = CamSpace::GetViewMatrixVREye(
				cam,
				false,
				true,
				cam->cam_eye_dist
			);
			vrProj = CamSpace::GetOffAxisProjMatrix(
				cam,
				cam->vr_scr_pos,
				m_scrHorzDist, m_scrVertDist,
				cam->vr_eye_pos
			);
			break;
		case DEFAULT:
			break;
		default:
			break;
	}

	glBindFramebuffer(GL_READ_FRAMEBUFFER, m_gbuffer.deferredFBO);
	// Write to light framebuffer
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_lbuffer.lightFBO);
	glBlitFramebuffer(0, 0, (GLint)m_Width, (GLint)m_Height, 0, 0, (GLint)m_Width, 
					  (GLint)m_Height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	// light pass using light framebuffer
	glDepthMask(GL_FALSE);
	glBindFramebuffer(GL_FRAMEBUFFER, m_lbuffer.lightFBO);
	glClear(GL_COLOR_BUFFER_BIT);

	// set uniforms and textures
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);

	shader->Use();
	shader->setUniform("viewPos", glm::vec3(cam->pos()));
	shader->setUniform("DrawSky", GLboolean(false));
	shader->setUniform("LightVolume", GLboolean(false));
	RendSpace::BindPassTexture(shader, &m_gbuffer);

	//bool insideVolume = false;
	for( size_t i = 0; i < m_resourceBin->light_counter; i++ ) {
		DD_Light* lghtInfo = ResSpace::findDD_Light(m_resourceBin, (unsigned)i);

		glm::vec3 lightPos = glm::vec3(
			lghtInfo->parent_transform * glm::vec4(lghtInfo->_position, 1.0f));
		shader->setUniform("Light.position", lightPos);
		shader->setUniform("Light.type", lghtInfo->m_type);
		shader->setUniform("Light.direction", lghtInfo->m_direction);
		shader->setUniform("Light.color", lghtInfo->m_color);
		shader->setUniform("Light.linear", lghtInfo->m_linear);
		shader->setUniform("Light.quadratic", lghtInfo->m_quadratic);
		shader->setUniform("Light.cutoff_i", lghtInfo->m_cutoff_i);
		shader->setUniform("Light.cutoff_o", lghtInfo->m_cutoff_o);
		shader->setUniform("Light.spotExponent", lghtInfo->m_spotExp);

		GLfloat lvRadius = LightSpace::CalculateLightVolumeRadius(lghtInfo);
		glm::mat4 model;
		model = glm::translate(model, lightPos);
		model = glm::scale(model, glm::vec3(lvRadius));
		glm::mat4 MVP;
		if( eye != VR_Eye::DEFAULT ) {
			MVP = vrProj * view * model;
		}
		else {
			MVP = proj * view * model;
		}

		if( lghtInfo->m_type == LightType::DIRECTION_L ) {
			// render shadows
			DD_Light* shadowL = GetDirectionalShadowLight(m_resourceBin);
			if( m_flagShadow && shadowL ) {
				RendSpace::BindPassTexture(shader, &m_gbuffer);
				//RendSpace::BindPassTexture(shader, &m_sbuffer, true);

				glActiveTexture(GL_TEXTURE3);
				glBindTexture(GL_TEXTURE_2D, m_fbuffer.depthTex);

				shader->setUniform("ShadowMap", true);
				shader->setUniform("LSM", shadowL->GetLSM());
			}

			// render quad
			RendSpace::RenderQuad(shader, identity);
			shader->setUniform("ShadowMap", false);

		}
		else if( lghtInfo->m_type == LightType::POINT_L ) {
			GLfloat camDist = glm::distance(
				glm::vec3(cam->pos()), lghtInfo->_position);

			if( camDist < lvRadius * 1.05f ) {
				// camera is inside volume (disable stencil buffer trick)
				glCullFace(GL_FRONT);
				glDepthFunc(GL_GREATER);

				// lighting pass
				this->RenderLightSphere(shader, MVP);

				// return state to default
				glCullFace(GL_BACK);
				glDepthFunc(GL_LESS);
			}
			else {
				// use stencil buffer pass to optimize culling when outside volume
				glEnable(GL_STENCIL_TEST);
				shader = &m_shaders[Shaders::LIGHT_STENCIL];
				shader->Use();

				glDisable(GL_CULL_FACE);
				glClear(GL_STENCIL_BUFFER_BIT);
				// stencil test to be enabled to succeed always. Only the depth test matters.
				glStencilFunc(GL_ALWAYS, 0, 0);
				// stencil trick to only render objects inside volume
				glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
				glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);

				// stencil pass
				this->RenderLightSphere(shader, MVP);

				// lighting pass
				glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
				glEnable(GL_CULL_FACE);
				shader = &m_shaders[Shaders::LIGHT];
				shader->Use();
				this->RenderLightSphere(shader, MVP);

				glDisable(GL_STENCIL_TEST);
			}
		}
		else {
			// spot light

		}
	}
	glDisable(GL_BLEND);
}

void DD_Renderer::RenderLightSphere(DD_Shader * shader, const glm::mat4 MVP)
{
	// bind mesh data
	shader->setUniform("MVP", MVP);
	shader->setUniform("LightVolume", GLboolean(true));
	shader->setUniform("screenDimension", glm::vec2(m_Width, m_Height));

	glBindVertexArray(m_volumeSphere->VAO[0]);
	glDrawElements(GL_TRIANGLES,
				   (GLint)m_volumeSphere->meshes[0].indices.size(),
				   GL_UNSIGNED_INT,
				   0);
	glBindVertexArray(0);

	shader->setUniform("LightVolume", GLboolean(false));
}

void DD_Renderer::CreateCubeMap(const char * ID)
{
	DD_Skybox* skybox = ResSpace::findDD_Skybox(m_resourceBin, ID);

	if( skybox ) {
		skybox->Generate();
		m_flagCubeMap = skybox->isActive();
		m_cubeMapHandle = skybox->handle;
	}
}

GLfloat DD_Renderer::ComputeLuminance()
{
	// set first
	DD_Shader* shader = &m_shaders[Shaders::LUMINANCE];
	shader->Use();
	shader->setUniform("computeLum", GLboolean(true));
	glBindImageTexture(0, m_lbuffer.colorTex, 0, GL_FALSE, 0,
					   GL_READ_ONLY, GL_RGBA16F);
	glBindImageTexture(1, m_luminOutput[0], 0, GL_FALSE, 0,
					   GL_WRITE_ONLY, GL_RGBA16F);
	glDispatchCompute(m_lumiTexSizes[0], m_lumiTexSizes[1], 1);
	// make sure writing to image has finished before reading from it
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	shader->setUniform("computeLum", GLboolean(false));

	// parallel reduction for loop
	int count = ((int)m_lumiTexSizes.size() / 2);
	GLint lastTex[2];
	for( int i = 1; i < count; ++i ) {
		glBindImageTexture(0, m_luminOutput[i - 1], 0, GL_FALSE, 0,
						   GL_READ_ONLY, GL_RGBA16F);
		glBindImageTexture(1, m_luminOutput[i], 0, GL_FALSE, 0,
						   GL_WRITE_ONLY, GL_RGBA16F);
		lastTex[0] = m_lumiTexSizes[2 * i];
		lastTex[1] = m_lumiTexSizes[2 * i + 1];
		glDispatchCompute(lastTex[0], lastTex[1], 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
	//printf("compute_W: %d, compute_H: %d\n", lastTex[0], lastTex[1]);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_luminOutput[count - 1]);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, &m_luminValues[0]);
	glm::vec3 avgLumCol = glm::vec3(0.0f);
	int numPixels = lastTex[0] * lastTex[1];
	for( int i = 0; i < numPixels; ++i ) {
		avgLumCol += glm::vec3(m_luminValues[4 * i], m_luminValues[4 * i + 1],
							   m_luminValues[4 * i + 2]);
	}
	avgLumCol /= numPixels;
	//printf("red: %.3f, green: %.3f, blue: %.3f\r", avgLumCol.r, avgLumCol.g, avgLumCol.b);
	glBindTexture(GL_TEXTURE_2D, 0);

	// avg equal natural log exponent calculated from shader
	return expf(avgLumCol.r / (numPixels));
	//return avgLumCol.r;
}

void SortSpace::QuickSort(DD_Agent * agent,
						  const int p,
						  const int r,
						  partition_func partfunc)
{
	if( p < r ) {
		const int q = (*partfunc)(agent, p, r);
		QuickSort(agent, p, q - 1, partfunc);
		QuickSort(agent, q + 1, r, partfunc);
	}
}

// Use randomized quick sort on instance array (pivot --> LOD flags)
int SortSpace::PartitionLOD(DD_Agent * agent, const int p, const int r)
{
	// randomize
	int randInd = (std::rand() % (r - p)) + p;
	int temp0 = agent->flag_cull_inst[randInd];
	glm::mat4 temp1 = agent->f_inst_m4x4[randInd];
	agent->flag_cull_inst[randInd] = agent->flag_cull_inst[r];
	agent->f_inst_m4x4[randInd] = agent->f_inst_m4x4[r];
	agent->flag_cull_inst[r] = temp0;
	agent->f_inst_m4x4[r] = temp1;
	if( agent->flag_color ) {
		glm::vec3 temp2 = agent->f_inst_colors[randInd];
		agent->f_inst_colors[randInd] = agent->f_inst_colors[r];
		agent->f_inst_colors[r] = temp2;
	}
	// in-place sort
	const int x = agent->flag_cull_inst[r];
	int i = p - 1;
	for( int j = p; j < r; j++ ) {
		if( agent->flag_cull_inst[j] <= x ) {
			i += 1;
			temp0 = agent->flag_cull_inst[i];
			temp1 = agent->f_inst_m4x4[i];
			agent->flag_cull_inst[i] = agent->flag_cull_inst[j];
			agent->f_inst_m4x4[i] = agent->f_inst_m4x4[j];
			agent->flag_cull_inst[j] = temp0;
			agent->f_inst_m4x4[j] = temp1;
			if( agent->flag_color ) {
				glm::vec3 temp2 = agent->f_inst_colors[i];
				agent->f_inst_colors[i] = agent->f_inst_colors[j];
				agent->f_inst_colors[j] = temp2;
			}
		}
	}
	temp0 = agent->flag_cull_inst[i + 1];
	temp1 = agent->f_inst_m4x4[i + 1];
	agent->flag_cull_inst[i + 1] = agent->flag_cull_inst[r];
	agent->f_inst_m4x4[i + 1] = agent->f_inst_m4x4[r];
	agent->flag_cull_inst[r] = temp0;
	agent->f_inst_m4x4[r] = temp1;
	if( agent->flag_color ) {
		glm::vec3 temp2 = agent->f_inst_colors[i + 1];
		agent->f_inst_colors[i + 1] = agent->f_inst_colors[r];
		agent->f_inst_colors[r] = temp2;
	}
	return i + 1;
}
