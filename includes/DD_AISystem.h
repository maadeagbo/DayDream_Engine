#pragma once

/*
* Copyright (c) 2017, Moses Adeagbo
* All rights reserved.
*/

#include "DD_Types.h"

#include "DD_AITypes.h"
#include "DD_EventQueue.h"
#include "DD_MathLib.h"
#include "DD_MeshTypes.h"
#include "DD_ResourceLoader.h"

/**
        Manages all AI instances w/in engine. Implements handler interface in
        update() function.
*/
class DD_AISystem {
 public:
  DD_AISystem() {}
  ~DD_AISystem() {}

  DD_Resources* res_ptr;
  DD_Event update(DD_Event& event);

  void generatePRM(DD_AIObject* ai_obj);
  void mapPRMtoLines(DD_AIObject* ai_obj);
  void createLinePath(AI_Agent* agent, const char* lineID);
  void moveAlongPath(DD_AIObject* ai_obj, AI_Agent* agent, const float dt);
  glm::vec3 ObstacleAvoidance(AI_Agent* agent);
  float timeToCollision(AI_Agent* agentA, AI_Agent* agentB);
  void SimulateCrowd(AI_Agent* agent, const float dt);

 private:
};
