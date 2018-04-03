#pragma once

/*
* Copyright (c) 2017, Moses Adeagbo
* All rights reserved.
*/

#include "DD_ResourceLoader.h"
#include "DD_Types.h"

/**
        \brief Manages miscellaneous compute shader submissions
*/
class DD_Compute {
 public:
  DD_Compute() {}
  ~DD_Compute();

  DD_Resources* res_ptr;
  PushFunc push;

  void init();
  DD_Event Compute(DD_Event& event);
  void compute(DD_LEvent& _event);

 private:
  GLuint m_kinectDepthBuff[2];
};
