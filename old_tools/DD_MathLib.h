#pragma once

/*
* Copyright (c) 2017, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	DD_MathLib:
*		-	Contain miscellanious math functions for general use
*
*	TODO:	==
*			==
-----------------------------------------------------------------------------*/

#include "DD_MeshTypes.h"
#include "DD_ResourceLoader.h"
#include "DD_Types.h"

struct raycastBuff : public BaseData {
  glm::vec4 pos;
  size_t index = 0;
  bool hit;
};

namespace DD_MathLib {
void setResourceBin(DD_Resources* res_bin);

bool checkRayCircle(const glm::vec3 ray, const glm::vec3 point,
                    const glm::vec3 center, const float r);

float getHaltonValue(int index, const int base);

bool checkAABBoxPoint(BoundingBox& bbox, glm::vec4 point);
raycastBuff rayCast(const int mouseX, const int mouseY,
                    const char* agentID = "");
glm::vec3 rayCastW(const int mouseX, const int mouseY);
void randomSampleRect(const glm::vec3 center, glm::vec3 min, glm::vec3 max,
                      dd_array<glm::vec3>& points);
size_t calculateLOSLines(const size_t index, const dd_array<AI_Obstacle>& obst,
                         const dd_array<glm::vec3> other,
                         const dd_array<LinePoint>& container,
                         const dd_array<glm::vec2>& cost_bin,
                         const float agent_radius,
                         const float search_radius = -1.f);
}
