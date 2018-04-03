#pragma once

#include "AssetNav.h"
#include "DD_ResourceLoader.h"
#include "DD_Types.h"

class DD_AssetViewer {
 public:
  DD_AssetViewer();
  ~DD_AssetViewer(){};

  DD_Resources* res_ptr;
  void Load();
  void setInterface(const float dt);
  DD_Event Update(DD_Event& event);

  size_t m_screenW;
  size_t m_screenH;
  float m_scrHorzDist;
  float m_scrVertDist;
  float bgcolor[4] = {0.f, 0.f, 0.f, 1.f};
  std::string m_cubeMapID, m_assetfile;
};
