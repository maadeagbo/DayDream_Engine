#include "DD_Texture2D.h"
#include "DD_Terminal.h"
#include <SOIL.h>

namespace {
	const size_t buff_size = 256;
	char char_buff[buff_size];
	GLenum err;
}

DD_Texture2D::~DD_Texture2D()
{
	if( handle != 0 ) {
		glDeleteTextures(1, &handle);
	}
}

bool DD_Texture2D::Generate(const char * full_path)
{
	path = full_path;
	unsigned char* image = SOIL_load_image(
		full_path, &(this->Width), &(this->Height), 0, SOIL_LOAD_RGBA
	);

	if( image == NULL ) {
		snprintf(char_buff, buff_size, "Could not open image: %s", full_path);
		DD_Terminal::post(char_buff);
		return false;
	}
	if( handle != 0 ) {
		glDeleteTextures(1, &handle);
	}
	glGenTextures(1, &handle);
	while( (err = glGetError()) != GL_NO_ERROR ) {
		snprintf(char_buff, buff_size, "DD_Texture2D <%s>:: OpenGL error: %d\n",
				 full_path, err);
		DD_Terminal::post(char_buff);
	}
	glBindTexture(GL_TEXTURE_2D, handle);

	int num_mipmaps = (int)floor(log2(std::max(Width, Height))) + 1;
	glTexStorage2D(GL_TEXTURE_2D, num_mipmaps, Internal_Format, Width, Height);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Width, Height, Image_Format,
					GL_UNSIGNED_BYTE, image);
	// texture settings
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, Wrap_S);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, Wrap_T);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, Filter_Min);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, Filter_Max);

	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	loaded_to_GPU = true;
	return true;
}

bool DD_Texture2D::Refill(unsigned char * data)
{
	if( data == NULL ) {
		snprintf(char_buff, buff_size, "Could not replace texture: %s",
				 path.c_str());
		DD_Terminal::post(char_buff);
		return false;
	}
	glBindTexture(GL_TEXTURE_2D, handle);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Width, Height, Image_Format,
					GL_UNSIGNED_BYTE, data);

	while( (err = glGetError()) != GL_NO_ERROR ) {
		snprintf(char_buff, buff_size, "DD_Texture2D <Refill>:: OpenGL error: %d\n",
				 err);
		DD_Terminal::post(char_buff);
	}

	glBindTexture(GL_TEXTURE_2D, 0);
	return true;
}

bool DD_Texture2D::Refill(const char * full_path)
{
	path = full_path;
	unsigned char* image = SOIL_load_image(
		full_path, &(this->Width), &(this->Height), 0, SOIL_LOAD_RGBA
	);

	if( image == NULL ) {
		snprintf(char_buff, buff_size, "Could not open image: %s", full_path);
		DD_Terminal::post(char_buff);
		return false;
	}

	glBindTexture(GL_TEXTURE_2D, handle);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Width, Height, Image_Format,
					GL_UNSIGNED_BYTE, image);

	while( (err = glGetError()) != GL_NO_ERROR ) {
		snprintf(char_buff, buff_size, "DD_Texture2D <Refill>:: OpenGL error: %d\n",
				 err);
		DD_Terminal::post(char_buff);
	}

	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);
	return true;
}

bool DD_Skybox::Generate()
{
	glGenTextures(1, &handle);
	glBindTexture(GL_TEXTURE_CUBE_MAP, handle);

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	GLuint targets[] = {
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};
	// create textures (get data from 1st texture)
	unsigned char* image = SOIL_load_image(
		(TEX_DIR + right).c_str(), &m_width, &m_height, 0, SOIL_LOAD_AUTO);
	SOIL_free_image_data(image);

	glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, m_width, m_height);
	for( int i = 0; i < 6; i++ ) {
		std::string file;
		switch( targets[i] ) {
			case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
				file = TEX_DIR + right;
				break;
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
				file = TEX_DIR + left;
				break;
			case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
				file = TEX_DIR + top;
				break;
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
				file = TEX_DIR + bottom;
				break;
			case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
				file = TEX_DIR + back;
				break;
			case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
				file = TEX_DIR + front;
				break;
			default:
				break;
		}
		image = SOIL_load_image(
			file.c_str(), &m_width, &m_height, 0, SOIL_LOAD_RGBA);
		if( image == nullptr ) {
			snprintf(char_buff, buff_size, "CubeMap <%s> creation failed.",
					 m_ID.c_str());
			DD_Terminal::post(char_buff);

			return activated;
		}
		glTexSubImage2D(targets[i], 0, 0, 0, m_width, m_height, GL_RGBA,
						GL_UNSIGNED_BYTE, image);
		SOIL_free_image_data(image);
	}

	while( (err = glGetError()) != GL_NO_ERROR ) {
		snprintf(char_buff, buff_size, "DD_Skybox <0>:: OpenGL error: %d\n", err);
		DD_Terminal::post(char_buff);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	activated = true;

	return activated;
}

void DD_Skybox::GenerateNull(const int w, const int h)
{
	m_width = w;
	m_height = h;

	glGenTextures(1, &handle);
	glBindTexture(GL_TEXTURE_CUBE_MAP, handle);

	while( (err = glGetError()) != GL_NO_ERROR ) {
		snprintf(char_buff, buff_size, "DD_Skybox <NULL_0>:: OpenGL error: %d\n",
				 err);
		DD_Terminal::post(char_buff);
	}

	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	// create textures
	glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, m_width, m_height);

	while( (err = glGetError()) != GL_NO_ERROR ) {
		snprintf(char_buff, buff_size, "DD_Skybox <NULL_1>:: OpenGL error: %d\n",
				 err);
		DD_Terminal::post(char_buff);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	activated = true;
}
