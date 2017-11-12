#include "DD_ParticleTypes.h"
#include "DD_Terminal.h"

#define PRIM_RESTART 0xffffff

namespace {
	const size_t char_buff_size = 256;
	char char_buff[char_buff_size];

	glm::vec4 scratch[100][100];
	bool scratch_bool = false;
}

// Free up gpu memory
void DD_Emitter::Free()
{
	// m_VAO, m_posBuf, m_velBuf, m_colBuf, m_lifeBuf,
	glDeleteBuffers(1, &m_posBuf);
	glDeleteBuffers(1, &m_velBuf);
	glDeleteBuffers(1, &m_colBuf);
	glDeleteBuffers(1, &m_lifeBuf);
	glDeleteVertexArrays(1, &m_VAO);
}

void DD_Emitter::Initialize(const char* ID, 
							const char* parent, 
							const float radius,
							const float lifetime, 
							const float size, 
							const int _emitPerSec,
							const EmitterType type, 
							glm::mat4& placement, 
							const float deathR,
							const float rotationPS, 
							glm::mat4& _direc, 
							const RenderTextureSets ts,
							const float streamF)
{
	m_numPtcl = (int)((float)_emitPerSec * lifetime);
	m_ID = ID;
	_parent_id = parent;
	if( parent != std::string("") ) {
		_flag_parent = true;
	}
	m_radius = radius;
	_size = size;
	m_lifetime = lifetime;
	m_type = type;

	m_modelMat = placement;
	m_emitPerSec = _emitPerSec;
	m_deathRate = deathR;

	m_rotPerSec = rotationPS;
	m_rotMat = glm::rotate(glm::mat4(), glm::radians(m_rotPerSec),
						   glm::vec3(0.0, 1.0, 0.0));

	m_forceUp = streamF;
	m_streamDirec = _direc;

	registerTexSet(ts);
}

// kill emitter after last particle is dead
bool DD_Emitter::isDead()
{
	float lastPtcl = m_lifetime + m_deathRate;
	return (m_livedTime >= lastPtcl) ? true : false;
}

float myRand(const float a, const float b)
{
	return a + static_cast<float>(rand()) / 
		(static_cast<float>(RAND_MAX / (b - a)));
}

void DD_Cloth::Initialize(const char* ID, 
						  const size_t rowSize,
						  const size_t colSize, 
						  const float pointDist, 
						  const glm::vec3 firstPos,
						  dd_array<size_t> pinned)
{
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(PRIM_RESTART);

	m_ID = std::string(ID);
	m_rowS = (GLuint)rowSize;
	m_colS = (GLuint)colSize;
	m_horizDist = pointDist;
	m_vertDist = pointDist;
	m_diagDist = sqrtf(pointDist * pointDist + pointDist * pointDist);

	m_points.resize(rowSize * colSize);
	m_velocity.resize(rowSize * colSize);
	m_normal.resize(rowSize * colSize);

	m_uvs.resize(rowSize * colSize);
	m_oldPos.resize(rowSize * colSize);

	m_indices.resize((colSize * 2) * (rowSize - 1) + (rowSize - 1)); // triangle strip

	glm::vec3 xRot = glm::vec3(1.f, 0.f, 0.f);
	glm::mat4 transf = glm::translate(glm::mat4(), firstPos);
	transf = glm::rotate(transf, glm::radians(-80.f), xRot);

	// fill in points, velocities, generate UVs
	for( size_t i = 0; i < rowSize; i++ ) { // row
		for( size_t j = 0; j < colSize; j++ ) { // column
			const int index = (int)(i * colSize + j);
			m_points[index] = glm::vec4(0.f, 0.f, 0.f, 1.f) + glm::vec4(
				j * pointDist, i * -pointDist, 0.f, 1.f);
			m_points[index] = transf * m_points[index];
			m_oldPos[index] = m_points[index];

			m_velocity[index] = glm::vec4(0.0);
			m_uvs[index] = glm::vec2((float)j / (float)colSize, (float)i / 
				(float)rowSize);
		}
	}
	// generate EBO
	size_t currIndex = 0;
	for( size_t i = 0; i < rowSize - 1; i++ ) {
		for( size_t j = 0; j < colSize; j++ ) {
			m_indices[currIndex] = (GLuint)((i + 1) * colSize + j); 
			currIndex += 1;
			m_indices[currIndex] = (GLuint)((i)* colSize + j);
			currIndex += 1;
		}
		m_indices[currIndex] = PRIM_RESTART; currIndex += 1;
	}
}

void DD_Cloth::Free()
{
	// m_VAO, m_posBuf, m_velBuf, m_colBuf, m_lifeBuf,
	glDeleteBuffers(1, &m_posBuf[0]);
	glDeleteBuffers(1, &m_posBuf[1]);
	glDeleteBuffers(1, &m_velBuf[0]);
	glDeleteBuffers(1, &m_velBuf[1]);
	glDeleteBuffers(1, &m_EBO);
	glDeleteBuffers(1, &m_uvBuf);
	glDeleteBuffers(1, &m_oldBuf);
	glDeleteBuffers(1, &m_normBuf);
	glDeleteVertexArrays(1, &m_VAO);
}

DD_Water::~DD_Water()
{
	if( active ) {
		glDeleteBuffers(3, m_posBuf);
		glDeleteBuffers(3, m_velBuf);
		glDeleteBuffers(1, &m_EBO);
		glDeleteBuffers(1, &m_uvBuf);
		glDeleteBuffers(1, &m_normBuf);
		glDeleteVertexArrays(1, &m_VAO);
	}
}

void DD_Water::initialize(const size_t rowSize, 
						  const size_t colSize,
						  const float x_pointDist, 
						  const float y_pointDist, 
						  const glm::vec3 firstPos,
						  const size_t tex_repeat)
{
	glEnable(GL_PRIMITIVE_RESTART);
	glPrimitiveRestartIndex(PRIM_RESTART);
	active = true;

	m_rowS = (GLuint)rowSize;
	m_colS = (GLuint)colSize;
	m_xDist = x_pointDist;
	m_yDist = y_pointDist;
	m_waterHeight = firstPos.y;

	m_pos.resize(rowSize, colSize);
	m_vel.resize(rowSize, colSize);
	m_normal.resize(rowSize, colSize);
	m_uvs.resize(rowSize * colSize);
	m_xhalf.resize(rowSize, colSize);
	m_zhalf.resize(rowSize, colSize);

	m_indices.resize((colSize * 2) * (rowSize - 1) + (rowSize - 1)); // tri strip

	glm::mat4 transf = glm::translate(glm::mat4(), firstPos);
	m_transpose_mat = glm::inverse(transf);

	// fill in points, velocities, generate UVs
	for( size_t i = 0; i < rowSize; i++ ) { // row
		for( size_t j = 0; j < colSize; j++ ) { // column
			const int index = (int)(i * colSize + j);
			glm::vec4 _p = glm::vec4(j * m_xDist, 0.f, i * m_yDist, 1.f);
			_p = transf * _p;
			m_pos[i][j] = _p;
			m_vel[i][j] = glm::vec2(0.f);

			m_uvs[index] = glm::vec2((float)j / (float)colSize,
				(float)i / (float)rowSize);
			m_uvs[index] *= (float)tex_repeat;

			scratch[i][j].z = _p.y;
		}
	}
	// raise verts
	/*if( raised_verts ) {
		dd_array<glm::uvec2> bin = *raised_verts;
		for( size_t i = 0; i < bin.size(); i++ ) {
			m_pos[bin[i].x][bin[i].y].y += 300.f;
		}
	}*/
	// generate EBO
	size_t currIndex = 0;
	for( size_t i = 0; i < rowSize - 1; i++ ) {
		for( size_t j = 0; j < colSize; j++ ) {
			m_indices[currIndex] = (GLuint)((i)* colSize + j);
			currIndex += 1;
			m_indices[currIndex] = (GLuint)((i + 1) * colSize + j);
			currIndex += 1;
		}
		m_indices[currIndex] = PRIM_RESTART; currIndex += 1;
	}

	// bind buffers
	glGenBuffers(3, m_posBuf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_posBuf[0]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, m_pos.sizeInBytes(),
				 &m_pos[0][0], GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_posBuf[1]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, m_pos.sizeInBytes(),
				 &m_pos[0][0], GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_posBuf[2]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, m_pos.sizeInBytes(),
				 &m_pos[0][0], GL_DYNAMIC_DRAW);
	glGenBuffers(3, m_velBuf);
	for( size_t i = 0; i < 3; i++ ) {
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_velBuf[i]);
		glBufferData(GL_SHADER_STORAGE_BUFFER, m_vel.sizeInBytes(),
					 &m_pos[0][0], GL_DYNAMIC_DRAW);
	}
	glGenBuffers(1, &m_normBuf);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_normBuf);
	glBufferData(GL_SHADER_STORAGE_BUFFER, m_normal.sizeInBytes(), NULL,
				 GL_DYNAMIC_DRAW);

	// Create VAO for rendering
	glGenVertexArrays(1, &m_VAO);
	glBindVertexArray(m_VAO);
	glGenBuffers(1, &m_EBO);
	glGenBuffers(1, &m_uvBuf);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.sizeInBytes(),
				 &m_indices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_posBuf[0]);	// position[0]
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4),
		(GLvoid*)0);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, m_normBuf);	// normals
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4),
		(GLvoid*)0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, m_uvBuf);		// uv
	glBufferData(GL_ARRAY_BUFFER, m_uvs.sizeInBytes(), &m_uvs[0],
				 GL_STATIC_DRAW);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2),
		(GLvoid*)0);
	glEnableVertexAttribArray(2);

	GLenum err;
	while( (err = glGetError()) != GL_NO_ERROR ) {
		snprintf(char_buff, char_buff_size, "DD_Water <%s> :: OpenGL error: %d\n",
				 m_ID.c_str(), err);
		DD_Terminal::post(char_buff);
	}

	glBindVertexArray(0);
}

float sq_f(const float val) { return val * val; }

void DD_Water::update(const float dt)
{
	const float hgrav = 9.81f * 0.5;
	// calculate half step for x
#pragma omp parallel for
	for( int i = 0; i < (int)m_rowS - 1; i++ ) {
		for( int j = 0; j < (int)m_colS; j++ ) {
			const float vx_00 = m_vel[i][j].x;
			const float vx_01 = m_vel[i + 1][j].x;
			const float h_00 = m_pos[i][j].y;
			const float h_01 = m_pos[i + 1][j].y;
			const float vz_00 = m_vel[i][j].y;
			const float vz_01 = m_vel[i + 1][j].y;

			// main momentum in x
			m_xhalf[i][j].x = (vx_00 + vx_01) * 0.5f - (dt * 0.5f) * (
				sq_f(vx_01) / h_01 + hgrav * sq_f(h_01) -
				sq_f(vx_00) / h_00 - hgrav * sq_f(h_00)) / m_xDist;
			// height in y
			m_xhalf[i][j].y = (h_00 + h_01) * 0.5f - (dt * 0.5f) *
				(vx_01 - vx_00) / m_xDist;
			// momentum in z
			m_xhalf[i][j].z = (vz_00 + vz_01) * 0.5f - (dt * 0.5f) *
				((vx_01 * vz_01) / h_01 - (vx_00 * vz_00) / h_00) / m_xDist;

			scratch[i][j] = glm::vec4(m_xhalf[i][j], 1.0f);
		}
	}
	// calculate half step for y
#pragma omp parallel for
	for( int i = 0; i < (int)m_rowS; i++ ) {
		for( int j = 0; j < (int)m_colS - 1; j++ ) {
			const float vz_00 = m_vel[i][j].y;
			const float vz_01 = m_vel[i][j + 1].y;
			const float h_00 = m_pos[i][j].y;
			const float h_01 = m_pos[i][j + 1].y;
			const float vx_00 = m_vel[i][j].x;
			const float vx_01 = m_vel[i][j + 1].x;

			// main momentum in z
			m_zhalf[i][j].z = (vz_00 + vz_01) * 0.5f - (dt * 0.5f) * (
				sq_f(vz_01) / h_01 + hgrav * sq_f(h_01) -
				sq_f(vz_00) / h_00 - hgrav * sq_f(h_00)) / m_yDist;
			// height in y
			m_zhalf[i][j].y = (h_00 + h_01) * 0.5f - (dt * 0.5f) *
				(vz_01 + vz_00) / m_yDist;
			// momentum in x
			m_zhalf[i][j].x = (vx_01 + vx_00) * 0.5f - (dt * 0.5f) *
				((vz_01 * vx_01) / h_01 - (vz_00 * vx_00) / h_00) / m_yDist;

			scratch[i][j] = glm::vec4(m_zhalf[i][j], 1.0f);
		}
	}

	const float dampc = 0.2f;
	const float dt_x = dt / m_xDist;
	const float dt_y = dt / m_yDist;

	// calculate complete
//#pragma omp parallel for
//	for (int i = 1; i < (int)m_rowS - 1; i++) {
//		for (int j = 1; j < (int)m_colS - 1; j++) {
//			const glm::vec3 u_00 = m_xhalf[i][j-1];
//			const glm::vec3 u_01 = m_xhalf[i - 1][j-1];
//			const glm::vec3 v_00 = m_zhalf[i-1][j];
//			const glm::vec3 v_01 = m_zhalf[i-1][j - 1];
//
//			// height
//			m_pos[i][j].y -= dt_x * (u_00.x - u_01.x) -
//							 dt_y * (v_00.z - v_01.z);
//			// x momentum
//			m_vel[i][j].x -= dt_x * (
//									(sq_f(u_00.x)/u_00.y + hgrav * sq_f(u_00.y)) -
//									(sq_f(u_01.x)/u_01.y + hgrav * sq_f(u_01.y)))
//						   - dt_y * (((v_00.z * v_00.x)/v_00.y) -
//									((v_01.z * v_01.x) / v_01.y));
//			// z momentum
//			m_vel[i][j].y -= dt_y * (
//									(sq_f(v_00.z)/v_00.y + hgrav * sq_f(v_00.y)) -
//									(sq_f(v_01.z)/v_01.y + hgrav * sq_f(v_01.y)))
//						   - dt_x * (((u_00.x * u_00.z) / u_00.y) -
//									((u_01.x * u_01.z) / u_01.y));
//		}
//	}

	// calculate complete for x
#pragma omp parallel for
	for( int i = 1; i < (int)m_rowS - 1; i++ ) {
		for( int j = 0; j < (int)m_colS; j++ ) {
			// height
			m_pos[i][j].y -= dt_x * (m_xhalf[i][j].x - m_xhalf[i - 1][j].x);
			// velocity in x
			m_vel[i][j].x -= dt_x * (dampc * m_vel[i][j].x +
									 sq_f(m_xhalf[i][j].x) / m_xhalf[i][j].y + hgrav *
									 sq_f(m_xhalf[i][j].y) -
									 sq_f(m_xhalf[i - 1][j].x) / m_xhalf[i - 1][j].y - hgrav *
									 sq_f(m_xhalf[i - 1][j].y)
									 );
		}
	}

	//	// calculate complete for y
	//#pragma omp parallel for
	//	for (int i = 0; i < (int)m_rowS; i++) {
	//		for (int j = 1; j < (int)m_colS - 1; j++) {
	//			// height
	//			m_pos[i][j].y -= dt_y * (m_zhalf[i][j].z - m_zhalf[i][j - 1].z);
	//			// velocity
	//			m_vel[i][j].y -= dt_y * (dampc * m_vel[i][j].y +
	//				sq_f(m_zhalf[i][j].z) / m_zhalf[i][j].y + hgrav *
	//				sq_f(m_zhalf[i][j].y) -
	//				sq_f(m_zhalf[i][j - 1].z) / m_zhalf[i][j - 1].y - hgrav *
	//				sq_f(m_zhalf[i][j - 1].y)
	//				);
	//
	//			scratch[i][j].x = m_vel[i][j].x;
	//			scratch[i][j].y = m_pos[i][j].y;
	//		}
	//	}

		// edge cases
	for( size_t j = 0; j < m_colS; j++ ) {
		m_pos[0][j].y = m_pos[m_rowS - 2][j].y;
		m_vel[0][j].x = m_vel[m_rowS - 2][j].x;
		m_pos[m_rowS - 1][j].y = m_pos[1][j].y;
		m_vel[m_rowS - 1][j].x = m_vel[1][j].x;
	}
	/*for (size_t i = 0; i < m_rowS; i++) {
		m_pos[i][0].y = m_pos[i][1].y;
		m_vel[i][0].y = m_vel[i][1].y;
		m_pos[i][m_colS - 1].y = m_pos[i][m_colS - 2].y;
		m_vel[i][m_colS - 1].y = m_vel[i][m_colS - 2].y;
	}*/

	scratch_bool = false;
}

void DD_Water::addDrop(const glm::vec2 pos_xz, const float radius,
					   const float height)
{
	scratch_bool = true;

	DD_Terminal::post("--> (" + std::to_string(pos_xz.x) + ", " +
					  std::to_string(pos_xz.y) + ")");
#pragma omp parallel for
	for( int i = 0; i < (int)m_rowS; i++ ) {
		for( int j = 0; j < (int)m_colS; j++ ) {
			glm::vec4 point_v4 = m_transpose_mat *
				glm::vec4(i * m_xDist, j * m_yDist, 0.0, 0.0);
			const glm::vec2 point = glm::vec2(point_v4);
			const float dist = glm::distance(point, pos_xz);

			if( dist < radius ) { m_pos[i][j].y += height * (1 - dist / radius); }
		}
	}
}
