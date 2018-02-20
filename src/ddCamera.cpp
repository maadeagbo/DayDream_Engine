#include "ddCamera.h"
#include "ddTerminal.h"

/*
        Calculate and return view matrix for specified eye position
        \param _left flag for calculating view from left eye
        \param _right flag for calculating view from right eye
        \param dist Distance between eye in meters
*/
// glm::mat4 CamSpace::GetViewMatrixVREye(const DD_Camera* cam, const bool
// _left,
//                                        const bool _right, const float dist) {
//   glm::vec3 front = glm::normalize(cam->camQuat() * cam->front());
//   // cam->setFVec(glm::vec4(front.x, front.y, front.z, 1.f));

//   glm::vec3 right =
//       glm::normalize(glm::cross(front, glm::vec3(cam->worldUp())));
//   glm::vec3 up = glm::normalize(glm::cross(right, front));

//   glm::vec3 pos = glm::vec3(cam->pos());
//   glm::mat4 view;

//   if (_left) {
//     pos += (right * (dist / 2));
//     view = glm::lookAt(pos, pos + front, up);
//   } else if (_right) {
//     pos += (-right * (dist / 2));
//     view = glm::lookAt(pos, pos + front, up);
//   }
//   return view;
// }

/*
        Calculate and return off-axis perspective projection matrix
        \param screenPos Absolute location of screen/monitor
        \param scrWidth Horizontal length of screen
        \param scrHeight Vertical length of screen
        \param eyePos Absolute location of eye
*/
// glm::mat4 CamSpace::GetOffAxisProjMatrix(const DD_Camera* cam,
//                                          const glm::vec3 screenPos,
//                                          const float scrWidth,
//                                          const float scrHeight,
//                                          const glm::vec3 eyePos) {
//   // get corners of screen (assume screen is flat and on wall)
//   glm::vec3 wcsTopLeftScr(screenPos.x - scrWidth / 2,
//                           screenPos.y + scrHeight / 2, screenPos.z);
//   glm::vec3 wcsBotRightScr(screenPos.x + scrWidth / 2,
//                            screenPos.y - scrHeight / 2, screenPos.z);

//   // printf("Eye     %.5f %.5f, %.5f\n", eyePos.x, eyePos.y, eyePos.z);
//   // printf("Screen  %.5f %.5f, %.5f\n", screenPos.x, screenPos.y,
//   screenPos.z);

//   glm::vec3 camTopLeftScr = wcsTopLeftScr - eyePos;
//   glm::vec3 camTopLeftNear = cam->near_plane / camTopLeftScr.z *
//   camTopLeftScr;
//   glm::vec3 camBotRightScr = wcsBotRightScr - eyePos;
//   glm::vec3 camBotRightNear =
//       camBotRightScr / camBotRightScr.z * cam->near_plane;

//   /*printf("%.5f, ", -camTopLeftNear.x);
//   printf("%.5f, ", -camBotRightNear.x);
//   printf("%.5f, ", -camBotRightNear.y);
//   printf("%.5f, ", -camTopLeftNear.y);
//   printf("%.5f, ", cam->near_plane);
//   printf("%.5f, ", cam->far_plane);
//   printf("\n");*/

//   glm::mat4 frst =
//       glm::frustum(-camTopLeftNear.x, -camBotRightNear.x, -camBotRightNear.y,
//                    -camTopLeftNear.y, cam->near_plane, cam->far_plane);
//   return frst;
// }

#define DDCAM_META_NAME "LuaClass.ddCam"
#define check_ddCam(L) (ddCam **)luaL_checkudata(L, 1, DDCAM_META_NAME)

// static method functions
static int get_id(lua_State *L);
static int set_active(lua_State *L);
static int get_fov(lua_State *L);
static int set_fov(lua_State *L);
// p = pitch, y = yaw, r = roll
static int get_p_y_r(lua_State *L);
static int set_p_y_r(lua_State *L);

static int to_string(lua_State *L);

// method list
static const struct luaL_Reg cam_methods[] = {{"id", get_id},
                                              {"set_fovh", set_fov},
                                              {"fovh", get_fov},
                                              {"set_active", set_active},
                                              {"eulerPYR", get_p_y_r},
                                              {"set_eulerPYR", set_p_y_r},
                                              {"__tostring", to_string},
                                              {NULL, NULL}};

const char *ddCam_meta_name() { return DDCAM_META_NAME; }

void log_meta_ddCam(lua_State *L) {
  luaL_newmetatable(L, DDCAM_META_NAME);  // create meta table
  lua_pushvalue(L, -1);                   /* duplicate the metatable */
  lua_setfield(L, -2, "__index");         /* mt.__index = mt */
  luaL_setfuncs(L, cam_methods, 0);       /* register metamethods */
}

int get_id(lua_State *L) {
  ddCam *cam = *check_ddCam(L);
  lua_pushinteger(L, cam->id);
  return 1;
}

int set_active(lua_State *L) {
  ddCam *cam = *check_ddCam(L);
  if (lua_isboolean(L, -1)) {
    cam->active = (bool)lua_toboolean(L, -1);
  } else {
    ddTerminal::f_post("Invalid boolean argument for cam: %u", cam->id);
  }

  return 0;
}

int get_fov(lua_State *L) {
  ddCam *cam = *check_ddCam(L);
  lua_pushnumber(L, cam->fovh);
  return 1;
}

int set_fov(lua_State *L) {
  ddCam *cam = *check_ddCam(L);
  if (lua_isnumber(L, -1)) {
    cam->fovh = (float)lua_tonumber(L, -1);
  } else {
    ddTerminal::f_post("Invalid fov argument for cam: %u", cam->id);
  }

  return 0;
}

int get_p_y_r(lua_State *L) {
  ddCam *cam = *check_ddCam(L);
  push_vec3_to_lua(L, cam->pitch, cam->yaw, cam->roll);
  return 1;
}

int set_p_y_r(lua_State *L) {
  ddCam *cam = *check_ddCam(L);
  int top = lua_gettop(L);
  for (unsigned i = 2; i <= (unsigned)top; i++) {
    if (lua_isnumber(L, i)) {
      switch (i) {
        case 2:
          cam->pitch = (float)lua_tonumber(L, i);
          break;
        case 3:
          cam->yaw = (float)lua_tonumber(L, i);
          break;
        case 4:
          cam->roll = (float)lua_tonumber(L, i);
          break;
        default:
          break;
      }
    }
  }

  return 0;
}

int to_string(lua_State *L) {
  ddCam *cam = *check_ddCam(L);
  cbuff<64> out;
  out.format("ddCam(%llu): %s, %.3f, %.3f::%.3f::%.3f",
             (unsigned long long)cam->id, cam->active ? "active" : "inactive",
             glm::degrees(cam->fovh), cam->pitch, cam->yaw, cam->roll);
  lua_pushstring(L, out.str());
  return 1;
}
