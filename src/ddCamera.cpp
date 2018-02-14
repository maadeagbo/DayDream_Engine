#include "ddCamera.h"

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

namespace {
	const char* meta_name = "LuaClass.ddCam";
}

const char* ddCam_meta_t_name() {
	return meta_name;
}

void log_metatable_ddCam(lua_State *L) {
	
}