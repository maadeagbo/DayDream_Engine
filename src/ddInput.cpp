#include "ddInput.h"
#include <imgui.h>
#include <imgui_impl_glfw_gl3.h>

namespace {
float mouse_x = 0;
float mouse_y = 0;
float mouse_lastx = 0;
float mouse_lasty = 0;
bool flag_first_mouse = false;

InputData global_input;
}  // namespace

/// \brief Modify keyboard keys
/// \param idata InputData buffer
/// \param sdlk SDL2 key code
/// \param b_flag
/// \param i_flag
void edit_key(int key, const bool b_flag, const float i_flag);

/*
inputBuff* DD_Input_::GetInput() {
  inputBuff* MyInput = new inputBuff();
  for (size_t i = 0; i < 512; i++) {
    MyInput->rawInput[i] = Keys[i];
  }
  MyInput->mouseLMR[0] = MouseClicks_L_M_R[0];
  MyInput->mouseLMR[1] = MouseClicks_L_M_R[1];
  MyInput->mouseLMR[2] = MouseClicks_L_M_R[2];
  MyInput->mouseScroll = MouseY_Scroll;

  int delta_x = 0, delta_y = 0;
  delta_x = lastX - newX;
  delta_y = lastY - newY;
  glm::vec2 xy_out =
      filterMouseInput(glm::vec2((float)delta_x, (float)delta_y));

  MyInput->mouseXDelta = (int)xy_out.x;
  MyInput->mouseYDelta = (int)xy_out.y;
  MyInput->mouseX = newX;
  MyInput->mouseY = newY;

  lastX = newX;
  lastY = newY;
  MouseY_Scroll = 0;

  return MyInput;
}

glm::vec2 DD_Input_::filterMouseInput(glm::vec2 frame_in) {
  for (size_t i = 9; i > 0; i--) {
    mouse_hist[i] = mouse_hist[i - 1];
  }
  mouse_hist[0] = frame_in;
  float avg_x = 0.f, avg_y = 0.f, avg_total = 0.f, current_weight = 1.f;

  for (size_t i = 0; i < 10; i++) {
    glm::vec2 tmp = mouse_hist[i];
    avg_x += tmp.x * current_weight;
    avg_y += tmp.y * current_weight;
    avg_total += 1.f * current_weight;
    current_weight *= MOUSE_FILTER_WEIGHT;
  }
  return glm::vec2(avg_x / avg_total, avg_y / avg_total);
}
//*/

namespace ddInput {
void new_frame() {
  // reset key order tracker
  global_input.order_tracker = 0;
  // reset scroll wheel tracker
  global_input.keys[(unsigned)DD_Keys::MOUSE_SCROLL].order = 0;
}

/*
void update_keyup(SDL_Keysym& key) {
  edit_key(key, true, global_input.order_tracker);
  global_input.order_tracker++;
}

void update_keydown(SDL_Keysym& key) {
  edit_key(key, false, global_input.order_tracker);
  global_input.order_tracker++;
}

void update_mouse_button(SDL_MouseButtonEvent& key, const bool b_flag) {
  if (b_flag) {
    switch (key.button) {
      case SDL_BUTTON_LEFT:
        global_input.keys[(unsigned)DD_Keys::MOUSE_LEFT].active = true;
        break;
      case SDL_BUTTON_MIDDLE:
        global_input.keys[(unsigned)DD_Keys::MOUSE_MIDDLE].active = true;
        break;
      case SDL_BUTTON_RIGHT:
        global_input.keys[(unsigned)DD_Keys::MOUSE_RIGHT].active = true;
        break;
      default:
        break;
    }
  } else {
    switch (key.button) {
      case SDL_BUTTON_LEFT:
        global_input.keys[(unsigned)DD_Keys::MOUSE_LEFT].active = false;
        break;
      case SDL_BUTTON_MIDDLE:
        global_input.keys[(unsigned)DD_Keys::MOUSE_MIDDLE].active = false;
        break;
      case SDL_BUTTON_RIGHT:
        global_input.keys[(unsigned)DD_Keys::MOUSE_RIGHT].active = false;
        break;
      default:
        break;
    }
  }
}

void update_mouse_pos(SDL_MouseMotionEvent& key) {
  if (flag_first_mouse) {
    // set up for 1st movement
    mouse_lastx = (int)key.x;
    mouse_lasty = (int)key.y;
    flag_first_mouse = false;
  }
  // log current mouse movement
  mouse_x = (int)key.x;
  mouse_y = (int)key.y;
  // get delta
  global_input.keys[(unsigned)DD_Keys::MOUSE_YDELTA].order =
      mouse_lasty - mouse_y;
  global_input.keys[(unsigned)DD_Keys::MOUSE_XDELTA].order =
      mouse_lastx - mouse_x;
  global_input.keys[(unsigned)DD_Keys::MOUSE_Y].order = mouse_y;
  global_input.keys[(unsigned)DD_Keys::MOUSE_X].order = mouse_x;
  // set old mouse position
  mouse_lastx = mouse_x;
  mouse_lasty = mouse_y;
}

void update_mouse_wheel(SDL_MouseWheelEvent& key) {
  global_input.keys[(unsigned)DD_Keys::MOUSE_SCROLL].order = key.y;
}

//*/

const InputData& get_input() { return global_input; }

void send_upstream_to_lua(lua_State* L) {
  /** \brief Each key in the table will be another 2-item table. This sets it*/
  auto set_table_field = [&](const char* id, const bool* flag, const float* val) {
    if (flag) {
      lua_pushboolean(L, *flag);
      lua_setfield(L, -2, id);
    } else {
      if (val) {
        lua_pushnumber(L, *val);
        lua_setfield(L, -2, id);
      }
    }
  };

  // create table and fill up w/ InputData
  //*
  lua_newtable(L);  // create new table and put on top of stack
  luaL_checkstack(L, 2, "too many arguments");  // check stack size

  // set fields
  key_flags kf = global_input.keys[(unsigned)DD_Keys::A_Key];
  set_table_field("a", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::B_Key];
  set_table_field("b", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::C_Key];
  set_table_field("c", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::D_Key];
  set_table_field("d", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::E_Key];
  set_table_field("e", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::F_Key];
  set_table_field("f", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::G_Key];
  set_table_field("g", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::H_Key];
  set_table_field("h", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::I_Key];
  set_table_field("i", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::J_Key];
  set_table_field("j", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::K_Key];
  set_table_field("k", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::L_Key];
  set_table_field("l", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::M_Key];
  set_table_field("m", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::N_Key];
  set_table_field("n", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::O_Key];
  set_table_field("o", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::P_Key];
  set_table_field("p", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::Q_Key];
  set_table_field("q", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::R_Key];
  set_table_field("r", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::S_Key];
  set_table_field("s", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::T_Key];
  set_table_field("t", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::U_Key];
  set_table_field("u", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::V_Key];
  set_table_field("v", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::W_Key];
  set_table_field("w", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::X_Key];
  set_table_field("x", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::Y_Key];
  set_table_field("y", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::Z_Key];
  set_table_field("z", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::Space_Key];
  set_table_field("space", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::Enter_Key];
  set_table_field("enter", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::ALT_L_Key];
  set_table_field("l_alt", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::CTRL_L_Key];
  set_table_field("l_ctrl", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::Shift_L_Key];
  set_table_field("l_shift", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::ALT_R_Key];
  set_table_field("r_alt", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::CTRL_R_Key];
  set_table_field("r_ctrl", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::SHIFT_R_Key];
  set_table_field("r_shift", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::MOUSE_RIGHT];
  set_table_field("mouse_b_r", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::MOUSE_MIDDLE];
  set_table_field("mouse_b_m", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::MOUSE_LEFT];
  set_table_field("mouse_b_l", &kf.active, nullptr);

  global_input.keys[(unsigned)DD_Keys::MOUSE_XDELTA].order =
      mouse_lastx - mouse_x;
  kf = global_input.keys[(unsigned)DD_Keys::MOUSE_XDELTA];
  set_table_field("mouse_x_delta", nullptr, &kf.order);
  global_input.keys[(unsigned)DD_Keys::MOUSE_YDELTA].order =
      mouse_lasty - mouse_y;
  kf = global_input.keys[(unsigned)DD_Keys::MOUSE_YDELTA];

  set_table_field("mouse_y_delta", nullptr, &kf.order);
  kf = global_input.keys[(unsigned)DD_Keys::MOUSE_X];
  set_table_field("mouse_x", nullptr, &kf.order);
  kf = global_input.keys[(unsigned)DD_Keys::MOUSE_Y];
  set_table_field("mouse_y", nullptr, &kf.order);
  kf = global_input.keys[(unsigned)DD_Keys::MOUSE_SCROLL];
  set_table_field("mouse_scroll", nullptr, &kf.order);
  kf = global_input.keys[(unsigned)DD_Keys::UP_KEY];
  set_table_field("up", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::DOWN_KEY];
  set_table_field("down", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::LEFT_KEY];
  set_table_field("left", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::RIGHT_KEY];
  set_table_field("right", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::TAB_Key];
  set_table_field("tab", &kf.active, nullptr);
  kf = global_input.keys[(unsigned)DD_Keys::TILDE];
  set_table_field("grave", &kf.active, nullptr);

  lua_setglobal(L, "__dd_input");

	// set old mouse position
	mouse_lastx = mouse_x;
	mouse_lasty = mouse_y;
  //*/
}

}  // namespace ddInput

//*
void edit_key(int key, const bool b_flag, const float i_flag) {
  switch (key) {
    case GLFW_KEY_A:
      global_input.keys[(unsigned)DD_Keys::A_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_B:
      global_input.keys[(unsigned)DD_Keys::B_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_C:
      global_input.keys[(unsigned)DD_Keys::C_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_D:
      global_input.keys[(unsigned)DD_Keys::D_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_E:
      global_input.keys[(unsigned)DD_Keys::E_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_F:
      global_input.keys[(unsigned)DD_Keys::F_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_G:
      global_input.keys[(unsigned)DD_Keys::G_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_H:
      global_input.keys[(unsigned)DD_Keys::H_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_I:
      global_input.keys[(unsigned)DD_Keys::I_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_J:
      global_input.keys[(unsigned)DD_Keys::J_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_K:
      global_input.keys[(unsigned)DD_Keys::K_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_L:
      global_input.keys[(unsigned)DD_Keys::L_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_M:
      global_input.keys[(unsigned)DD_Keys::M_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_N:
      global_input.keys[(unsigned)DD_Keys::N_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_O:
      global_input.keys[(unsigned)DD_Keys::O_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_P:
      global_input.keys[(unsigned)DD_Keys::P_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_Q:
      global_input.keys[(unsigned)DD_Keys::Q_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_R:
      global_input.keys[(unsigned)DD_Keys::R_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_S:
      global_input.keys[(unsigned)DD_Keys::S_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_T:
      global_input.keys[(unsigned)DD_Keys::T_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_U:
      global_input.keys[(unsigned)DD_Keys::U_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_V:
      global_input.keys[(unsigned)DD_Keys::V_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_W:
      global_input.keys[(unsigned)DD_Keys::W_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_X:
      global_input.keys[(unsigned)DD_Keys::X_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_Y:
      global_input.keys[(unsigned)DD_Keys::Y_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_Z:
      global_input.keys[(unsigned)DD_Keys::Z_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_SPACE:
      global_input.keys[(unsigned)DD_Keys::Space_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_ENTER:
      global_input.keys[(unsigned)DD_Keys::Enter_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_ESCAPE:
      global_input.keys[(unsigned)DD_Keys::Escape_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_LEFT_ALT:
      global_input.keys[(unsigned)DD_Keys::ALT_L_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_LEFT_CONTROL:
      global_input.keys[(unsigned)DD_Keys::CTRL_L_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_LEFT_SHIFT:
      global_input.keys[(unsigned)DD_Keys::Shift_L_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_RIGHT_ALT:
      global_input.keys[(unsigned)DD_Keys::ALT_R_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_RIGHT_CONTROL:
      global_input.keys[(unsigned)DD_Keys::CTRL_R_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_RIGHT_SHIFT:
      global_input.keys[(unsigned)DD_Keys::SHIFT_R_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_UP:
      global_input.keys[(unsigned)DD_Keys::UP_KEY] = {b_flag, i_flag};
      break;
    case GLFW_KEY_DOWN:
      global_input.keys[(unsigned)DD_Keys::DOWN_KEY] = {b_flag, i_flag};
      break;
    case GLFW_KEY_RIGHT:
      global_input.keys[(unsigned)DD_Keys::RIGHT_KEY] = {b_flag, i_flag};
      break;
    case GLFW_KEY_LEFT:
      global_input.keys[(unsigned)DD_Keys::LEFT_KEY] = {b_flag, i_flag};
      break;
    case GLFW_KEY_TAB:
      global_input.keys[(unsigned)DD_Keys::TAB_Key] = {b_flag, i_flag};
      break;
    case GLFW_KEY_GRAVE_ACCENT:
      global_input.keys[(unsigned)DD_Keys::TILDE] = {b_flag, i_flag};
      break;
    default:
      break;
  }
}
//*/

void dd_key_callback(GLFWwindow* window, int key, int scancode, int action,
                     int mods) {
  if (action == GLFW_PRESS) {
    edit_key(key, true, global_input.order_tracker);  // key down
  }
  if (action == GLFW_RELEASE) {
    edit_key(key, false, global_input.order_tracker);  // key up
  }
  ImGui_ImplGlfwGL3_KeyCallback(window, key, scancode, action, mods);
}

void dd_mouse_pos_callback(GLFWwindow* window, double xpos, double ypos) {
  if (flag_first_mouse) {
    // set up for 1st movement
    mouse_lastx = (float)xpos;
    mouse_lasty = (float)ypos;
    flag_first_mouse = false;
  }
	// set old mouse position
	mouse_lastx = mouse_x;
	mouse_lasty = mouse_y;
  // log current mouse movement
  mouse_x = (float)xpos;
  mouse_y = (float)ypos;
  global_input.keys[(unsigned)DD_Keys::MOUSE_Y].order = mouse_y;
  global_input.keys[(unsigned)DD_Keys::MOUSE_X].order = mouse_x;
}

void dd_mouse_click_callback(GLFWwindow* window, int button, int action,
                             int mods) {
  switch (button) {
    case GLFW_MOUSE_BUTTON_RIGHT:
      if (action == GLFW_PRESS) {
        global_input.keys[(unsigned)DD_Keys::MOUSE_RIGHT].active = true;
      }
      if (action == GLFW_RELEASE) {
        global_input.keys[(unsigned)DD_Keys::MOUSE_RIGHT].active = false;
      }
      break;
    case GLFW_MOUSE_BUTTON_MIDDLE:
      if (action == GLFW_PRESS) {
        global_input.keys[(unsigned)DD_Keys::MOUSE_MIDDLE].active = true;
      }
      if (action == GLFW_RELEASE) {
        global_input.keys[(unsigned)DD_Keys::MOUSE_MIDDLE].active = false;
      }
      break;
    case GLFW_MOUSE_BUTTON_LEFT:
      if (action == GLFW_PRESS) {
        global_input.keys[(unsigned)DD_Keys::MOUSE_LEFT].active = true;
      }
      if (action == GLFW_RELEASE) {
        global_input.keys[(unsigned)DD_Keys::MOUSE_LEFT].active = false;
      }
      break;
    default:
      break;
  }
  ImGui_ImplGlfwGL3_MouseButtonCallback(window, button, action, mods);
}

void dd_scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
  global_input.keys[(unsigned)DD_Keys::MOUSE_SCROLL].order = (float)yoffset;

  ImGui_ImplGlfwGL3_ScrollCallback(window, xoffset, yoffset);
}
