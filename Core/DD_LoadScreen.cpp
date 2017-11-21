#include "DD_LoadScreen.h"
#include "DD_Texture2D.h"
#include <SOIL.h>

namespace {
	GLuint loadCircleVAO = 0, loadCircleVBO = 0;
	glm::mat4 loadCircleMat, scaleMat, transMat;
	float timeFlag = 0.0f, scale_w_factor = 1.0, screenW, screenH,
		scale_all_factor = 0.10f;
	DD_Text arialTTF;
	DD_Texture2D load_tex = DD_Texture2D();
}

bool LoadScrSpace::LoadTextures(const int _screenW, const int _screenH)
{
	const std::string path = std::string(TEX_DIR) + "loading_base.png";

	load_tex.Generate(path.c_str());

	// set scale for circular quad
	screenW = (float)_screenW;
	screenH = (float)_screenH;
	scale_w_factor = screenH / screenW;
	scaleMat = glm::scale(scaleMat, glm::vec3(scale_w_factor, 1.0, 1.0));
	scaleMat = glm::scale(scaleMat, glm::vec3(scale_all_factor));
	transMat = glm::translate(glm::mat4(), glm::vec3(0.9f, -0.85f, 0.0f));

	// Load Fonts
	arialTTF = std::move(
		TextSpace::InitFontLib("arial", 0, 48, (float)_screenW, (float)_screenH));
	return true;
}

void LoadScrSpace::DrawLoadScreen(float timeElasped, DD_Shader* shader)
{
	glClearColor(0.f, 0.f, 0.f, 1.f); // make black background for load screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if( loadCircleVAO == 0 ) {
		GLfloat quadVertices[] = {
			// Positions        // Texture Coords
			-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
			-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
			1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
			1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
		};
		// Setup plane VAO
		glGenVertexArrays(1, &loadCircleVAO);
		glGenBuffers(1, &loadCircleVBO);
		glBindVertexArray(loadCircleVAO);
		glBindBuffer(GL_ARRAY_BUFFER, loadCircleVBO);
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
	bool moveDot = ((timeFlag + 0.1f) > timeElasped) ? false : true;
	if( moveDot ) {
		loadCircleMat = glm::rotate(loadCircleMat, glm::radians(-30.0f),
									glm::vec3(0.0, 0.0, 1.0));
		timeFlag = timeElasped;
	}
	// flip multiplication order b/c skew when Rot * Scale
	shader->setUniform("MVP", transMat * scaleMat * loadCircleMat);
	shader->setUniform("QuadRender", GLboolean(true));

	// shader uniforms
	glActiveTexture(GL_TEXTURE0);
	shader->setUniform("ColorTex", 0);
	glBindTexture(GL_TEXTURE_2D, load_tex.handle);
	// set post process color attributes
	shader->setUniform("DoToneMap", GLboolean(false));
	shader->setUniform("AveLum", 1.0f);
	shader->setUniform("Exposure", 0.75f); // control exposure
	shader->setUniform("White", 0.97f); // control white value

	glBindVertexArray(loadCircleVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

DD_Text LoadScrSpace::GetArialTTF()
{
	return arialTTF;
}
