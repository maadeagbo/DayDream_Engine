#include "DD_ParticleEngine.h"
#include "DD_Terminal.h"

namespace {
	bool newCloth = false;
	const size_t char_buff_size = 256;
	char char_buff[char_buff_size];

	float time_accum = 1.f;//, f_test_buff[100000];
	glm::vec4 v4_test_buff[100000];
}

template<class T>
void GetBufferDataFromOpenGL(GLuint &handle, T* buffer, const size_t buffsize)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
	glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, buffsize, &buffer[0]);
}

template<class T>
void SendBufferDataToOpenGL(GLuint &handle, T* buffer, const size_t buffsize)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, handle);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, buffsize, &buffer[0]);
}

//  Load shader and emitter data types onto the GPU
void DD_ParticleSys::Load(const float Width, const float Height)
{
	m_Width = Width;
	m_Height = Height;

	// ************** Cloth shaders *****************
	m_crend_sh.init();
	m_crend_sh.CreateVertexShader(
		(std::string(SHADER_DIR) + "Cloth_V.vert").c_str());
	m_crend_sh.CreateFragShader(
		(std::string(SHADER_DIR) + "Cloth_F.frag").c_str());

	m_csimcp_sh.init();
	m_csimcp_sh.CreateComputeShader(
		(std::string(SHADER_DIR) + "Cloth_C.comp").c_str());
	m_normcp_sh.init();
	m_normcp_sh.CreateComputeShader(
		(std::string(SHADER_DIR) + "ClothNorm_C.comp").c_str());

	// ************** Particle shaders *****************
	m_prend_sh.init();
	m_prend_sh.CreateVertexShader(
		(std::string(SHADER_DIR) + "Particle_V.vert").c_str());
	m_prend_sh.CreateGeomShader(
		(std::string(SHADER_DIR) + "Particle_G.geom").c_str());
	m_prend_sh.CreateFragShader(
		(std::string(SHADER_DIR) + "Particle_F.frag").c_str());

	m_pinitcp_sh.init();
	m_pinitcp_sh.CreateComputeShader(
		(std::string(SHADER_DIR) + "InitEmitter_C.comp").c_str());
	m_psimcp_sh.init();
	m_psimcp_sh.CreateComputeShader(
		(std::string(SHADER_DIR) + "UpdateParticle_C.comp").c_str());

	// ************** Shallow Water shaders **************
	m_wcomp_sh.init();
	m_wcomp_sh.CreateComputeShader((
		std::string(SHADER_DIR) + "Water_C.comp").c_str());
	m_wsetzcp_sh.init();
	m_wsetzcp_sh.CreateComputeShader(
		(std::string(SHADER_DIR) + "WaterSetZ_C.comp").c_str());
	m_wsetxcp_sh.init();
	m_wsetxcp_sh.CreateComputeShader(
		(std::string(SHADER_DIR) + "WaterSetX_C.comp").c_str());
	m_wmidcp_sh.init();
	m_wmidcp_sh.CreateComputeShader(
		(std::string(SHADER_DIR) + "WaterMH_C.comp").c_str());

	m_wrend_sh.init();
	m_wrend_sh.CreateVertexShader(
		(std::string(SHADER_DIR) + "ShinyWater_V.vert").c_str());
	m_wrend_sh.CreateFragShader(
		(std::string(SHADER_DIR) + "ShinyWater_F.frag").c_str());

	GLenum err;
	while( (err = glGetError()) != GL_NO_ERROR ) {
		snprintf(char_buff, char_buff_size,
				 "DD_ParticleEngine <LOAD> :: OpenGL error: %d\n",
				 err);
		DD_Terminal::post(char_buff);
	}
}

// Get particle textures from resource bin (performed during loading screen)
bool DD_ParticleSys::Init()
{
	// create buffer for other jobs
	glGenVertexArrays(1, &m_jobs_VAO);
	glBindVertexArray(m_jobs_VAO);
	glGenBuffers(1, &m_jobs_VBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_jobs_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4) * 500000, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (GLvoid*)0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// check star texture
	DD_Texture2D* tex = ResSpace::getNewDD_Texture2D(m_resBin, "star");
	tex->path = std::string(TEX_DIR) + "star.png";
	m_texSetIndex[RenderTextureSets::STAR] = (int)m_resBin->tex_counter - 1;

	// check fire texture
	tex = ResSpace::getNewDD_Texture2D(m_resBin, "fire1");
	tex->path = std::string(TEX_DIR) + "fire03.png";
	m_texSetIndex[RenderTextureSets::FIRE01] = (int)m_resBin->tex_counter - 1;

	// check blue ball texture
	tex = ResSpace::getNewDD_Texture2D(m_resBin, "blueBall");
	tex->path = std::string(TEX_DIR) + "blueBall.png";
	m_texSetIndex[RenderTextureSets::B_BALL] = (int)m_resBin->tex_counter - 1;

	// check smoke texture
	tex = ResSpace::getNewDD_Texture2D(m_resBin, "smoke");
	tex->path = std::string(TEX_DIR) + "smoke01.png";
	m_texSetIndex[RenderTextureSets::SMOG] = (int)m_resBin->tex_counter - 1;

	// check fabric 01 texture
	tex = ResSpace::getNewDD_Texture2D(m_resBin, "fabric01");
	tex->path = std::string(TEX_DIR) + "fabric01.png";
	m_texSetIndex[RenderTextureSets::FABRIC_1] = (int)m_resBin->tex_counter - 1;

	return false; // finished loading
}

// handle posted events to Particle system
DD_Event DD_ParticleSys::Create(DD_Event & event)
{
	if( event.m_type == "generate_emitter" ) {
		if( m_resBin->emitter_counter >= 50 ) {
			printf("Emitter creation failed. Reached emitter limit...\n");
		}
		else {
			emitterInit* vars = static_cast<emitterInit*>(event.m_message);

			printf("Creating emitter ---> %s\n", vars->ID.c_str());

			DD_Emitter* em = ResSpace::getNewDD_Emitter(m_resBin,
														vars->ID.c_str());
			em->Initialize(vars->ID.c_str(),
						   vars->parentID.c_str(),
						   vars->radius,
						   vars->lifetime,
						   vars->size,
						   (int)vars->emitPerSec,
						   vars->type,
						   vars->model,
						   vars->deathRate,
						   vars->rotPerSec,
						   vars->direction,
						   vars->textureSet,
						   vars->velUp);

			// Generate
			LoadToGPU(em);

			bool set = SetEmitterParent(em, vars->parentID.c_str());

			if( set ) {
				printf("%s ---> %s (emitter parent)\n", vars->ID.c_str(),
					   vars->parentID.c_str());
			}
		}
	}
	else if( event.m_type == "create_cloth" ) {
		if( m_activeClothes >= 20 ) {
			printf("Cloth creation failed. Reached cloth limit...\n");
		}
		else {
			clothInit* vars = static_cast<clothInit*>(event.m_message);

			printf("Creating cloth ---> %s\n", vars->ID.c_str());

			m_clothes[m_activeClothes] = DD_Cloth();
			m_clothes[m_activeClothes].Initialize(vars->ID.c_str(),
												  (size_t)vars->rowSize,
												  (size_t)vars->colSize,
												  vars->pointDist,
												  vars->firstPoint,
												  vars->pinnedPoints);

			// Generate
			LoadToGPU(&m_clothes[m_activeClothes]);
			newCloth = true;

			m_activeClothes += 1;
		}
	}
	else if( event.m_type == "system_pause" ) {
		pause = !pause;
	}
	else if( event.m_type.compare("post") == 0 && newCloth ) {
		newCloth = false;
		DD_Event newEvent = DD_Event();

		return newEvent;
	}
	else if( event.m_type == "update_cloth" ) {
		clothUpdate* clUp = static_cast<clothUpdate*>(event.m_message);
		for( size_t i = 0; i < m_activeClothes; i++ ) {
			if( m_clothes[i].m_ID == clUp->ID ) {
				m_clothes[i].m_windSpeed = clUp->windSpeed;
			}
		}
		if( clUp->ball_radius > 0.f ) {
			ballColl.active = true;
			ballColl.mass = clUp->ball_mass;
			ballColl.pos = clUp->ball_pos;
			ballColl.radius = clUp->ball_radius;
			ballColl.vel = clUp->ball_velocity;
		}
		else {
			ballColl.active = false;
		}
	}
	return DD_Event();
}

int repeats = 0;

// called once per frame to iterate over alive emitters
bool DD_ParticleSys::Draw(const float dt,
						  glm::mat4 view,
						  glm::mat4 proj,
						  const glm::vec3 camP,
						  const GLuint particleFBO,
						  const GLuint gbufferFBO)
{
	bool particle_rendered = false;
	time_accum += dt;
	float currentTime = dt;
	if( pause ) {
		currentTime = 0.0f;
	}

	// set OpenGL attributes
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, particleFBO);

	// blend on for transparent emitters
	glBlendFunc(GL_ONE, GL_ONE);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);

	for( size_t i = 0; i < m_resBin->emitter_counter; i++ ) {
		particle_rendered = true;

		DD_Emitter* em = ResSpace::findDD_Emitter(m_resBin, (int)i);

		if( em->isChild() ) {
			DD_Agent* agent = m_resBin->agents[em->parentIndex()];
			if( agent ) {
				em->m_parentMat = agent->inst_m4x4[0];
			}
		}
		if( !em->isDead() ) {
			// update time
			GLfloat livedTime = em->updateLivedTime(currentTime);

			// check to generate new particles
			if( !pause ) {
				Generate(em, currentTime);
			}

			// update particles
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, em->m_posBuf);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, em->m_velBuf);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, em->m_colBuf);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, em->m_lifeBuf);

			// set emitter uniforms
			m_psimcp_sh.Use();
			m_psimcp_sh.setUniform("Halo", false);
			m_psimcp_sh.setUniform("Fire", false);
			m_psimcp_sh.setUniform("Water", false);
			m_psimcp_sh.setUniform("Magic", false);
			m_psimcp_sh.setUniform("CenterPos", em->pos());
			m_psimcp_sh.setUniform("deltaT", currentTime);
			m_psimcp_sh.setUniform("EmitterDT", livedTime);
			switch( em->type() ) {
				case EmitterType::HALO:
					m_psimcp_sh.setUniform("Halo", true);
					break;
				case EmitterType::FIRE:
					m_psimcp_sh.setUniform("Fire", true);
					break;
				case EmitterType::WATER:
					m_psimcp_sh.setUniform("Water", true);
					m_psimcp_sh.setUniform("InvMassPtcl", 1.0f / 5.f);
					break;
				case EmitterType::MAGIC:
					m_psimcp_sh.setUniform("Magic", true);
					m_psimcp_sh.setUniform("InvMassPtcl", 1.0f / 0.1f);
					break;
				default:
					break;
			}

			GLfloat temp = (float)em->numParticles() / 1000.f;
			GLuint numWG = (temp >= 1.f) ? (GLuint)temp : 1;
			if( !pause ) {
				glDispatchCompute(numWG, 1, 1);
				// make sure writing to image has finished before reading from it
				glMemoryBarrier(GL_ALL_BARRIER_BITS);
			}

			m_prend_sh.Use();
			m_prend_sh.setUniform("Proj", proj);
			m_prend_sh.setUniform("EmitterLife", em->deathRate());

			m_prend_sh.setUniform("MV", view * em->m_parentMat);
			m_prend_sh.setUniform("Size2", em->size());

			// stretch fire textures
			if( em->type() == EmitterType::FIRE ) {
				m_prend_sh.setUniform("fireFlag", true);
			}
			else {
				m_prend_sh.setUniform("fireFlag", false);
			}
			// set textures if avaliable
			if( em->texSetExists() ) {
				DD_Texture2D* tex01;
				m_prend_sh.setUniform("texFlag01", false);
				switch( em->tex() ) {
					case RenderTextureSets::STAR:
						tex01 = &m_resBin->textures[
							m_texSetIndex[RenderTextureSets::STAR]];
						m_prend_sh.setUniform("texFlag01", true);
						glActiveTexture(GL_TEXTURE0);
						m_prend_sh.setUniform("ParticleTex01", 0);
						glBindTexture(GL_TEXTURE_2D, tex01->handle);
						break;
					case RenderTextureSets::FIRE01:
						tex01 = &m_resBin->textures[
							m_texSetIndex[RenderTextureSets::FIRE01]];
						m_prend_sh.setUniform("texFlag01", true);
						glActiveTexture(GL_TEXTURE0);
						m_prend_sh.setUniform("ParticleTex01", 0);
						glBindTexture(GL_TEXTURE_2D, tex01->handle);

						tex01 = &m_resBin->textures[
							m_texSetIndex[RenderTextureSets::SMOG]];
						glActiveTexture(GL_TEXTURE1);
						m_prend_sh.setUniform("ParticleTex02", 1);
						glBindTexture(GL_TEXTURE_2D, tex01->handle);
						break;
					case RenderTextureSets::B_BALL:
						tex01 = &m_resBin->textures[
							m_texSetIndex[RenderTextureSets::B_BALL]];
						m_prend_sh.setUniform("texFlag01", true);
						glActiveTexture(GL_TEXTURE0);
						m_prend_sh.setUniform("ParticleTex01", 0);
						glBindTexture(GL_TEXTURE_2D, tex01->handle);
						break;
					case RenderTextureSets::SMOG:
						tex01 = &m_resBin->textures[
							m_texSetIndex[RenderTextureSets::SMOG]];
						m_prend_sh.setUniform("texFlag01", true);
						glActiveTexture(GL_TEXTURE0);
						m_prend_sh.setUniform("ParticleTex01", 0);
						glBindTexture(GL_TEXTURE_2D, tex01->handle);
						break;
					default:
						break;
				}
			}
			glBindVertexArray(em->m_VAO);
			glDrawArrays(GL_POINTS, 0, em->scratch_numCreated);
			glBindVertexArray(0);
		}
	}
	glDisable(GL_BLEND);

	// render particle jobs A
	for( size_t i = 0; i < m_numJobsA; i++ ) {
		particle_rendered = true;

		DD_Shader* shader = ResSpace::findDD_Shader(
			m_resBin, m_jobsA[i].shader_ID.c_str());
		DD_Texture2D* tex = ResSpace::findDD_Texture2D(
			m_resBin, m_jobsA[i].texture_ID.c_str());

		shader->Use();
		shader->setUniform("MV", view);
		shader->setUniform("Proj", proj);
		shader->setUniform("HalfWidth", 2.f);

		float angle = glm::radians(180.f);
		glm::mat4 rot = glm::rotate(glm::mat4(), angle, glm::vec3(0.f, 0.f, 1.f));
		shader->setUniform("upsideDown", rot);

		glBindBuffer(GL_ARRAY_BUFFER, m_jobs_VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)m_jobsA[i].byte_size01,
						m_jobsA[i].data_01);

		// load texture
		glActiveTexture(GL_TEXTURE0);
		shader->setUniform("Tex01", 0);
		glBindTexture(GL_TEXTURE_2D, tex->handle);

		// draw
		glBindVertexArray(m_jobs_VAO);
		glDrawArrays(GL_POINTS, 0, (GLsizei)m_jobsA[i].data_info[0]);
		glBindVertexArray(0);
	}
	m_numJobsA = 0;

	glDisable(GL_CULL_FACE); // render backface

	// render water
	for( size_t i = 0; i < m_resBin->water_counter; i++ ) {
		particle_rendered = true;

		DD_Water* _w = ResSpace::findDD_Water(m_resBin, (int)i);
		if( !_w->active ) { continue; }

		int numX = (int)(_w->m_rowS / 10.f);
		int numY = (int)(_w->m_colS / 10.f);

		const size_t numIterations = 1;
		float new_dt = currentTime / (float)numIterations;
		for( size_t j = 0; j < numIterations; j++ ) {
			_w->update(new_dt); // cpu update func
		}
		SendBufferDataToOpenGL<glm::vec4>(_w->m_posBuf[0], &_w->m_pos[0][0],
										  _w->m_pos.sizeInBytes());

		/*
		//load gpu
		SendBufferDataToOpenGL<glm::vec4>(_w->m_posBuf[0], _w->m_points);
		SendBufferDataToOpenGL<glm::vec4>(_w->m_posBuf[1], _w->m_midX);
		SendBufferDataToOpenGL<glm::vec4>(_w->m_posBuf[2], _w->m_midZ);
		SendBufferDataToOpenGL<glm::vec4>(_w->m_velBuf[0], _w->m_velocity);

		for (size_t i = 0; i  < numIterations && !_w->pause; i++) {
			// start testing 2d mid-height calculation
			m_wsetzcp_sh.Use();
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _w->m_posBuf[0]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _w->m_velBuf[0]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _w->m_posBuf[2]);
			m_wsetzcp_sh.setUniform("Gravity", -981.f);
			m_wsetzcp_sh.setUniform("deltaT", dt);
			m_wsetzcp_sh.setUniform("zDist", _w->m_yDist);

			glDispatchCompute(numX, numY, 1);
			// make sure writing to image has finished before reading from it
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			m_wsetxcp_sh.Use();
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _w->m_posBuf[0]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _w->m_velBuf[0]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _w->m_posBuf[1]);
			m_wsetxcp_sh.setUniform("Gravity", -981.f);
			m_wsetxcp_sh.setUniform("deltaT", dt);
			m_wsetxcp_sh.setUniform("xDist", _w->m_xDist);

			glDispatchCompute(numX, numY, 1);
			// make sure writing to image has finished before reading from it
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			m_wmidcp_sh.Use();
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _w->m_posBuf[1]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _w->m_posBuf[2]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, _w->m_posBuf[0]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, _w->m_velBuf[0]);
			m_wmidcp_sh.setUniform("Gravity", -981.f);
			m_wmidcp_sh.setUniform("deltaT", dt);
			m_wmidcp_sh.setUniform("xDist", _w->m_xDist);
			m_wmidcp_sh.setUniform("zDist", _w->m_yDist);
			m_wmidcp_sh.setUniform("dampConst", 0.001f);

			glDispatchCompute(numX, numY, 1);
			// make sure writing to image has finished before reading from it
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		}

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		for (size_t i = 0; i < 5; i++) {
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, 0);
		}

		// save buffer to array
		GetBufferDataFromOpenGL<glm::vec4>(_w->m_posBuf[1], _w->m_midX);
		GetBufferDataFromOpenGL<glm::vec4>(_w->m_posBuf[2], _w->m_midZ);
		GetBufferDataFromOpenGL<glm::vec4>(_w->m_posBuf[0], _w->m_points);
		GetBufferDataFromOpenGL<glm::vec4>(_w->m_velBuf[0], _w->m_velocity);

		check_iter += 1;

		if (check_iter == 100) {
			_w->m_points[(100 * 49) + 49].y += 200;
			_w->m_points[(100 * 49) + 50].y += 100;
			_w->m_points[(100 * 49) + 48].y += 100;
			_w->m_points[(100 * 48) + 49].y += 100;
			_w->m_points[(100 * 50) + 49].y += 100;
		}

		if (check_iter > 0) {
			// query final height calculation
			for (size_t i = 0; i < _w->m_midZ.size(); i++) {
				v4_test_buff[i] = _w->m_midZ[i];
			}
			for (size_t i = 0; i < _w->m_midX.size(); i++) {
				v4_test_buff[i] = _w->m_midX[i];
			}
			for (size_t i = 0; i < _w->m_points.size(); i++) {
				v4_test_buff[i] = _w->m_points[i];
				if (v4_test_buff[i].y != v4_test_buff[i].y) {
					v4_test_buff[i].y = 0.f;
				}
			}
			for (size_t i = 0; i < _w->m_velocity.size(); i++) {
				v4_test_buff[i] = _w->m_velocity[i];
			}
			DD_Terminal::post("Height vel center: " +
				std::to_string(v4_test_buff[(100 * 49) + 49].x) + ", " +
				std::to_string(v4_test_buff[(100 * 49) + 49].z));
		}
		//*/

		GLenum err;
		while( (err = glGetError()) != GL_NO_ERROR ) {
			snprintf(char_buff, char_buff_size,
					 "DD_ParticleEngine <0> :: OpenGL error: %d\n",
					 err);
			DD_Terminal::post(char_buff);
		}

		// calculate normals
		m_normcp_sh.Use();
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _w->m_posBuf[0]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, _w->m_normBuf);
		glDispatchCompute(numX, numY, 1);
		// make sure writing to image has finished before reading from it
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		//*
		m_wrend_sh.Use();
		m_wrend_sh.setUniform("MVP", proj * view);
		m_wrend_sh.setUniform("camPos", camP);
		m_wrend_sh.setUniform("translate", time_accum);
		m_wrend_sh.setUniform("underwater", camP.y > _w->m_waterHeight);

		DD_Skybox* sb01 = ResSpace::findDD_Skybox(m_resBin, _w->m_skyb01.c_str());
		DD_Texture2D* n_tex = ResSpace::findDD_Texture2D(
			m_resBin, _w->m_normTex.c_str());

		if( n_tex ) {
			m_wrend_sh.setUniform("fake_normals", GLboolean(true));
			glActiveTexture(GL_TEXTURE0);
			m_wrend_sh.setUniform("normTex", 0);
			glBindTexture(GL_TEXTURE_2D, n_tex->handle);
		}
		else { m_wrend_sh.setUniform("fake_normals", GLboolean(false)); }

		if( sb01 ) {
			if( !sb01->isActive() ) { sb01->Generate(); }
			glActiveTexture(GL_TEXTURE1);
			m_wrend_sh.setUniform("reflectBox", 1);
			glBindTexture(GL_TEXTURE_CUBE_MAP, sb01->handle);
		}
		//*/

		/*
		m_crend_sh.Use();
		m_crend_sh.setUniform("MVP", proj * view);
		DD_Texture2D* tex01 = findAsset_Name<DD_Texture2D*>(m_resBin, _w->m_normTex.c_str());
		glActiveTexture(GL_TEXTURE0);
		m_crend_sh.setUniform("ClothTex", 0);
		glBindTexture(GL_TEXTURE_2D, tex01->handle);
		//*/

		glBindVertexArray(_w->m_VAO);
		glDrawElements(GL_TRIANGLE_STRIP, 
					   (GLsizei)_w->m_indices.size(), 
					   GL_UNSIGNED_INT,
					   0);
		glBindVertexArray(0);
	}

	// render clothing
	for( size_t i = 0; i < m_activeClothes; i++ ) {
		particle_rendered = true;

		DD_Cloth* cl = &m_clothes[i];

		int numX = int(cl->m_rowS / 10.f);
		int numY = int(cl->m_colS / 10.f);

		// compute shader
		m_csimcp_sh.Use();
		m_csimcp_sh.setUniform("Gravity", -981.f);
		//m_csimcp_sh.setUniform("Gravity", -10.f);
		m_csimcp_sh.setUniform("InvMassPtcl", (float)(1.0 / 0.05));
		m_csimcp_sh.setUniform("SpringK", 20000.f);
		m_csimcp_sh.setUniform("horzRest", cl->m_horizDist);
		m_csimcp_sh.setUniform("vertRest", cl->m_vertDist);
		m_csimcp_sh.setUniform("diagRest", cl->m_diagDist);
		m_csimcp_sh.setUniform("DampingC", 5.1f);
		m_csimcp_sh.setUniform("aliveTime", cl->m_lifetime);
		m_csimcp_sh.setUniform("WindSpeed", cl->m_windSpeed);

		// ball interaction
		m_csimcp_sh.setUniform("ballFlag", ballColl.active);
		m_csimcp_sh.setUniform("ballPos", ballColl.pos);
		m_csimcp_sh.setUniform("ballVel", ballColl.vel);
		m_csimcp_sh.setUniform("ballMass", ballColl.mass);
		m_csimcp_sh.setUniform("ballRadius", ballColl.radius);

		const size_t numIterations = 1000;
		float new_dt = currentTime / (float)numIterations;
		m_csimcp_sh.setUniform("deltaT", new_dt);
		size_t currentBuf = 0;

		for( size_t j = 0; j < numIterations; j++ ) {
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0,
							 cl->m_posBuf[currentBuf]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1,
							 cl->m_posBuf[1 - currentBuf]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2,
							 cl->m_velBuf[currentBuf]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3,
							 cl->m_velBuf[1 - currentBuf]);
			glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4,
							 cl->m_oldBuf);

			//*
			//if (repeats <= 1000000) {
			glDispatchCompute(numX, numY, 1);
			// make sure writing to image has finished before reading from it
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			//repeats += 1;
		//}
		//*/

			currentBuf = (currentBuf + 1) % 2;
		}
		// calculate normals
		m_normcp_sh.Use();
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, cl->m_posBuf[0]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, cl->m_normBuf);
		glDispatchCompute(numX, numY, 1);
		// make sure writing to image has finished before reading from it
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		m_crend_sh.Use();
		m_crend_sh.setUniform("MVP", proj * view);

		DD_Texture2D* tex01 =
			&m_resBin->textures[m_texSetIndex[RenderTextureSets::FABRIC_1]];
		glActiveTexture(GL_TEXTURE0);
		m_crend_sh.setUniform("ClothTex", 0);
		glBindTexture(GL_TEXTURE_2D, tex01->handle);

		glBindVertexArray(cl->m_VAO);
		glDrawElements(GL_TRIANGLE_STRIP,
					   (GLsizei)cl->m_indices.size(), 
					   GL_UNSIGNED_INT,
					   0);
		glBindVertexArray(0);

		cl->m_lifetime += currentTime;
	}
	glEnable(GL_CULL_FACE);

	glDisable(GL_DEPTH_TEST);
	CleanEmitterBin(); // remove dead emitters
	return particle_rendered;
}

DD_Event DD_ParticleSys::AddJobToQueue(DD_Event & event)
{
	if( event.m_type.compare("particle_jobA") == 0 ) {
		if( m_numJobsA <= 5 ) {
			shaderDataA* _ct = static_cast<shaderDataA*>(event.m_message);

			m_jobsA[m_numJobsA] = *_ct;
			m_numJobsA += 1;
		}
	}
	return DD_Event();
}

// Remove dead emiiters from resources bin
void DD_ParticleSys::CleanEmitterBin()
{
	size_t num_em = m_resBin->emitter_counter;
	for( size_t i = 0; i < num_em; i++ ) {
		std::string name = m_resBin->emitters[i].m_ID;
		bool dead = m_resBin->emitters[i].isDead();
		if( dead ) {
			ResSpace::deleteEmitter(m_resBin, name.c_str());
		}
	}
}

// Called once per draw for each emitter (based on emitPerSec)
int DD_ParticleSys::Generate(DD_Emitter * em, const float dt)
{
	int newPtcls = 0;
	int remaining = em->numParticles() - em->scratch_numCreated;
	//printf("Remain: %d      \r", remaining);
	if( remaining > 0 ) {
		float timeAlive = em->updateLivedTime(0.f);
		newPtcls = (int)(timeAlive * (float)em->emitPerSec()) -
			(int)em->scratch_numCreated;
		em->scratch_numCreated += newPtcls;

		// initialize particles
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, em->m_posBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, em->m_velBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, em->m_colBuf);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, em->m_lifeBuf);
		m_pinitcp_sh.Use();
		m_pinitcp_sh.setUniform("lifeTime", em->deathRate());
		m_pinitcp_sh.setUniform("CenterPos", em->pos());
		m_pinitcp_sh.setUniform("createEndIndex",
			(GLint)em->scratch_numCreated);
		m_pinitcp_sh.setUniform("createStartIndex",
			(GLint)em->scratch_numCreated - newPtcls);

		glm::mat4 rot = em->updateRotMat();
		m_pinitcp_sh.setUniform("Halo", false);
		m_pinitcp_sh.setUniform("Fire", false);
		m_pinitcp_sh.setUniform("Water", false);
		m_pinitcp_sh.setUniform("Magic", false);
		switch( em->type() ) {
			case EmitterType::HALO:
				m_pinitcp_sh.setUniform("Halo", true);
				m_pinitcp_sh.setUniform("angle", 360.f / em->numParticles());
				m_pinitcp_sh.setUniform("radius", em->radius());
				break;
			case EmitterType::FIRE:
				m_pinitcp_sh.setUniform("Fire", true);
				m_pinitcp_sh.setUniform("angle", 360.f / em->numParticles());
				m_pinitcp_sh.setUniform("radius", em->radius());
				m_pinitcp_sh.setUniform("forwardF", em->m_forceUp);
				m_pinitcp_sh.setUniform("direction", rot * em->m_streamDirec);
				break;
			case EmitterType::WATER:
				m_pinitcp_sh.setUniform("Water", true);
				m_pinitcp_sh.setUniform("angle", 360.f / em->numParticles());
				m_pinitcp_sh.setUniform("radius", em->radius());
				m_pinitcp_sh.setUniform("deltaT", dt);
				m_pinitcp_sh.setUniform("forwardF", em->m_forceUp);
				m_pinitcp_sh.setUniform("direction", rot * em->m_streamDirec);
				break;
			case EmitterType::MAGIC:
				m_pinitcp_sh.setUniform("Magic", true);
				m_pinitcp_sh.setUniform("angle", 360.f / em->numParticles());
				m_pinitcp_sh.setUniform("radius", em->radius());
				m_pinitcp_sh.setUniform("forwardF", em->m_forceUp);
				m_pinitcp_sh.setUniform("direction", em->m_streamDirec);
				break;
			default:
				break;
		}

		float temp = em->numParticles() / 1000.f;
		int numWG = (temp >= 1.f) ? (GLuint)temp : 1;
		glDispatchCompute(numWG, 1, 1);
		// make sure writing to image has finished before reading from it
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}

	return newPtcls;
}

// Create buffers and VAO
void DD_ParticleSys::LoadToGPU(DD_Emitter * em)
{
	glGenBuffers(1, &em->m_posBuf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, em->m_posBuf);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		(GLuint)em->numParticles() * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &em->m_velBuf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, em->m_velBuf);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		(GLuint)em->numParticles() * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &em->m_colBuf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, em->m_colBuf);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		(GLuint)em->numParticles() * sizeof(glm::vec4), NULL, GL_DYNAMIC_DRAW);

	dd_array<float> initData(em->numParticles());
	glGenBuffers(1, &em->m_lifeBuf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, em->m_lifeBuf);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
		(GLuint)em->numParticles() * sizeof(float), &initData[0], GL_DYNAMIC_DRAW);

	// Create VAO for rendering
	glGenVertexArrays(1, &em->m_VAO);
	glBindVertexArray(em->m_VAO);

	glBindBuffer(GL_ARRAY_BUFFER, em->m_posBuf); // position
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
						  sizeof(glm::vec4), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, em->m_velBuf); // velocity
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
						  sizeof(glm::vec4), (GLvoid*)0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, em->m_colBuf); // color
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE,
						  sizeof(glm::vec4), (GLvoid*)0);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, em->m_lifeBuf); // life
	glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE,
						  sizeof(float), (GLvoid*)0);
	glEnableVertexAttribArray(3);

	glBindVertexArray(0);
}

// Create buffers and VAO
void DD_ParticleSys::LoadToGPU(DD_Cloth * cloth)
{
	glGenBuffers(1, &cloth->m_posBuf[0]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cloth->m_posBuf[0]);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
				 cloth->m_points.sizeInBytes(),
				 &cloth->m_points[0],
				 GL_DYNAMIC_DRAW);

	glGenBuffers(1, &cloth->m_posBuf[1]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cloth->m_posBuf[1]);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
				 cloth->m_points.sizeInBytes(), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &cloth->m_velBuf[0]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cloth->m_velBuf[0]);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
				 cloth->m_velocity.sizeInBytes(),
				 &cloth->m_velocity[0],
				 GL_DYNAMIC_DRAW);

	glGenBuffers(1, &cloth->m_velBuf[1]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cloth->m_velBuf[1]);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
				 cloth->m_velocity.sizeInBytes(), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &cloth->m_oldBuf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cloth->m_oldBuf);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
				 cloth->m_oldPos.sizeInBytes(),
				 &cloth->m_oldPos[0],
				 GL_DYNAMIC_DRAW);

	glGenBuffers(1, &cloth->m_normBuf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cloth->m_normBuf);
	glBufferData(GL_SHADER_STORAGE_BUFFER,
				 cloth->m_normal.sizeInBytes(),
				 &cloth->m_normal[0],
				 GL_DYNAMIC_DRAW);

	// Create VAO for rendering
	glGenVertexArrays(1, &cloth->m_VAO);
	glBindVertexArray(cloth->m_VAO);
	glGenBuffers(1, &cloth->m_EBO);
	glGenBuffers(1, &cloth->m_uvBuf);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cloth->m_EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				 cloth->m_indices.sizeInBytes(),
				 &cloth->m_indices[0],
				 GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, cloth->m_posBuf[0]);	// position[0]
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, cloth->m_normBuf);	// normals
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (GLvoid*)0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, cloth->m_uvBuf);		// uv
	glBufferData(GL_ARRAY_BUFFER,
				 cloth->m_uvs.sizeInBytes(), &cloth->m_uvs[0], GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (GLvoid*)0);
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);
}

bool DD_ParticleSys::SetEmitterParent(DD_Emitter * em, const char * pID)
{
	const size_t numAgents = m_resBin->m_num_agents;
	size_t index = 0;
	bool searching = true;
	std::string _ID = pID;
	while( searching && index < numAgents ) {
		std::string name = m_resBin->agents[index]->m_ID;
		if( name == _ID ) {
			searching = false;
		}
		index += 1;
	}
	if( searching ) {
		em->unParent(); // if name not found, remove parent
		return false;
	}
	em->SetParentIndex((int)index - 1); // name found, set index
	return true;
}
