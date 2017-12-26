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
  MOUSE_RIGHT,
  MOUSE_MIDDLE,
  MOUSE_LEFT,
  MOUSE_XDELTA,
  MOUSE_YDELTA,
  MOUSE_X,
  MOUSE_Y,
  MOUSE_SCROLL,
  UP_KEY,
  DOWN_KEY,
  RIGHT_KEY,
  LEFT_KEY,
  TAB_Key,
  NUM_KEYS
};

struct key_flags {
  key_flags(const bool a = false, const int b = 0) : active(a), order(b) {}
  bool active;
  int order;
};

struct InputData {
  key_flags keys[DD_Keys::NUM_KEYS];
  int order_tracker = 0;
};

namespace DD_Input {
/// \brief Reset key order counter
void new_frame(InputData& input_data);
/// \brief Set true boolean flag for key
/// \param input_data key buffer
/// \param key
void update_keyup(InputData& input_data, SDL_Keysym& key);
/// \brief Set false boolean flag for ke
/// \param input_data key buffery
/// \param key
void update_keydown(InputData& input_data, SDL_Keysym& key);
/// \brief Set boolean flag for mouse button
/// \param input_data key buffer
/// \param key
void update_mouse_button(InputData& input_data, SDL_MouseButtonEvent& key,
                         const bool b_flag);
/// \brief Set mouse x & y integer position
/// \param input_data key buffer
/// \param key
void update_mouse_pos(InputData& input_data, SDL_MouseMotionEvent& key);
/// \brief Set scroll wheel delta distance (integer)
/// \param input_data key buffer
/// \param key
void update_mouse_wheel(InputData& input_data, SDL_MouseWheelEvent& key);
}  // namespace DD_Input
