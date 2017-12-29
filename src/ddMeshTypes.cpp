#include "ddMeshTypes.h"

/// \brief Set the 8 corners of bounding box
void BoundingBox::SetCorners() {
  if (min.x == max.x) {
    min.x -= 1.f;
    max.x += 1.f;
  }
  if (min.y == max.y) {
    min.y -= 1.f;
    max.y += 1.f;
  }
  if (min.z == max.z) {
    min.z -= 1.f;
    max.z += 1.f;
  }
  this->corner1 =
      glm::vec3(this->min.x, this->min.y, this->min.z);  // back bottom left
  this->corner2 =
      glm::vec3(this->max.x, this->min.y, this->min.z);  // back bottom right
  this->corner3 =
      glm::vec3(this->max.x, this->max.y, this->min.z);  // back top right
  this->corner4 =
      glm::vec3(this->min.x, this->max.y, this->min.z);  // back top left
  this->corner5 =
      glm::vec3(this->min.x, this->min.y, this->max.z);  // front bottom left
  this->corner6 =
      glm::vec3(this->max.x, this->min.y, this->max.z);  // front bottom right
  this->corner7 =
      glm::vec3(this->max.x, this->max.y, this->max.z);  // front top right
  this->corner8 =
      glm::vec3(this->min.x, this->max.y, this->max.z);  // front top left
}

/// \brief Apply transformation to bounding box (translate/rotate/scale)
BoundingBox BoundingBox::transformCorners(const glm::mat4 transMat) {
  BoundingBox bbox;

  bbox.corner1 = glm::vec3(transMat * glm::vec4(this->corner1, 1.0f));
  bbox.corner2 = glm::vec3(transMat * glm::vec4(this->corner2, 1.0f));
  bbox.corner3 = glm::vec3(transMat * glm::vec4(this->corner3, 1.0f));
  bbox.corner4 = glm::vec3(transMat * glm::vec4(this->corner4, 1.0f));
  bbox.corner5 = glm::vec3(transMat * glm::vec4(this->corner5, 1.0f));
  bbox.corner6 = glm::vec3(transMat * glm::vec4(this->corner6, 1.0f));
  bbox.corner7 = glm::vec3(transMat * glm::vec4(this->corner7, 1.0f));
  bbox.corner8 = glm::vec3(transMat * glm::vec4(this->corner8, 1.0f));

  bbox.UpdateAABB_min();
  bbox.UpdateAABB_max();

  return bbox;
}

/// \brief Update and return min AABB point
glm::vec3 BoundingBox::UpdateAABB_min() {
  this->min = this->corner1;
  glm::vec3 points[] = {this->corner2, this->corner3, this->corner4,
                        this->corner5, this->corner6, this->corner7,
                        this->corner8};

  for (int i = 0; i < 7; ++i) {
    this->min.x = (points[i].x < this->min.x) ? points[i].x : this->min.x;
    this->min.y = (points[i].y < this->min.y) ? points[i].y : this->min.y;
    this->min.z = (points[i].z < this->min.z) ? points[i].z : this->min.z;
  }
  return this->min;
}

/// \brief Update and return max AABB point
glm::vec3 BoundingBox::UpdateAABB_max() {
  this->max = this->corner1;
  glm::vec3 points[] = {this->corner2, this->corner3, this->corner4,
                        this->corner5, this->corner6, this->corner7,
                        this->corner8};

  for (int i = 0; i < 7; ++i) {
    this->max.x = (points[i].x > this->max.x) ? points[i].x : this->max.x;
    this->max.y = (points[i].y > this->max.y) ? points[i].y : this->max.y;
    this->max.z = (points[i].z > this->max.z) ? points[i].z : this->max.z;
  }
  return this->max;
}

/// \brief Get min based on frustum plane
glm::vec3 BoundingBox::GetFrustumPlaneMin(glm::vec3 norm) {
  glm::vec3 newMin = this->max;
  if (norm.x >= 0) {
    newMin.x = this->min.x;
  }
  if (norm.y >= 0) {
    newMin.y = this->min.y;
  }
  if (norm.z >= 0) {
    newMin.z = this->min.z;
  }
  return newMin;
}

/// \brief Get max based on frustum plane
glm::vec3 BoundingBox::GetFrustumPlaneMax(glm::vec3 norm) {
  glm::vec3 newMax = this->min;
  if (norm.x >= 0) {
    newMax.x = this->max.x;
  }
  if (norm.y >= 0) {
    newMax.y = this->max.y;
  }
  if (norm.z >= 0) {
    newMax.z = this->max.z;
  }
  return newMax;
}

/// \brief Create buffer for line render
void BoundingBox::SetLineBuffer() {
  //                                    4           3
  // back bottom left		corner 1        . . . . . . .
  // back bottom right	corner 2        . . 8       . . 7
  // back top right		corner 3        .   . . . . . . .
  // back top left		corner 4        .   .       .   .
  // front bottom left	corner 5        .   .     2 .   .
  // front bottom right	corner 6      1 . . . . . . .   .
  // front top right		corner 7          . .         . .
  // front top left		corner 8          5 . . . . . . . 6

  buffer[0] = glm::vec4(corner1, 1.f);
  buffer[1] = glm::vec4(corner2, 1.f);
  buffer[2] = glm::vec4(corner1, 1.f);
  buffer[3] = glm::vec4(corner5, 1.f);
  buffer[4] = glm::vec4(corner2, 1.f);
  buffer[5] = glm::vec4(corner6, 1.f);
  buffer[6] = glm::vec4(corner5, 1.f);
  buffer[7] = glm::vec4(corner6, 1.f);

  buffer[8] = glm::vec4(corner4, 1.f);
  buffer[9] = glm::vec4(corner3, 1.f);
  buffer[10] = glm::vec4(corner4, 1.f);
  buffer[11] = glm::vec4(corner8, 1.f);
  buffer[12] = glm::vec4(corner7, 1.f);
  buffer[13] = glm::vec4(corner8, 1.f);
  buffer[14] = glm::vec4(corner7, 1.f);
  buffer[15] = glm::vec4(corner3, 1.f);

  buffer[16] = glm::vec4(corner3, 1.f);
  buffer[17] = glm::vec4(corner2, 1.f);
  buffer[18] = glm::vec4(corner7, 1.f);
  buffer[19] = glm::vec4(corner6, 1.f);
  buffer[20] = glm::vec4(corner8, 1.f);
  buffer[21] = glm::vec4(corner5, 1.f);
  buffer[22] = glm::vec4(corner4, 1.f);
  buffer[23] = glm::vec4(corner1, 1.f);
}

glm::vec4 getVec4f(const char* str) {
  glm::vec4 v4;
  char* nxt;
  v4[0] = strtof(str, &nxt);
  nxt++;
  v4[1] = strtof(nxt, &nxt);
  nxt++;
  v4[2] = strtof(nxt, &nxt);
  nxt++;
  v4[3] = strtof(nxt, nullptr);
  return v4;
}

glm::uvec4 getVec4u(const char* str) {
  glm::uvec4 v4;
  char* nxt;
  v4[0] = strtoul(str, &nxt, 10);
  nxt++;
  v4[1] = strtoul(nxt, &nxt, 10);
  nxt++;
  v4[2] = strtoul(nxt, &nxt, 10);
  nxt++;
  v4[3] = strtoul(nxt, nullptr, 10);
  return v4;
};

glm::vec3 getVec3f(const char* str) {
  glm::vec3 v3;
  char* nxt;
  v3[0] = strtof(str, &nxt);
  v3[1] = strtof(nxt, &nxt);
  v3[2] = strtof(nxt, nullptr);
  return v3;
};

glm::vec2 getVec2f(const char* str) {
  glm::vec2 v2;
  char* nxt;
  v2[0] = strtof(str, &nxt);
  nxt++;
  v2[1] = strtof(nxt, nullptr);
  return v2;
}

/// \brief Translate 3-component vector to quaternion
glm::quat getQuat(const char* str) {
  char* nxt;
  float x, y, z;
  x = strtof(str, &nxt);
  nxt++;
  y = strtof(nxt, &nxt);
  nxt++;
  z = strtof(nxt, nullptr);
  glm::quat qx, qy, qz;
  qx = glm::rotate(glm::quat(), glm::radians(x), glm::vec3(1.f, 0.f, 0.f));
  qy = glm::rotate(glm::quat(), glm::radians(y), glm::vec3(0.f, 1.f, 0.f));
  qz = glm::rotate(glm::quat(), glm::radians(z), glm::vec3(0.f, 0.f, 1.f));
  return qz * qy * qx;
}

std::string Vec4f_Str(const glm::vec4 vIn) {
  char buff[32];
  snprintf(buff, sizeof(buff), "%.3f %.3f %.3f %.3f\n", vIn.x, vIn.y, vIn.z,
           vIn.w);
  return std::string(buff);
}

glm::mat4 createMatrix(const glm::vec3& pos, const glm::vec3& rot,
                       const glm::vec3& scale) {
  glm::mat4 mt = glm::translate(glm::mat4(), pos);
  glm::mat4 mrx =
      glm::rotate(glm::mat4(), glm::radians(rot.x), glm::vec3(1.f, 0.f, 0.f));
  glm::mat4 mry =
      glm::rotate(glm::mat4(), glm::radians(rot.y), glm::vec3(0.f, 1.f, 0.f));
  glm::mat4 mrz =
      glm::rotate(glm::mat4(), glm::radians(rot.z), glm::vec3(0.f, 0.f, 1.f));
  glm::mat4 ms = glm::scale(glm::mat4(), glm::vec3(scale));
  return mt * mrz * mry * mrx * ms;
}

void printGlmMat(glm::mat4 mat) {
  glm::vec4 temp_vec;
  std::string line;

  temp_vec = glm::vec4(mat[0][0], mat[1][0], mat[2][0], mat[3][0]);
  line = Vec4f_Str(temp_vec);
  // ddTerminal::post(line);
  printf("%s\n", line.c_str());

  temp_vec = glm::vec4(mat[0][1], mat[1][1], mat[2][1], mat[3][1]);
  line = Vec4f_Str(temp_vec);
  // ddTerminal::post(line);
  printf("%s\n", line.c_str());

  temp_vec = glm::vec4(mat[0][2], mat[1][2], mat[2][2], mat[3][2]);
  line = Vec4f_Str(temp_vec);
  // ddTerminal::post(line);
  printf("%s\n", line.c_str());

  temp_vec = glm::vec4(mat[0][3], mat[1][3], mat[2][3], mat[3][3]);
  line = Vec4f_Str(temp_vec);
  // ddTerminal::post(line);
  printf("%s\n", line.c_str());
}