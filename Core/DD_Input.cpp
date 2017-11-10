#include "DD_Input.h"

void DD_Input::UpdateKeyDown(SDL_Keysym & key)
{
	switch( key.sym ) {
		case SDLK_a:
			Keys[(unsigned)DD_Keys::A_Key] = true;
			break;
		case SDLK_b:
			Keys[(unsigned)DD_Keys::B_Key] = true;
			break;
		case SDLK_c:
			Keys[(unsigned)DD_Keys::C_Key] = true;
			break;
		case SDLK_d:
			Keys[(unsigned)DD_Keys::D_Key] = true;
			break;
		case SDLK_e:
			Keys[(unsigned)DD_Keys::E_Key] = true;
			break;
		case SDLK_f:
			Keys[(unsigned)DD_Keys::F_Key] = true;
			break;
		case SDLK_g:
			Keys[(unsigned)DD_Keys::G_Key] = true;
			break;
		case SDLK_h:
			Keys[(unsigned)DD_Keys::H_Key] = true;
			break;
		case SDLK_i:
			Keys[(unsigned)DD_Keys::I_Key] = true;
			break;
		case SDLK_j:
			Keys[(unsigned)DD_Keys::J_Key] = true;
			break;
		case SDLK_k:
			Keys[(unsigned)DD_Keys::K_Key] = true;
			break;
		case SDLK_l:
			Keys[(unsigned)DD_Keys::L_Key] = true;
			break;
		case SDLK_m:
			Keys[(unsigned)DD_Keys::M_Key] = true;
			break;
		case SDLK_n:
			Keys[(unsigned)DD_Keys::N_Key] = true;
			break;
		case SDLK_o:
			Keys[(unsigned)DD_Keys::O_Key] = true;
			break;
		case SDLK_p:
			Keys[(unsigned)DD_Keys::P_Key] = true;
			break;
		case SDLK_q:
			Keys[(unsigned)DD_Keys::Q_Key] = true;
			break;
		case SDLK_r:
			Keys[(unsigned)DD_Keys::R_Key] = true;
			break;
		case SDLK_s:
			Keys[(unsigned)DD_Keys::S_Key] = true;
			break;
		case SDLK_t:
			Keys[(unsigned)DD_Keys::T_Key] = true;
			break;
		case SDLK_u:
			Keys[(unsigned)DD_Keys::U_Key] = true;
			break;
		case SDLK_v:
			Keys[(unsigned)DD_Keys::V_Key] = true;
			break;
		case SDLK_w:
			Keys[(unsigned)DD_Keys::W_Key] = true;
			break;
		case SDLK_x:
			Keys[(unsigned)DD_Keys::X_Key] = true;
			break;
		case SDLK_y:
			Keys[(unsigned)DD_Keys::Y_Key] = true;
			break;
		case SDLK_z:
			Keys[(unsigned)DD_Keys::Z_Key] = true;
			break;
		case SDLK_SPACE:
			Keys[(unsigned)DD_Keys::Space_Key] = true;
			break;
		case SDLK_KP_ENTER: // doesn't work
			Keys[(unsigned)DD_Keys::Enter_Key] = true;
			break;
		case SDLK_ESCAPE:
			Keys[(unsigned)DD_Keys::Escape_Key] = true;
			break;
		case SDLK_LALT:
			Keys[(unsigned)DD_Keys::ALT_L_Key] = true;
			break;
		case SDLK_LCTRL:
			Keys[(unsigned)DD_Keys::CTRL_L_Key] = true;
			break;
		case SDLK_LSHIFT:
			Keys[(unsigned)DD_Keys::Shift_L_Key] = true;
			break;
		case SDLK_RALT:
			Keys[(unsigned)DD_Keys::ALT_R_Key] = true;
			break;
		case SDLK_RCTRL:
			Keys[(unsigned)DD_Keys::CTRL_R_Key] = true;
			break;
		case SDLK_RSHIFT:
			Keys[(unsigned)DD_Keys::SHIFT_R_Key] = true;
			break;
		case SDLK_UP:
			Keys[(unsigned)DD_Keys::UP_KEY] = true;
			break;
		case SDLK_DOWN:
			Keys[(unsigned)DD_Keys::DOWN_KEY] = true;
			break;
		case SDLK_TAB:
			Keys[(unsigned)DD_Keys::TAB_Key] = true;
			break;
		default:
			break;
	}
}

void DD_Input::UpdateKeyUp(SDL_Keysym & key)
{
	switch( key.sym ) {
		case SDLK_a:
			Keys[(unsigned)DD_Keys::A_Key] = false;
			break;
		case SDLK_b:
			Keys[(unsigned)DD_Keys::B_Key] = false;
			break;
		case SDLK_c:
			Keys[(unsigned)DD_Keys::C_Key] = false;
			break;
		case SDLK_d:
			Keys[(unsigned)DD_Keys::D_Key] = false;
			break;
		case SDLK_e:
			Keys[(unsigned)DD_Keys::E_Key] = false;
			break;
		case SDLK_f:
			Keys[(unsigned)DD_Keys::F_Key] = false;
			break;
		case SDLK_g:
			Keys[(unsigned)DD_Keys::G_Key] = false;
			break;
		case SDLK_h:
			Keys[(unsigned)DD_Keys::H_Key] = false;
			break;
		case SDLK_i:
			Keys[(unsigned)DD_Keys::I_Key] = false;
			break;
		case SDLK_j:
			Keys[(unsigned)DD_Keys::J_Key] = false;
			break;
		case SDLK_k:
			Keys[(unsigned)DD_Keys::K_Key] = false;
			break;
		case SDLK_l:
			Keys[(unsigned)DD_Keys::L_Key] = false;
			break;
		case SDLK_m:
			Keys[(unsigned)DD_Keys::M_Key] = false;
			break;
		case SDLK_n:
			Keys[(unsigned)DD_Keys::N_Key] = false;
			break;
		case SDLK_o:
			Keys[(unsigned)DD_Keys::O_Key] = false;
			break;
		case SDLK_p:
			Keys[(unsigned)DD_Keys::P_Key] = false;
			break;
		case SDLK_q:
			Keys[(unsigned)DD_Keys::Q_Key] = false;
			break;
		case SDLK_r:
			Keys[(unsigned)DD_Keys::R_Key] = false;
			break;
		case SDLK_s:
			Keys[(unsigned)DD_Keys::S_Key] = false;
			break;
		case SDLK_t:
			Keys[(unsigned)DD_Keys::T_Key] = false;
			break;
		case SDLK_u:
			Keys[(unsigned)DD_Keys::U_Key] = false;
			break;
		case SDLK_v:
			Keys[(unsigned)DD_Keys::V_Key] = false;
			break;
		case SDLK_w:
			Keys[(unsigned)DD_Keys::W_Key] = false;
			break;
		case SDLK_x:
			Keys[(unsigned)DD_Keys::X_Key] = false;
			break;
		case SDLK_y:
			Keys[(unsigned)DD_Keys::Y_Key] = false;
			break;
		case SDLK_z:
			Keys[(unsigned)DD_Keys::Z_Key] = false;
			break;
		case SDLK_SPACE:
			Keys[(unsigned)DD_Keys::Space_Key] = false;
			break;
		case SDLK_KP_ENTER: // doesn't work
			Keys[(unsigned)DD_Keys::Enter_Key] = false;
			break;
		case SDLK_ESCAPE:
			Keys[(unsigned)DD_Keys::Escape_Key] = false;
			break;
		case SDLK_LALT:
			Keys[(unsigned)DD_Keys::ALT_L_Key] = false;
			break;
		case SDLK_LCTRL:
			Keys[(unsigned)DD_Keys::CTRL_L_Key] = false;
			break;
		case SDLK_LSHIFT:
			Keys[(unsigned)DD_Keys::Shift_L_Key] = false;
			break;
		case SDLK_RALT:
			Keys[(unsigned)DD_Keys::ALT_R_Key] = false;
			break;
		case SDLK_RCTRL:
			Keys[(unsigned)DD_Keys::CTRL_R_Key] = false;
			break;
		case SDLK_RSHIFT:
			Keys[(unsigned)DD_Keys::SHIFT_R_Key] = false;
			break;
		case SDLK_UP:
			Keys[(unsigned)DD_Keys::UP_KEY] = false;
			break;
		case SDLK_DOWN:
			Keys[(unsigned)DD_Keys::DOWN_KEY] = false;
			break;
		case SDLK_TAB:
			Keys[(unsigned)DD_Keys::TAB_Key] = false;
			break;
		default:
			break;
	}
}

void DD_Input::UpdateMouse(SDL_MouseButtonEvent& key,
						   const bool down)
{
	if( down ) {
		switch( key.button ) {
			case SDL_BUTTON_LEFT:
				MouseClicks_L_M_R[0] = true;
				break;
			case SDL_BUTTON_MIDDLE:
				MouseClicks_L_M_R[1] = true;
				break;
			case SDL_BUTTON_RIGHT:
				MouseClicks_L_M_R[2] = true;
				break;
			default:
				break;
		}
	}
	else {
		switch( key.button ) {
			case SDL_BUTTON_LEFT:
				MouseClicks_L_M_R[0] = false;
				break;
			case SDL_BUTTON_MIDDLE:
				MouseClicks_L_M_R[1] = false;
				break;
			case SDL_BUTTON_RIGHT:
				MouseClicks_L_M_R[2] = false;
				break;
			default:
				break;
		}
	}
}

void DD_Input::UpdateMousePos(SDL_MouseMotionEvent & key)
{
	if( firstMouse ) {
		lastX = (int)key.x;
		lastY = (int)key.y;
	}
	newX = (int)key.x;
	newY = (int)key.y;
}

void DD_Input::UpdateMouseWheel(SDL_MouseWheelEvent & key)
{
	MouseY_Scroll = key.y;
}

inputBuff* DD_Input::GetInput()
{
	inputBuff* MyInput = new inputBuff();
	for( size_t i = 0; i < 512; i++ ) {
		MyInput->rawInput[i] = Keys[i];
	}
	MyInput->mouseLMR[0] = MouseClicks_L_M_R[0];
	MyInput->mouseLMR[1] = MouseClicks_L_M_R[1];
	MyInput->mouseLMR[2] = MouseClicks_L_M_R[2];
	MyInput->mouseScroll = MouseY_Scroll;

	int delta_x = 0, delta_y = 0;
	delta_x = lastX - newX;
	delta_y = lastY - newY;
	glm::vec2 xy_out = filterMouseInput(glm::vec2((float)delta_x,
		(float)delta_y));

	MyInput->mouseXDelta = (int)xy_out.x;
	MyInput->mouseYDelta = (int)xy_out.y;
	MyInput->mouseX = newX;
	MyInput->mouseY = newY;

	lastX = newX;
	lastY = newY;
	MouseY_Scroll = 0;

	return MyInput;
}

glm::vec2 DD_Input::filterMouseInput(glm::vec2 frame_in)
{
	for( size_t i = 9; i > 0; i-- ) {
		mouse_hist[i] = mouse_hist[i - 1];
	}
	mouse_hist[0] = frame_in;
	float avg_x = 0.f, avg_y = 0.f, avg_total = 0.f, current_weight = 1.f;

	for( size_t i = 0; i < 10; i++ ) {
		glm::vec2 tmp = mouse_hist[i];
		avg_x += tmp.x * current_weight;
		avg_y += tmp.y * current_weight;
		avg_total += 1.f * current_weight;
		current_weight *= MOUSE_FILTER_WEIGHT;
	}
	return glm::vec2(avg_x / avg_total, avg_y / avg_total);
}
