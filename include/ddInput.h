#pragma once

/*
 * Copyright (c) 2016, Moses Adeagbo
 * All rights reserved.
 */

/*-----------------------------------------------------------------------------
*
*	ddInput:
*		- Class for setting key inputs from sdl
*			-
*
*	TODO:	==
*			==
*
-----------------------------------------------------------------------------*/

//#include <SDL.h>
#include "GLFW/glfw3.h"
#include "LuaHooks.h"
#include "ddIncludes.h"

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
  TILDE,
  NUM_KEYS
};

struct key_flags {
  key_flags(const bool a = false, const float b = 0) : active(a), order(b) {}
  bool active;
  float order;
};

struct InputData {
  key_flags keys[DD_Keys::NUM_KEYS];
  int order_tracker = 0;
};

/** \brief Interace for getting peripheral input information */
namespace ddInput {
/** \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine */
void new_frame();

/*
/// \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine
void update_keyup(SDL_Keysym& key);
/// \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine
void update_keydown(SDL_Keysym& key);
/// \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine
void update_mouse_button(SDL_MouseButtonEvent& key, const bool b_flag);
/// \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine
void update_mouse_pos(SDL_MouseMotionEvent& key);
/// \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine
void update_mouse_wheel(SDL_MouseWheelEvent& key);
//*/

/**
 * \brief Retrieve input for current frame
 * \return InputData containing key press information
 */
const InputData &get_input();

/**
 * \brief Make available input data to lua script
 */
void send_upstream_to_lua(lua_State *L);
}  // namespace ddInput

/** \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine */
void dd_key_callback(GLFWwindow *window, int key, int scancode, int action,
                     int mods);

/** \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine */
void dd_mouse_pos_callback(GLFWwindow *window, double xpos, double ypos);

/** \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine */
void dd_mouse_click_callback(GLFWwindow *window, int button, int action,
                             int mods);

/** \brief DO NOT CALL. ONLY TO BE USED INTERNALLY BY ddEngine */
void dd_scroll_callback(GLFWwindow *window, double xoffset, double yoffset);