#include "Camera.h"

/**
        Update camera internals (frustum, position, quaternion)
        \param pos Vector-4 position to set camera to
        \param quaternion Rotation to set camera to
*/
// void DD_Camera::updateCamera(const glm::vec4 pos, const glm::quat quaternion)
// {
//   cam_pos = pos;
//   cam_quat = quaternion;
//   SetupFrustum();
// }

/**
        Calculate new camera frustum for current frame
*/
// void DD_Camera::SetupFrustum() {
//   // recalculate front, right, and up for frustum calculation
//   glm::vec3 front = glm::normalize(cam_quat * cam_front);
//   glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(world_up)));
//   glm::vec3 up = glm::cross(right, front);

//   // calculate new frustum
//   glm::vec3 nearCenter = glm::vec3(glm::vec3(cam_pos) + front * near_plane);
//   glm::vec3 farCenter = glm::vec3(glm::vec3(cam_pos) + front * far_plane);
//   GLfloat tang = tan(fov_h / 2);
//   GLfloat ratio = (scr_height / scr_width);
//   GLfloat wNear = tang * near_plane * 2.0f;
//   GLfloat hNear = wNear * ratio;
//   GLfloat wFar = tang * far_plane * 2.0f;
//   GLfloat hFar = wFar * ratio;

//   // corners
//   cam_frustum.corners[0] = nearCenter + (up * hNear) + (right * wNear);
//   cam_frustum.corners[1] = nearCenter - (up * hNear) + (right * wNear);
//   cam_frustum.corners[2] = nearCenter + (up * hNear) - (right * wNear);
//   cam_frustum.corners[3] = nearCenter - (up * hNear) - (right * wNear);
//   cam_frustum.corners[4] = farCenter + (up * hFar) + (right * wFar);
//   cam_frustum.corners[5] = farCenter - (up * hFar) + (right * wFar);
//   cam_frustum.corners[6] = farCenter + (up * hFar) - (right * wFar);
//   cam_frustum.corners[7] = farCenter - (up * hFar) - (right * wFar);

//   // frustum top
//   glm::vec3 topPos = nearCenter + (up * hNear);
//   glm::vec3 tempVec = glm::normalize(topPos - glm::vec3(cam_pos));
//   glm::vec3 upNorm = glm::cross(tempVec, right);

//   // frustum bottom
//   glm::vec3 botPos = nearCenter - (up * hNear);
//   tempVec = glm::normalize(botPos - glm::vec3(cam_pos));
//   glm::vec3 downNorm = glm::cross(right, tempVec);

//   // frustum left
//   glm::vec3 leftPos = nearCenter - (right * wNear);
//   tempVec = glm::normalize(leftPos - glm::vec3(cam_pos));
//   glm::vec3 leftNorm = glm::cross(tempVec, up);

//   // frustum right normal ---> (nc + left * Wnear / 2) - p
//   glm::vec3 rightPos = nearCenter + (right * wNear);
//   tempVec = glm::normalize(rightPos - glm::vec3(cam_pos));
//   glm::vec3 rightNorm = glm::cross(up, tempVec);

//   // points
//   cam_frustum.points[0] = nearCenter;
//   cam_frustum.points[1] = farCenter;
//   cam_frustum.points[2] = rightPos;
//   cam_frustum.points[3] = leftPos;
//   cam_frustum.points[4] = topPos;
//   cam_frustum.points[5] = botPos;

//   // normals
//   cam_frustum.normals[0] = front;
//   cam_frustum.normals[1] = -front;
//   cam_frustum.normals[2] = rightNorm;
//   cam_frustum.normals[3] = leftNorm;
//   cam_frustum.normals[4] = upNorm;
//   cam_frustum.normals[5] = downNorm;

//   // D
//   cam_frustum.d[0] = -glm::dot(cam_frustum.normals[0],
//   cam_frustum.points[0]);
//   cam_frustum.d[1] = -glm::dot(cam_frustum.normals[1],
//   cam_frustum.points[1]);
//   cam_frustum.d[2] = -glm::dot(cam_frustum.normals[2],
//   cam_frustum.points[2]);
//   cam_frustum.d[3] = -glm::dot(cam_frustum.normals[3],
//   cam_frustum.points[3]);
//   cam_frustum.d[4] = -glm::dot(cam_frustum.normals[4],
//   cam_frustum.points[4]);
//   cam_frustum.d[5] = -glm::dot(cam_frustum.normals[5],
//   cam_frustum.points[5]);
// }

/*
        Calculate and return view matrix
*/
// glm::mat4 CamSpace::GetViewMatrix(const DD_Camera* cam) {
//   const glm::vec3 pos = glm::vec3(cam->pos());

//   glm::vec3 front = glm::normalize(cam->camQuat() * cam->front());
//   // cam->setFVec(glm::vec4(front.x, front.y, front.z, 1.f));

//   glm::vec3 right =
//       glm::normalize(glm::cross(front, glm::vec3(cam->worldUp())));
//   glm::vec3 up = glm::normalize(glm::cross(right, front));

//   return glm::lookAt(pos, pos + front, up);
// }

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
        Calculate and return perspective projection matrix
*/
// glm::mat4 CamSpace::GetPerspecProjMatrix(const DD_Camera* cam) {
//   return glm::perspectiveFov(cam->fov_h, cam->scr_width, cam->scr_height,
//                              cam->near_plane, cam->far_plane);
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

/*
        \todo remove all references
*/
// void CamSpace::PrintInfo(const DD_Camera& cam) {
//   printf("\nCamera ID: %s\n", cam.m_ID.c_str());

//   glm::vec4 pos = cam.pos();
//   printf("\tPosition: %.3f, %.3f, %.3f\n", pos.x, pos.y, pos.z);
//   glm::vec4 up = cam.worldUp();
//   printf("\tUp: %.3f, %.3f, %.3f\n", up.x, up.y, up.z);
//   glm::quat q = cam.camQuat();

//   printf("\tRotation (quat): %.3f, %.3f, %.3f, %.3f\n", q.x, q.y, q.z, q.w);
//   printf("\tFOV horizontal: %.1f degrees\n", glm::degrees(cam.fov_h));
//   printf("\tNear plane: %.3f\n", cam.near_plane);
//   printf("\tFar plane: %.3f\n", cam.far_plane);
//   if (cam.active) {
//     printf("\tActive: True\n");
//   } else {
//     printf("\tActive: False\n");
//   }
// }
