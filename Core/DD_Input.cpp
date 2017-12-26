#include "DD_Input.h"

namespace {
int mouse_xdelta = 0;
int mouse_ydelta = 0;
int mouse_x = 0;
int mouse_y = 0;
int mouse_lastx = 0;
int mouse_lasty = 0;
int mouse_scroll = 0;
bool flag_first_mouse = false;
}  // namespace

/// \brief Modify keyboard keys
/// \param idata InputData buffer
/// \param sdlk SDL2 key code
/// \param b_flag
/// \param i_flag
void edit_key(InputData& idata, SDL_Keysym& key, const bool b_flag,
              const int i_flag);

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

namespace DD_Input {
void new_frame(InputData& input_data) { 
	// reset key order tracker
	input_data.order_tracker = 0; 
	// reset scroll wheel tracker
	input_data.keys[(unsigned)DD_Keys::MOUSE_SCROLL].order = 0;
}

void update_keyup(InputData& input_data, SDL_Keysym& key) {
  edit_key(input_data, key, true, input_data.order_tracker);
  input_data.order_tracker++;
}

void update_keydown(InputData& input_data, SDL_Keysym& key) {
  edit_key(input_data, key, false, input_data.order_tracker);
  input_data.order_tracker++;
}

void update_mouse_button(InputData& input_data, SDL_MouseButtonEvent& key,
                         const bool b_flag) {
  if (b_flag) {
    switch (key.button) {
      case SDL_BUTTON_LEFT:
				input_data.keys[(unsigned)DD_Keys::MOUSE_LEFT].active = true;
        break;
      case SDL_BUTTON_MIDDLE:
				input_data.keys[(unsigned)DD_Keys::MOUSE_MIDDLE].active = true;
        break;
      case SDL_BUTTON_RIGHT:
				input_data.keys[(unsigned)DD_Keys::MOUSE_RIGHT].active = true;
        break;
      default:
        break;
    }
  } else {
    switch (key.button) {
      case SDL_BUTTON_LEFT:
				input_data.keys[(unsigned)DD_Keys::MOUSE_LEFT].active = false;
        break;
      case SDL_BUTTON_MIDDLE:
				input_data.keys[(unsigned)DD_Keys::MOUSE_MIDDLE].active = false;
        break;
      case SDL_BUTTON_RIGHT:
				input_data.keys[(unsigned)DD_Keys::MOUSE_RIGHT].active = false;
        break;
      default:
        break;
    }
  }
}

void update_mouse_pos(InputData & input_data, SDL_MouseMotionEvent & key) {
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
	input_data.keys[(unsigned)DD_Keys::MOUSE_YDELTA].order = mouse_lasty - mouse_y;
	input_data.keys[(unsigned)DD_Keys::MOUSE_XDELTA].order = mouse_lastx - mouse_x;
	input_data.keys[(unsigned)DD_Keys::MOUSE_Y].order = mouse_y;
	input_data.keys[(unsigned)DD_Keys::MOUSE_X].order = mouse_x;
	// set old mouse position
	mouse_lastx = mouse_x;
	mouse_lasty = mouse_y;
}

void update_mouse_wheel(InputData & input_data, SDL_MouseWheelEvent & key) {
	input_data.keys[(unsigned)DD_Keys::MOUSE_SCROLL].order = key.y;
}

}  // namespace DD_Input

void edit_key(InputData& idata, SDL_Keysym& key, const bool b_flag,
              const int i_flag) {
  switch (key.sym) {
    case SDLK_a:
			idata.keys[(unsigned)DD_Keys::A_Key] = { b_flag, i_flag };
      break;
    case SDLK_b:
      idata.keys[(unsigned)DD_Keys::B_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::B_Key].order = i_flag;
      break;
    case SDLK_c:
      idata.keys[(unsigned)DD_Keys::C_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::C_Key].order = i_flag;
      break;
    case SDLK_d:
      idata.keys[(unsigned)DD_Keys::D_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::D_Key].order = i_flag;
      break;
    case SDLK_e:
      idata.keys[(unsigned)DD_Keys::E_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::E_Key].order = i_flag;
      break;
    case SDLK_f:
      idata.keys[(unsigned)DD_Keys::F_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::F_Key].order = i_flag;
      break;
    case SDLK_g:
      idata.keys[(unsigned)DD_Keys::G_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::G_Key].order = i_flag;
      break;
    case SDLK_h:
      idata.keys[(unsigned)DD_Keys::H_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::H_Key].order = i_flag;
      break;
    case SDLK_i:
      idata.keys[(unsigned)DD_Keys::I_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::I_Key].order = i_flag;
      break;
    case SDLK_j:
      idata.keys[(unsigned)DD_Keys::J_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::J_Key].order = i_flag;
      break;
    case SDLK_k:
      idata.keys[(unsigned)DD_Keys::K_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::K_Key].order = i_flag;
      break;
    case SDLK_l:
      idata.keys[(unsigned)DD_Keys::L_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::L_Key].order = i_flag;
      break;
    case SDLK_m:
      idata.keys[(unsigned)DD_Keys::M_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::M_Key].order = i_flag;
      break;
    case SDLK_n:
      idata.keys[(unsigned)DD_Keys::N_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::N_Key].order = i_flag;
      break;
    case SDLK_o:
      idata.keys[(unsigned)DD_Keys::O_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::O_Key].order = i_flag;
      break;
    case SDLK_p:
      idata.keys[(unsigned)DD_Keys::P_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::P_Key].order = i_flag;
      break;
    case SDLK_q:
      idata.keys[(unsigned)DD_Keys::Q_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::Q_Key].order = i_flag;
      break;
    case SDLK_r:
      idata.keys[(unsigned)DD_Keys::R_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::R_Key].order = i_flag;
      break;
    case SDLK_s:
      idata.keys[(unsigned)DD_Keys::S_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::S_Key].order = i_flag;
      break;
    case SDLK_t:
      idata.keys[(unsigned)DD_Keys::T_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::T_Key].order = i_flag;
      break;
    case SDLK_u:
      idata.keys[(unsigned)DD_Keys::U_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::U_Key].order = i_flag;
      break;
    case SDLK_v:
      idata.keys[(unsigned)DD_Keys::V_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::V_Key].order = i_flag;
      break;
    case SDLK_w:
      idata.keys[(unsigned)DD_Keys::W_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::W_Key].order = i_flag;
      break;
    case SDLK_x:
      idata.keys[(unsigned)DD_Keys::X_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::X_Key].order = i_flag;
      break;
    case SDLK_y:
      idata.keys[(unsigned)DD_Keys::Y_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::Y_Key].order = i_flag;
      break;
    case SDLK_z:
      idata.keys[(unsigned)DD_Keys::Z_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::Z_Key].order = i_flag;
      break;
    case SDLK_SPACE:
      idata.keys[(unsigned)DD_Keys::Space_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::Space_Key].order = i_flag;
      break;
    case SDLK_KP_ENTER:  // doesn't work
      idata.keys[(unsigned)DD_Keys::Enter_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::Enter_Key].order = i_flag;
      break;
    case SDLK_ESCAPE:
      idata.keys[(unsigned)DD_Keys::Escape_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::Escape_Key].order = i_flag;
      break;
    case SDLK_LALT:
      idata.keys[(unsigned)DD_Keys::ALT_L_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::ALT_L_Key].order = i_flag;
      break;
    case SDLK_LCTRL:
      idata.keys[(unsigned)DD_Keys::CTRL_L_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::CTRL_L_Key].order = i_flag;
      break;
    case SDLK_LSHIFT:
      idata.keys[(unsigned)DD_Keys::Shift_L_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::Shift_L_Key].order = i_flag;
      break;
    case SDLK_RALT:
      idata.keys[(unsigned)DD_Keys::ALT_R_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::ALT_R_Key].order = i_flag;
      break;
    case SDLK_RCTRL:
      idata.keys[(unsigned)DD_Keys::CTRL_R_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::CTRL_R_Key].order = i_flag;
      break;
    case SDLK_RSHIFT:
      idata.keys[(unsigned)DD_Keys::SHIFT_R_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::SHIFT_R_Key].order = i_flag;
      break;
    case SDLK_UP:
      idata.keys[(unsigned)DD_Keys::UP_KEY].active = b_flag;
      idata.keys[(unsigned)DD_Keys::UP_KEY].order = i_flag;
      break;
    case SDLK_DOWN:
      idata.keys[(unsigned)DD_Keys::DOWN_KEY].active = b_flag;
      idata.keys[(unsigned)DD_Keys::DOWN_KEY].order = i_flag;
      break;
    case SDLK_RIGHT:
      idata.keys[(unsigned)DD_Keys::RIGHT_KEY].active = b_flag;
      idata.keys[(unsigned)DD_Keys::RIGHT_KEY].order = i_flag;
      break;
    case SDLK_LEFT:
      idata.keys[(unsigned)DD_Keys::LEFT_KEY].active = b_flag;
      idata.keys[(unsigned)DD_Keys::LEFT_KEY].order = i_flag;
      break;
    case SDLK_TAB:
      idata.keys[(unsigned)DD_Keys::TAB_Key].active = b_flag;
      idata.keys[(unsigned)DD_Keys::TAB_Key].order = i_flag;
      break;
    default:
      break;
  }
}
