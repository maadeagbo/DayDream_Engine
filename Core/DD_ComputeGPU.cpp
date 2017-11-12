#include "DD_ComputeGPU.h"
#include "DD_Terminal.h"

namespace {
	const size_t char_buff_size = 512;
	char char_buff[char_buff_size];
	GLenum err;
}

/**
	Destructor unloads GPU resources
*/
DD_Compute::~DD_Compute()
{
	glDeleteBuffers(2, m_kinectDepthBuff);
}

/**
	Initializes any neccessary buffers from computing
	\todo Need to cannabalize this section and lock buffers to cumpute requests
*/
void DD_Compute::init()
{
	//*
	glGenBuffers(2, m_kinectDepthBuff);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_kinectDepthBuff[0]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * 512 * 424, NULL,
				 GL_DYNAMIC_DRAW);
	/*
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_kinectDepthBuff[1]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(glm::vec4) * 512 * 424,
			NULL, GL_DYNAMIC_DRAW);
	*/

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	//*/
}

/**

*/
DD_Event DD_Compute::Compute(DD_Event & event)
{
	if( event.m_type.compare("post") == 0 ) {
	}
	if( event.m_type == "compute_texA" ) {
		shaderDataA* _ct = static_cast<shaderDataA*>(event.m_message);

		// get texture (and compute dispatch dimensions
		DD_Texture2D* tex = ResSpace::findDD_Texture2D(
			res_ptr, _ct->texture_ID.c_str());
		DD_Shader* shader = ResSpace::findDD_Shader(
			res_ptr, _ct->shader_ID.c_str());
		int numX = 512 / (int)_ct->data_info[0];
		int numY = 424 / (int)_ct->data_info[1];

		bool second_buffer = false;
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_kinectDepthBuff[0]);
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, 
						0, 
			(GLsizeiptr)_ct->byte_size01,
						_ct->data_01);
		if( _ct->byte_size02 > 0 && _ct->data_02 ) { second_buffer = true; }

		second_buffer = false; // locked out debug buffer

		if( second_buffer ) {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_kinectDepthBuff[1]);
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 
							0, 
				(GLsizeiptr)_ct->byte_size02,
							_ct->data_02);
		}

		// check OpenGL error
		while( (err = glGetError()) != GL_NO_ERROR ) {
			snprintf(char_buff, char_buff_size,
					 "DD_ComputeGPU <0> :: OpenGL error: %d\n", err);
			DD_Terminal::post(char_buff);
		}

		shader->Use();
		// bind texture being sampled
		glBindImageTexture(0, tex->handle, 0, GL_FALSE, 0, GL_READ_ONLY,
						   GL_RGBA16F);
		if( _ct->texture2_ID != "" ) {
			DD_Texture2D* tex2 = ResSpace::findDD_Texture2D(
				res_ptr, _ct->texture2_ID.c_str());
			glBindImageTexture(2, tex2->handle, 0, GL_FALSE, 0, GL_READ_ONLY,
							   GL_RGBA16F);
		}
		// bind buffer being written to
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_kinectDepthBuff[0]);
		if( second_buffer ) {
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_kinectDepthBuff[1]);
		}
		// set extraneous uniforms
		shader->setUniform("nose_pos", _ct->uniform_01);

		glDispatchCompute(numX, numY, 1);
		// make sure writing to buffer has finished before reading from it
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		// save buffer to array
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_kinectDepthBuff[0]);
		glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 
						   0, 
						   (GLsizeiptr)_ct->byte_size01,
						   _ct->data_01);
		if( second_buffer ) {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_kinectDepthBuff[1]);
			glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 
							   0, 
							   (GLsizeiptr)_ct->byte_size02,
							   _ct->data_02);
		}

		// check OpenGL error
		while( (err = glGetError()) != GL_NO_ERROR ) {
			snprintf(char_buff, char_buff_size,
					 "DD_ComputeGPU <1> :: OpenGL error: %d\n", err);
			DD_Terminal::post(char_buff);
		}
	}
	return DD_Event();
}
