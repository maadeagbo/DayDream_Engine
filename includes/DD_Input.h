#pragma once

/*
* Copyright (c) 2016, Moses Adeagbo
* All rights reserved.
*/

/*-----------------------------------------------------------------------------
*
*	DD_Input:
*		- Class for setting key inputs from sdl
*			-
*
*	TODO:	==
*			==
*
-----------------------------------------------------------------------------*/

#include <SDL.h>
#include "DD_Types.h"

enum DD_Keys {
  A_Key,
  B_Key,
  C_Key,
  D_Key,
  E_Key,
  F_Key,
  G_Key,
  H_Key,
  I_Key,
  J_Key,
  K_Key,
  L_Key,
  M_Key,
  N_Key,
  O_Key,
  P_Key,
  Q_Key,
  R_Key,
  S_Key,
  T_Key,
  U_Key,
  V_Key,
  W_Key,
  X_Key,
  Y_Key,
  Z_Key,
  Space_Key,
  Enter_Key,
  Escape_Key,
  ALT_L_Key,
  CTRL_L_Key,
  Shift_L_Key,
  ALT_R_Key,
  CTRL_R_Key,
  SHIFT_R_Key,
  MOUSE_DOWN,
  UP_KEY,
  DOWN_KEY,
  TAB_Key,
  NUM_KEYS
};

struct DD_Input {
  DD_Input() {
    for (size_t i = 0; i < 512; i++) {
      Keys[i] = false;
    }
    for (size_t i = 0; i < 10; i++) {
      mouse_hist[i] = glm::vec2(0.0f);
    }
    firstMouse = false;
    MouseClicks_L_M_R[0] = false;
    MouseClicks_L_M_R[1] = false;
    MouseClicks_L_M_R[2] = false;
    MouseY_Scroll = 0;
  }

  glm::vec2 filterMouseInput(glm::vec2 frame_in);
  void UpdateKeyDown(SDL_Keysym& key);
  void UpdateKeyUp(SDL_Keysym& key);
  void UpdateMouse(SDL_MouseButtonEvent& key, const bool down);
  void UpdateMousePos(SDL_MouseMotionEvent& key);
  void UpdateMouseWheel(SDL_MouseWheelEvent& key);

  inputBuff* GetInput();

  float MOUSE_FILTER_WEIGHT = 0.2f;
  glm::vec2 mouse_hist[10];
  int lastX, lastY, newX, newY, MouseY_Scroll;
  bool Keys[(unsigned)DD_Keys::NUM_KEYS], firstMouse, MouseClicks_L_M_R[3];
};
