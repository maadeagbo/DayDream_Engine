#pragma once
#include "DD_Agent.h"

enum class AssetNavUI {
  SPACE,
  LMOUSE,
  RMOUSE,
  ESC,
  CTRL_L,
  ALT_L,
  SHIFT_L,
  COUNT
};

class AssetNav : public DD_Agent {
 public:
  AssetNav(const char* ID);
  ~AssetNav() {}

  DD_Event Update(DD_Event event);
  void initRot(const float rad_pitch, const float rad_Yaw);
  void lockRotMode(const bool set);

  bool pressed[(u32)AssetNavUI::COUNT];
  int mouseX = 0, mouseY = 0;
  bool locked_rot;
  bool ignore_controls;

 private:
};